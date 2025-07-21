#include "plugin.hpp"
#include <cmath>

struct InvertedRangeParamQuantity : rack::engine::ParamQuantity {
    float displayMin, displayMax;
    InvertedRangeParamQuantity(float min, float max, std::string paramName)
        : displayMin(min), displayMax(max) {
        name = paramName;
    }
    float getDisplayValue() override {
        return displayMax - getValue() * (displayMax - displayMin);
    }
    std::string getDisplayValueString() override {
        return rack::string::f("%.1f Hz", getDisplayValue());
    }
};

struct Biquad {
    float b0 = 1.f, b1 = 0.f, b2 = 0.f;
    float a1 = 0.f, a2 = 0.f;

    float x1 = 0.f, x2 = 0.f;
    float y1 = 0.f, y2 = 0.f;

    void setupLowpass(float cutoff, float resonance, float sampleRate) {
        float w0 = 2.f * M_PI * cutoff / sampleRate;
        float Q = resonance * 5.f + 0.1f;
        float alpha = sinf(w0) / (2.f * Q);
        float cosw0 = cosf(w0);

        b0 = (1.f - cosw0) * 0.5f;
        b1 = 1.f - cosw0;
        b2 = b0;
        float a0 = 1.f + alpha;
        a1 = -2.f * cosw0;
        a2 = 1.f - alpha;

        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }

    void setupHighpass(float cutoff, float resonance, float sampleRate) {
        float w0 = 2.f * M_PI * cutoff / sampleRate;
        float Q = resonance * 5.f + 0.1f;
        float alpha = sinf(w0) / (2.f * Q);
        float cosw0 = cosf(w0);

        b0 = (1.f + cosw0) * 0.5f;
        b1 = -(1.f + cosw0);
        b2 = b0;
        float a0 = 1.f + alpha;
        a1 = -2.f * cosw0;
        a2 = 1.f - alpha;

        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }

    float process(float in) {
        float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = in;
        y2 = y1;
        y1 = out;
        return out;
    }

    void reset() {
        x1 = x2 = y1 = y2 = 0.f;
    }
};

struct Bitcrusher : Module {
    enum ParamId {
        SAMPLERATE_PARAM, BITDEPTH_PARAM, DRY_WET_PARAM,
        CUTOFF_PARAM, RESONANCE_PARAM, FILTERTYPE_PARAM,
        VOLUME_PARAM, PARAMS_LEN
    };
    enum InputId {
        SAMPLERATECVIN_INPUT, BITDEPTHCVIN_INPUT, DRY_WETCVIN_INPUT,
        CUTOFFCVIN_INPUT, RESONANCECVIN_INPUT, VOLUMECVIN_INPUT,
        AUDIOLEFTIN_INPUT, AUDIORIGHTIN_INPUT, INPUTS_LEN
    };
    enum OutputId {
        AUDIOLEFTOUT_OUTPUT, AUDIORIGHTOUT_OUTPUT, OUTPUTS_LEN
    };
    enum LightId { LIGHTS_LEN };

    const float sampleRateMinHz = 20.f;
    const float sampleRateMaxHz = 15000.f;
    const float cutoffMinHz = 20.f;
    const float cutoffMaxHz = 8000.f;

    float sampleHoldPhase = 0.f;
    float leftSampleHold = 0.f;
    float rightSampleHold = 0.f;

    Biquad filterL, filterR;

