#include "plugin.hpp"
#include <cmath>

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

struct HighShelf {
    float a0 = 1.f, a1 = 0.f, a2 = 0.f, b1 = 0.f, b2 = 0.f;
    float a0_target = 1.f, a1_target = 0.f, a2_target = 0.f, b1_target = 0.f, b2_target = 0.f;
    float x1 = 0.f, x2 = 0.f, y1 = 0.f, y2 = 0.f;
    float smoothing = 0.01f;

    void calcTargetCoeffs(float sampleRate, float freq, float gainDB) {
        float nyquist = sampleRate * 0.5f;
        if (freq > nyquist * 0.95f) freq = nyquist * 0.95f;

        float A = powf(10.f, gainDB / 40.f);
        float w0 = 2.f * M_PI * freq / sampleRate;
        float cosw0 = cosf(w0);
        float sinw0 = sinf(w0);
        float alpha = sinw0 / (2.f * 0.707f);

        float sqrtA = sqrtf(A);

        float b0 = A * ((A + 1.f) + (A - 1.f) * cosw0 + 2.f * sqrtA * alpha);
        float b1_ = -2.f * A * ((A - 1.f) + (A + 1.f) * cosw0);
        float b2 = A * ((A + 1.f) + (A - 1.f) * cosw0 - 2.f * sqrtA * alpha);
        float a0_ = (A + 1.f) - (A - 1.f) * cosw0 + 2.f * sqrtA * alpha;
        float a1_ = 2.f * ((A - 1.f) - (A + 1.f) * cosw0);
        float a2_ = (A + 1.f) - (A - 1.f) * cosw0 - 2.f * sqrtA * alpha;

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
        x2 = x1; x1 = x0;
        y2 = y1; y1 = y0;
        return y0;
    }
};

struct MidPeakingEQ {
    float a0 = 1.f, a1 = 0.f, a2 = 0.f, b1 = 0.f, b2 = 0.f;
    float a0_target = 1.f, a1_target = 0.f, a2_target = 0.f, b1_target = 0.f, b2_target = 0.f;
    float x1 = 0.f, x2 = 0.f, y1 = 0.f, y2 = 0.f;
    float smoothing = 0.01f;

    void calcTargetCoeffs(float sampleRate, float freq, float gainDB, float Q = 0.5f) {
        float nyquist = sampleRate * 0.5f;
        if (freq > nyquist * 0.95f) freq = nyquist * 0.95f;

        float A = powf(10.f, gainDB / 40.f);
        float w0 = 2.f * M_PI * freq / sampleRate;
        float cosw0 = cosf(w0);
        float sinw0 = sinf(w0);
        float alpha = sinw0 / (2.f * Q);

        float b0 = 1.f + alpha * A;
        float b1_ = -2.f * cosw0;
        float b2 = 1.f - alpha * A;
        float a0_ = 1.f + alpha / A;
        float a1_ = -2.f * cosw0;
        float a2_ = 1.f - alpha / A;

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
        x2 = x1; x1 = x0;
        y2 = y1; y1 = y0;
        return y0;
    }
};

struct LowShelf {
    float a0=1.f, a1=0.f, a2=0.f, b1=0.f, b2=0.f;
    float a0_target=1.f, a1_target=0.f, a2_target=0.f, b1_target=0.f, b2_target=0.f;
    float x1=0.f, x2=0.f, y1=0.f, y2=0.f;
    float smoothing=0.01f;

    void calcTargetCoeffs(float sampleRate, float freq, float gainDB) {
        float nyquist = sampleRate * 0.5f;
        if (freq > nyquist * 0.95f) freq = nyquist * 0.95f;

        float A = powf(10.f, gainDB / 40.f);
        float w0 = 2.f * M_PI * freq / sampleRate;
        float cosw0 = cosf(w0);
        float sinw0 = sinf(w0);

        float alpha = sinw0 / 1.5f;  // approx 6dB/oct broad Q for Neve 1073 style

        float sqrtA = sqrtf(A);

        float b0 = A*((A+1.f) - (A-1.f)*cosw0 + 2.f*sqrtA*alpha);
        float b1_ = 2.f*A*((A-1.f) - (A+1.f)*cosw0);
        float b2 = A*((A+1.f) - (A-1.f)*cosw0 - 2.f*sqrtA*alpha);
        float a0_ = (A+1.f) + (A-1.f)*cosw0 + 2.f*sqrtA*alpha;
        float a1_ = -2.f*((A-1.f) + (A+1.f)*cosw0);
        float a2_ = (A+1.f) + (A-1.f)*cosw0 - 2.f*sqrtA*alpha;

        if (fabsf(a0_) < 1e-10f) a0_ = 1e-10f;

        a0_target = b0/a0_;
        a1_target = b1_/a0_;
        a2_target = b2/a0_;
        b1_target = a1_/a0_;
        b2_target = a2_/a0_;
    }

    void smoothCoeffs() {
        a0 += smoothing*(a0_target - a0);
        a1 += smoothing*(a1_target - a1);
        a2 += smoothing*(a2_target - a2);
        b1 += smoothing*(b1_target - b1);
        b2 += smoothing*(b2_target - b2);
    }

    float process(float x0) {
        smoothCoeffs();
        float y0 = a0*x0 + a1*x1 + a2*x2 - b1*y1 - b2*y2;
        x2 = x1; x1 = x0;
        y2 = y1; y1 = y0;
        return y0;
    }
};

struct _70sEQ : Module {
    enum ParamId {
        GAIN_PARAM,
        HIGH_SHELF_PARAM,
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

    HighShelf highShelfL, highShelfR;
    MidPeakingEQ midBandL, midBandR;
    LowShelf lowShelfL, lowShelfR;

