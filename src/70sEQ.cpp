#include "plugin.hpp"
#include <cmath>

// --- Highpass filter, unchanged ---
struct Highpass {
    float prevInput = 0.f;
    float prevOutput = 0.f;

    float process(float input, float alpha) {
        float output = alpha * (prevOutput + input - prevInput);
        prevInput = input;
        prevOutput = output;
        return output;
    }
};

// --- Smoothed High Shelf Biquad ---
struct HighShelf {
    float a0 = 1.f, a1 = 0.f, a2 = 0.f, b1 = 0.f, b2 = 0.f;
    float a0_target = 1.f, a1_target = 0.f, a2_target = 0.f, b1_target = 0.f, b2_target = 0.f;

    float x1 = 0.f, x2 = 0.f;
    float y1 = 0.f, y2 = 0.f;

    float smoothing = 0.01f;

    void calcTargetCoeffs(float sampleRate, float freq, float gainDB) {
        float nyquist = sampleRate * 0.5f;
        if (freq > nyquist * 0.95f)
            freq = nyquist * 0.95f;

        float A = powf(10.f, gainDB / 40.f);
        float w0 = 2.f * M_PI * freq / sampleRate;
        float cosw0 = cosf(w0);
        float sinw0 = sinf(w0);
        float alpha = sinw0 / (2.f * 0.707f);

        float sqrtA = sqrtf(A);

        float b0 =    A * ((A + 1.f) + (A - 1.f) * cosw0 + 2.f * sqrtA * alpha);
        float b1_ = -2.f * A * ((A - 1.f) + (A + 1.f) * cosw0);
        float b2 =    A * ((A + 1.f) + (A - 1.f) * cosw0 - 2.f * sqrtA * alpha);
        float a0_ =       (A + 1.f) - (A - 1.f) * cosw0 + 2.f * sqrtA * alpha;
        float a1_ =  2.f * ((A - 1.f) - (A + 1.f) * cosw0);
        float a2_ =       (A + 1.f) - (A - 1.f) * cosw0 - 2.f * sqrtA * alpha;

        if (fabsf(a0_) < 1e-10f) a0_ = 1e-10f;

        a0_target = b0 / a0_;
        a1_target = b1_ / a0_;
        a2_target = b2 / a0_;
        b1_target = a1_ / a0_;
        b2_target = a2_ / a0_;
    }

    void smoothCoeffs() {
        a0 += smoothing * (a0_target - a0);
        a1 += smoothing * (a1_target - a1);
        a2 += smoothing * (a2_target - a2);
        b1 += smoothing * (b1_target - b1);
        b2 += smoothing * (b2_target - b2);
    }

    float process(float x0) {
        smoothCoeffs();

        float y0 = a0 * x0 + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
        x2 = x1;
        x1 = x0;
        y2 = y1;
        y1 = y0;
        return y0;
    }
};

struct _70sEQ : Module {
    enum ParamId {
        GAIN_PARAM,
        HIGH_SHELF_PARAM,  // The high shelf gain knob ±15dB
        MID_PARAM,
        MIDFREQSELECT_PARAM,
        LOW_SHELF_PARAM,
        LOWFREQSELECT_PARAM,
        HIGHPASSFREQSELECT_PARAM,
        OUTPUTVOL_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        INL_INPUT,
        INR_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        OUTL_OUTPUT,
        OUTR_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        GAINLED_LIGHT,
        OUTLED_LIGHT,
        LIGHTS_LEN
    };

    Highpass highpassL1, highpassL2, highpassL3;
    Highpass highpassR1, highpassR2, highpassR3;

    HighShelf highShelfL;
    HighShelf highShelfR;