    Bitcrusher() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(SAMPLERATE_PARAM, 0.f, 1.f, 0.f, "Clock Frequency");
        configSwitch(BITDEPTH_PARAM, 0.f, 15.f, 0.f, "Bit Depth", {
            "16", "15", "14", "13", "12", "11", "10", "9", "8",
            "7", "6", "5", "4", "3", "2", "1"
        });
        configParam(DRY_WET_PARAM, 0.f, 1.f, 1.f, "Dry/Wet", "%", 0.f, 100.f);
        configParam(CUTOFF_PARAM, 0.f, 1.f, 1.f, "Cutoff", "hz", 400.f, 20.f);
        configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);
        configSwitch(FILTERTYPE_PARAM, 0.f, 1.f, 0.f, "Filter Type", {"Lowpass", "Highpass"});
        configParam(VOLUME_PARAM, 0.f, 1.f, 1.f, "Volume", "%", 0.f, 100.f);

        configInput(SAMPLERATECVIN_INPUT, "Sample Rate CV");
        configInput(BITDEPTHCVIN_INPUT, "Bit Depth CV");
        configInput(DRY_WETCVIN_INPUT, "Dry/Wet CV");
        configInput(CUTOFFCVIN_INPUT, "Cutoff CV");
        configInput(RESONANCECVIN_INPUT, "Resonance CV");
        configInput(VOLUMECVIN_INPUT, "Volume CV");
        configInput(AUDIOLEFTIN_INPUT, "Audio Left");
        configInput(AUDIORIGHTIN_INPUT, "Audio Right");

        configOutput(AUDIOLEFTOUT_OUTPUT, "Audio Left");
        configOutput(AUDIORIGHTOUT_OUTPUT, "Audio Right");

        paramQuantities[SAMPLERATE_PARAM] = new InvertedRangeParamQuantity(sampleRateMinHz, sampleRateMaxHz, "Clock Frequency");
        paramQuantities[SAMPLERATE_PARAM]->module = this;
        paramQuantities[SAMPLERATE_PARAM]->paramId = SAMPLERATE_PARAM;
    }

    void process(const ProcessArgs& args) override {
        float leftIn = inputs[AUDIOLEFTIN_INPUT].getVoltage();
        float rightIn = inputs[AUDIORIGHTIN_INPUT].isConnected() ? inputs[AUDIORIGHTIN_INPUT].getVoltage() : leftIn;

        float norm = clamp(params[SAMPLERATE_PARAM].getValue() + clamp(inputs[SAMPLERATECVIN_INPUT].getVoltage(), -5.f, 5.f) / 10.f, 0.f, 1.f);
        // Invert norm for the sample rate since param is inverted display
        float sampleRateHz = sampleRateMinHz + (1.f - norm) * (sampleRateMaxHz - sampleRateMinHz);

        float holdInterval = args.sampleRate / sampleRateHz;
        sampleHoldPhase += 1.f;
        if (sampleHoldPhase >= holdInterval) {
            sampleHoldPhase -= holdInterval;
            leftSampleHold = leftIn;
            rightSampleHold = rightIn;
        }

        float bitCV = clamp(inputs[BITDEPTHCVIN_INPUT].getVoltage(), -5.f, 5.f) / 5.f * 15.f;
        int bitDepth = (int)clamp(15.f - (params[BITDEPTH_PARAM].getValue() + bitCV), 0.f, 15.f);

        auto bitcrush = [&](float in) {
            if (bitDepth >= 15) return in;
            float norm = clamp((in + 5.f) / 10.f, 0.f, 1.f);
            float quantized = (bitDepth <= 0) ? (norm >= 0.5f ? 1.f : 0.f) : std::round(norm * ((1 << bitDepth) - 1)) / ((1 << bitDepth) - 1);
            return quantized * 10.f - 5.f;
        };

        float crushedL = bitcrush(leftSampleHold);
        float crushedR = bitcrush(rightSampleHold);

        float cutoffCV = clamp(inputs[CUTOFFCVIN_INPUT].getVoltage(), -5.f, 5.f) / 10.f;
        float cutoff = cutoffMinHz + clamp(params[CUTOFF_PARAM].getValue() + cutoffCV, 0.f, 1.f) * (cutoffMaxHz - cutoffMinHz);
        float resonance = clamp(params[RESONANCE_PARAM].getValue() + clamp(inputs[RESONANCECVIN_INPUT].getVoltage(), -5.f, 5.f) / 10.f, 0.f, 1.f);

        if (params[FILTERTYPE_PARAM].getValue() < 0.5f) {
            filterL.setupLowpass(cutoff, resonance, args.sampleRate);
            filterR.setupLowpass(cutoff, resonance, args.sampleRate);
        } else {
            filterL.setupHighpass(cutoff, resonance, args.sampleRate);
            filterR.setupHighpass(cutoff, resonance, args.sampleRate);
        }

        float filteredL = filterL.process(crushedL);
        float filteredR = filterR.process(crushedR);

        float dryWet = clamp(params[DRY_WET_PARAM].getValue() + clamp(inputs[DRY_WETCVIN_INPUT].getVoltage(), -5.f, 5.f) / 10.f, 0.f, 1.f);
        float volume = clamp(params[VOLUME_PARAM].getValue() + clamp(inputs[VOLUMECVIN_INPUT].getVoltage(), -5.f, 5.f) / 10.f, 0.f, 1.f);

        float outL = rack::math::crossfade(leftIn, filteredL, dryWet) * volume;
        float outR = rack::math::crossfade(rightIn, filteredR, dryWet) * volume;

        outputs[AUDIOLEFTOUT_OUTPUT].setVoltage(clamp(outL, -5.f, 5.f));
        outputs[AUDIORIGHTOUT_OUTPUT].setVoltage(clamp(outR, -5.f, 5.f));
    }
};

struct BitcrusherWidget : ModuleWidget {
	BitcrusherWidget(Bitcrusher* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Bitcrusher_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies_large>(mm2px(Vec(14.499, 18.5)), module, Bitcrusher::SAMPLERATE_PARAM));
		addParam(createParamCentered<Davies_large>(mm2px(Vec(46.5, 18.5)), module, Bitcrusher::BITDEPTH_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(30.502, 41.751)), module, Bitcrusher::DRY_WET_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(13.501, 58.998)), module, Bitcrusher::CUTOFF_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(47.502, 58.998)), module, Bitcrusher::RESONANCE_PARAM));
		addParam(createParamCentered<_2PosHorizontal>(mm2px(Vec(13.501, 95.002)), module, Bitcrusher::FILTERTYPE_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(47.502, 94.999)), module, Bitcrusher::VOLUME_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(13.501, 41.751)), module, Bitcrusher::SAMPLERATECVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.502, 41.751)), module, Bitcrusher::BITDEPTHCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(13.501, 77.963)), module, Bitcrusher::CUTOFFCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.502, 77.963)), module, Bitcrusher::DRY_WETCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.502, 77.963)), module, Bitcrusher::RESONANCECVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.502, 94.999)), module, Bitcrusher::VOLUMECVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.1, 111.001)), module, Bitcrusher::AUDIOLEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.001, 111.001)), module, Bitcrusher::AUDIORIGHTIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.1, 111.001)), module, Bitcrusher::AUDIOLEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(53.002, 111.001)), module, Bitcrusher::AUDIORIGHTOUT_OUTPUT));
	}
};


Model* modelBitcrusher = createModel<Bitcrusher, BitcrusherWidget>("Bitcrusher");