    _70sEQ() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(GAIN_PARAM, 0.f, 1.f, 0.2f, "Gain", "x", 0.f, 5.f);
        configParam(HIGH_SHELF_PARAM, -15.f, 15.f, 0.f, "High Shelf Gain", "dB");
        configParam(MID_PARAM, -15.f, 15.f, 0.f, "Mid Gain", "dB");
        configSwitch(MIDFREQSELECT_PARAM, 0.f, 6.f, 0.f, "Mid Freq Select",
            {"Off", "360hz", "700hz", "1.6khz", "3.2khz", "4.8khz", "7.2khz"});
        configParam(LOW_SHELF_PARAM, -15.f, 15.f, 0.f, "Low Shelf Gain", "dB");
        configSwitch(LOWFREQSELECT_PARAM, 0.f, 4.f, 0.f, "Low Freq Select",
            {"Off", "35hz", "60hz", "110hz", "220hz"});
        configSwitch(HIGHPASSFREQSELECT_PARAM, 0.f, 4.f, 0.f, "Highpass Freq Select",
            {"Off", "50hz", "80hz", "160hz", "300hz"});
        configParam(OUTPUTVOL_PARAM, 0.f, 2.f, 1.f, "Output Level", "x");
        configInput(INL_INPUT, "Audio Left");
        configInput(INR_INPUT, "Audio Right");
        configOutput(OUTL_OUTPUT, "Output Left");
        configOutput(OUTR_OUTPUT, "Output Right");
        configLight(GAINLED_LIGHT, "Gain LED");
        configLight(OUTLED_LIGHT, "Output LED");
    }

    float freqSelectToFreq(int sel, bool isMid) {
        if (isMid) {
            switch(sel) {
                case 1: return 360.f;
                case 2: return 700.f;
                case 3: return 1600.f;
                case 4: return 3200.f;
                case 5: return 4800.f;
                case 6: return 7200.f;
                default: return 0.f;
            }
        } else {
            switch(sel) {
                case 1: return 35.f;
                case 2: return 60.f;
                case 3: return 110.f;
                case 4: return 220.f;
                default: return 0.f;
            }
        }
    }

    float highpassFreqSelectToFreq(int sel) {
        switch(sel) {
            case 1: return 50.f;
            case 2: return 80.f;
            case 3: return 160.f;
            case 4: return 300.f;
            default: return 0.f;
        }
    }

    void process(const ProcessArgs& args) override {
        float inL = inputs[INL_INPUT].getVoltage();
        float inR = inputs[INR_INPUT].getVoltage();
    
        float gain = params[GAIN_PARAM].getValue();          // 0..1 normalized
        float outputVol = params[OUTPUTVOL_PARAM].getValue(); // 0..2x
    
        // Apply input gain boost from 1x to 5x
        float inputGain = 1.f + 4.f * gain;  // maps 0..1 -> 1..5
        inL *= inputGain;
        inR *= inputGain;
    
        // Clip input signal to ±5V (10V peak-to-peak)
        inL = std::fmax(std::fmin(inL, 5.f), -5.f);
        inR = std::fmax(std::fmin(inR, 5.f), -5.f);
    
        // --- EQ Processing stages ---
    
        // Highpass processing (3 stages)
        float hpFreq = highpassFreqSelectToFreq((int)params[HIGHPASSFREQSELECT_PARAM].getValue());
        float alpha = 0.f;
        if (hpFreq > 0.f) {
            float rc = 1.f / (2.f * M_PI * hpFreq);
            float dt = 1.f / args.sampleRate;
            alpha = rc / (rc + dt);
            inL = highpassL1.process(inL, alpha);
            inL = highpassL2.process(inL, alpha);
            inL = highpassL3.process(inL, alpha);
            inR = highpassR1.process(inR, alpha);
            inR = highpassR2.process(inR, alpha);
            inR = highpassR3.process(inR, alpha);
        }
    
        // Mid band
        int midFreqSel = (int)params[MIDFREQSELECT_PARAM].getValue();
        float midFreq = freqSelectToFreq(midFreqSel, true);
        float midGainDB = params[MID_PARAM].getValue();
        if (midFreq > 0.f) {
            midBandL.calcTargetCoeffs(args.sampleRate, midFreq, midGainDB, 0.5f);
            midBandR.calcTargetCoeffs(args.sampleRate, midFreq, midGainDB, 0.5f);
            inL = midBandL.process(inL);
            inR = midBandR.process(inR);
        }
    
        // High shelf
        float highShelfFreq = 10000.f;
        float highShelfGainDB = params[HIGH_SHELF_PARAM].getValue();
        highShelfL.calcTargetCoeffs(args.sampleRate, highShelfFreq, highShelfGainDB);
        highShelfR.calcTargetCoeffs(args.sampleRate, highShelfFreq, highShelfGainDB);
        inL = highShelfL.process(inL);
        inR = highShelfR.process(inR);
    
        // Low shelf
        int lowFreqSel = (int)params[LOWFREQSELECT_PARAM].getValue();
        float lowFreq = freqSelectToFreq(lowFreqSel, false);
        float lowGainDB = params[LOW_SHELF_PARAM].getValue();
        if (lowFreq > 0.f) {
            lowShelfL.calcTargetCoeffs(args.sampleRate, lowFreq, lowGainDB);
            lowShelfR.calcTargetCoeffs(args.sampleRate, lowFreq, lowGainDB);
            inL = lowShelfL.process(inL);
            inR = lowShelfR.process(inR);
        }
    
        // Apply output volume
        inL *= outputVol;
        inR *= outputVol;
    
        outputs[OUTL_OUTPUT].setVoltage(inL);
        outputs[OUTR_OUTPUT].setVoltage(inR);
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
