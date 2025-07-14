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
    float a0 = 1.f, a1 = 0.f, a2 = 0.f;
    float b1 = 0.f, b2 = 0.f;
    float z1 = 0.f, z2 = 0.f;

    void setupLowpass(float cutoff, float resonance, float sampleRate) {
        float w0 = 2.f * M_PI * cutoff / sampleRate;
        float alpha = sinf(w0) / (2.f * ((1.f - resonance) * 5.f + 0.001f));
        float cosw0 = cosf(w0);
        float norm = 1.f / (1.f + alpha);
        a0 = (1.f - cosw0) * 0.5f * norm;
        a1 = (1.f - cosw0) * norm;
        a2 = a0;
        b1 = -2.f * cosw0 * norm;
        b2 = (1.f - alpha) * norm;
    }

    float process(float in) {
        float out = a0 * in + a1 * z1 + a2 * z2 - b1 * z1 - b2 * z2;
        z2 = z1;
        z1 = out;
        return out;
    }

    void reset() {
        z1 = z2 = 0.f;
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
        configParam(DRY_WET_PARAM, 0.f, 1.f, 1.f, "Dry/Wet");
        configParam(CUTOFF_PARAM, 0.f, 1.f, 1.f, "Cutoff");
        configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance");
        configSwitch(FILTERTYPE_PARAM, 0.f, 1.f, 0.f, "Filter Type", {"Lowpass", "Highpass"});
        configParam(VOLUME_PARAM, 0.f, 1.f, 1.f, "Volume");

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

        paramQuantities[SAMPLERATE_PARAM] = new InvertedRangeParamQuantity(sampleRateMaxHz, sampleRateMinHz, "Clock Frequency");
        paramQuantities[SAMPLERATE_PARAM]->module = this;
        paramQuantities[SAMPLERATE_PARAM]->paramId = SAMPLERATE_PARAM;
    }

    void process(const ProcessArgs& args) override {
        float leftIn = inputs[AUDIOLEFTIN_INPUT].getVoltage();
        float rightIn = inputs[AUDIORIGHTIN_INPUT].isConnected() ? inputs[AUDIORIGHTIN_INPUT].getVoltage() : leftIn;

        float t = params[SAMPLERATE_PARAM].getValue() + clamp(inputs[SAMPLERATECVIN_INPUT].getVoltage(), -5.f, 5.f) / 10.f;
        float sampleRateHz = sampleRateMinHz + clamp(t, 0.f, 1.f) * (sampleRateMaxHz - sampleRateMinHz);
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

        filterL.setupLowpass(cutoff, resonance, args.sampleRate);
        filterR.setupLowpass(cutoff, resonance, args.sampleRate);

        float filteredL = filterL.process(crushedL);
        float filteredR = filterR.process(crushedR);

        float dryWet = clamp(params[DRY_WET_PARAM].getValue() + clamp(inputs[DRY_WETCVIN_INPUT].getVoltage(), -5.f, 5.f) / 10.f, 0.f, 1.f);
        float volume = clamp(params[VOLUME_PARAM].getValue() + clamp(inputs[VOLUMECVIN_INPUT].getVoltage(), -5.f, 5.f) / 10.f, 0.f, 1.f);

        float outL = crossfade(leftIn, filteredL, dryWet) * volume;
        float outR = crossfade(rightIn, filteredR, dryWet) * volume;

        outputs[AUDIOLEFTOUT_OUTPUT].setVoltage(clamp(outL, -5.f, 5.f));
        outputs[AUDIORIGHTOUT_OUTPUT].setVoltage(clamp(outR, -5.f, 5.f));
    }
};


struct BitcrusherWidget : ModuleWidget {
	BitcrusherWidget(Bitcrusher* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Bitcrusher.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(8.924, 19.095)), module, Bitcrusher::SAMPLERATE_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(31.302, 19.095)), module, Bitcrusher::BITDEPTH_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(20.32, 44.599)), module, Bitcrusher::DRY_WET_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(8.924, 67.068)), module, Bitcrusher::CUTOFF_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(31.302, 67.068)), module, Bitcrusher::RESONANCE_PARAM));
		addParam(createParamCentered<_2PosHorizontal>(mm2px(Vec(20.32, 85.948)), module, Bitcrusher::FILTERTYPE_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(20.32, 95.631)), module, Bitcrusher::VOLUME_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31.302, 95.631)), module, Bitcrusher::VOLUMECVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.924, 29.117)), module, Bitcrusher::SAMPLERATECVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31.302, 29.117)), module, Bitcrusher::BITDEPTHCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.32, 53.904)), module, Bitcrusher::DRY_WETCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.924, 77.047)), module, Bitcrusher::CUTOFFCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31.302, 77.047)), module, Bitcrusher::RESONANCECVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.585, 117.107)), module, Bitcrusher::AUDIOLEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.493, 117.107)), module, Bitcrusher::AUDIORIGHTIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(27.136, 117.107)), module, Bitcrusher::AUDIOLEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(36.083, 117.107)), module, Bitcrusher::AUDIORIGHTOUT_OUTPUT));
	}
};


Model* modelBitcrusher = createModel<Bitcrusher, BitcrusherWidget>("Bitcrusher");