    _70sEQ() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(GAIN_PARAM, 0.f, 1.f, 0.2f, "Gain", "x", 0.0f, 5.f);
        configParam(HIGH_SHELF_PARAM, -15.f, 15.f, 0.f, "High Shelf Gain", "dB");
        configParam(MID_PARAM, -1.f, 1.f, 0.f, "Mid", "%", 0.f, 100.f);
        configSwitch(MIDFREQSELECT_PARAM, 0.f, 6.f, 0.f, "Mid Freq Select", {"Off", "360hz", "700hz", "1.6khz", "3.2khz", "4.8khz", "7.2khz"});
        configParam(LOW_SHELF_PARAM, -1.f, 1.f, 0.f, "Low", "%", 0.f, 100.f);
        configSwitch(LOWFREQSELECT_PARAM, 0.f, 4.f, 0.f, "Low Freq Select", {"Off", "35hz", "60hz", "110hz", "220hz"});
        configSwitch(HIGHPASSFREQSELECT_PARAM, 0.f, 4.f, 0.f, "Highpass Freq Select", {"Off", "50hz", "80hz", "160hz", "300hz"});
        configParam(OUTPUTVOL_PARAM, 0.f, 1.f, 0.5f, "Output Level", "%", 0.f, 100.f);
        configInput(INL_INPUT, "Audio Left");
        configInput(INR_INPUT, "Audio Right");
        configOutput(OUTL_OUTPUT, "Audio Left");
        configOutput(OUTR_OUTPUT, "Audio Right");
    }

    void process(const ProcessArgs& args) override {
        float inL = inputs[INL_INPUT].getVoltage();
        float inR = inputs[INR_INPUT].getVoltage();

        float gain = params[GAIN_PARAM].getValue() * 5.f;
        inL *= gain;
        inR *= gain;

        inL = clamp(inL, -5.f, 5.f);
        inR = clamp(inR, -5.f, 5.f);

        int hpSel = (int)params[HIGHPASSFREQSELECT_PARAM].getValue();
        if (hpSel > 0) {
            static const float cutoffTable[] = { 50.f, 80.f, 160.f, 300.f };
            int cutoffIndex = clamp(hpSel - 1, 0, 3);
            float cutoff = cutoffTable[cutoffIndex];

            float sr = args.sampleRate;
            float RC = 1.0f / (2.0f * M_PI * cutoff);
            float alpha = RC / (RC + 1.f / sr);

            inL = highpassL1.process(inL, alpha);
            inL = highpassL2.process(inL, alpha);
            inL = highpassL3.process(inL, alpha);

            inR = highpassR1.process(inR, alpha);
            inR = highpassR2.process(inR, alpha);
            inR = highpassR3.process(inR, alpha);
        }

        float highShelfGainDB = params[HIGH_SHELF_PARAM].getValue();
        float highShelfFreq = 12000.f; // Center freq set to 12 kHz as requested

        highShelfL.calcTargetCoeffs(args.sampleRate, highShelfFreq, highShelfGainDB);
        highShelfR.calcTargetCoeffs(args.sampleRate, highShelfFreq, highShelfGainDB);

        inL = highShelfL.process(inL);
        inR = highShelfR.process(inR);

        float outGain = params[OUTPUTVOL_PARAM].getValue();
        inL *= outGain;
        inR *= outGain;

        outputs[OUTL_OUTPUT].setVoltage(inL);
        outputs[OUTR_OUTPUT].setVoltage(inR);

        lights[GAINLED_LIGHT].setBrightness(fminf(gain / 5.f, 1.f));
        lights[OUTLED_LIGHT].setBrightness(fminf(outGain, 1.f));
    }
};

struct _70sEQWidget : ModuleWidget {
    _70sEQWidget(_70sEQ* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/70sEQ.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.661, 16.659)), module, _70sEQ::GAIN_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.24, 30.105)), module, _70sEQ::HIGH_SHELF_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.661, 43.796)), module, _70sEQ::MID_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(23.688, 43.796)), module, _70sEQ::MIDFREQSELECT_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.661, 60.16)), module, _70sEQ::LOW_SHELF_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(23.688, 60.16)), module, _70sEQ::LOWFREQSELECT_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.24, 73.559)), module, _70sEQ::HIGHPASSFREQSELECT_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.661, 88.753)), module, _70sEQ::OUTPUTVOL_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.544, 103.75)), module, _70sEQ::INL_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.544, 116.722)), module, _70sEQ::INR_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.275, 103.75)), module, _70sEQ::OUTL_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.275, 116.722)), module, _70sEQ::OUTR_OUTPUT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(23.688, 16.659)), module, _70sEQ::GAINLED_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(23.688, 88.753)), module, _70sEQ::OUTLED_LIGHT));
    }
};

Model* model_70sEQ = createModel<_70sEQ, _70sEQWidget>("70sEQ");
