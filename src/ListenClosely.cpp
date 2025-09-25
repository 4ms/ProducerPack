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

struct ListenClosely : Module {
	enum ParamId {
		RATIO_PARAM,
		PEAKREDUCTION_PARAM,
		DRYWET_PARAM,
		GAIN_PARAM,
		LOWSHELF_PARAM,
		HIGHSHELF_PARAM,
		MID_PARAM,
		LOWFREQSELECT_PARAM,
		HIGHPASSFREQSELECT_PARAM,
		MIDFREQSELECT_PARAM,
		WIDTH_PARAM,
		OUTPUTVOL_PARAM,
		PREPOST_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		INL_INPUT,
		INR_INPUT,
		WIDTHCV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTL_OUTPUT,
		OUTR_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		GRAPH1_LIGHT,
		GRAPH2_LIGHT,
		GRAPH3_LIGHT,
		GRAPH4_LIGHT,
		GRAPH5_LIGHT,
		GRAPH6_LIGHT,
		GRAPH7_LIGHT,
		GRAPH8_LIGHT,
		GRAPH9_LIGHT,
		CLIPLED_LIGHT,
		_110LED_LIGHT,
		_80LED_LIGHT,
		_160LED_LIGHT,
		_220LED_LIGHT,
		_50LED_LIGHT,
		_300LED_LIGHT,
		_60LED_LIGHT,
		OFFLED_LIGHT,
		_35LED_LIGHT,
		_32KLED_LIGHT,
		_16KLED_LIGHT,
		_700LED_LIGHT,
		_48KLED_LIGHT,
		_360LED_LIGHT,
		_72KLED_LIGHT,
		LIGHTS_LEN
	};

	Highpass highpassL1, highpassL2, highpassL3;
    Highpass highpassR1, highpassR2, highpassR3;

    HighShelf highShelfL, highShelfR;
    MidPeakingEQ midBandL, midBandR;
    LowShelf lowShelfL, lowShelfR;

	ListenClosely() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(RATIO_PARAM, 0.f, 2.f, 0.f, "Ratio", {"Compressor", "Bypass", "Limiter"});
		configSwitch(PREPOST_PARAM, 0.f, 2.f, 0.f, "EQ", {"Pre", "Bypass", "Post"});

		configSwitch(LOWFREQSELECT_PARAM, 0.f, 1.f, 0.f, "Low Frequency Select");
		configSwitch(MIDFREQSELECT_PARAM, 0.f, 1.f, 0.f, "Mid Frequency Select");
		configSwitch(HIGHPASSFREQSELECT_PARAM, 0.f, 1.f, 0.f, "Highpass Frequency Select");

        configParam(PEAKREDUCTION_PARAM, 0.f, 1.f, 0.5f, "Amount", "%", 0.f, 100.f);
		configParam(DRYWET_PARAM, 0.f, 1.f, 1.f, "Dry/Wet", "%", 0.f, 100.f);
        configParam(GAIN_PARAM, 0.f, 1.f, 0.25f, "Gain", "db", 0.f, 40.f);

        configParam(LOWSHELF_PARAM, -15.f, 15.f, 0.f, "Low Shelf Gain", "dB");
		configParam(MID_PARAM, -15.f, 15.f, 0.f, "Mid Gain", "dB");
        configParam(HIGHSHELF_PARAM, -15.f, 15.f, 0.f, "High Shelf Gain", "dB");

		configParam(WIDTH_PARAM, 0.f, 1.f, 0.5f, "Width", "%", 0.f, 200.f);

        configParam(OUTPUTVOL_PARAM, 0.f, 1.f, 0.25f, "Output Level", "x");

		configInput(INL_INPUT, "Audio Left");
		configInput(INR_INPUT, "Audio Right");
		configInput(WIDTHCV_INPUT, "Width CV");
		configOutput(OUTL_OUTPUT, "Audio Left");
		configOutput(OUTR_OUTPUT, "Audio Right");
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

	// Rising edge detection state
float prevHighpassButton = 0.f;
float prevMidButton = 0.f;
float prevLowButton = 0.f;

int hpSel = 0;

int highpassLEDs[5] = {OFFLED_LIGHT, _50LED_LIGHT, _80LED_LIGHT, _160LED_LIGHT, _300LED_LIGHT};


	void process(const ProcessArgs& args) override {
		float inL = inputs[INL_INPUT].getVoltage();
		float inR = inputs[INR_INPUT].isConnected() ? inputs[INR_INPUT].getVoltage() : inL;
	
	//Stereo Width
	float width = params[WIDTH_PARAM].getValue();
	if (inputs[WIDTHCV_INPUT].isConnected())
		width += inputs[WIDTHCV_INPUT].getVoltage() * 0.1f; // same as /10.f

	width = clamp(width, 0.f, 1.f);

	float outLWidth = 0.f, outRWidth = 0.f;

	if (width <= 0.5f) {
		// Blend mono → stereo
		float t = width * 2.f;
		float mono = 0.5f * (inL + inR);
		outLWidth = crossfade(mono, inL, t);
		outRWidth = crossfade(mono, inR, t);
	} else {
		// Stereo widening
		float t = (width <= 0.75f) ? (width - 0.5f) * 4.f : 1.f;
		float boost = (width > 0.75f) ? (width - 0.75f) * 4.f : 0.f;
		float gain = 1.f + boost;

		float diff = 0.5f * (inL - inR);
		outLWidth = inL + t * gain * diff;
		outRWidth = inR - t * gain * diff;
	}

	// EQ 

    float outputVol = params[OUTPUTVOL_PARAM].getValue(); // 0..1
    bool bypassEQ = params[PREPOST_PARAM].getValue() == 1;

// HIGH PASS SELECT BUTTON
float currHighpassButton = params[HIGHPASSFREQSELECT_PARAM].getValue();

if (prevHighpassButton <= 0.5f && currHighpassButton > 0.5f) {
	hpSel = (hpSel + 1) % 5;  // +1 to include 0 ("off")
}
prevHighpassButton = currHighpassButton;

for (int i = 0; i < 5; i++) {
	lights[highpassLEDs[i]].setBrightness(i == hpSel ? 1.f : 0.f);
}

	static float prevHpFreq = -1.f;
    static float hpAlpha = 0.f;

    float hpFreq = highpassFreqSelectToFreq(hpSel);
    if (hpFreq > 0.f) {
        if (hpFreq != prevHpFreq) {
            float rc = 1.f / (2.f * M_PI * hpFreq);
            float dt = 1.f / args.sampleRate;
            hpAlpha = rc / (rc + dt);
            prevHpFreq = hpFreq;
        }

        outLWidth = highpassL1.process(outLWidth, hpAlpha);
        outLWidth = highpassL2.process(outLWidth, hpAlpha);
        outLWidth = highpassL3.process(outLWidth, hpAlpha);
        outRWidth = highpassR1.process(outRWidth, hpAlpha);
        outRWidth = highpassR2.process(outRWidth, hpAlpha);
        outRWidth = highpassR3.process(outRWidth, hpAlpha);
    }

    float eqOutL = outLWidth;
    float eqOutR = outRWidth;

    if (!bypassEQ) {
        // --- Mid band ---
        static float prevMidFreq = -1.f, prevMidGain = -999.f;
        int midSel = (int)params[MIDFREQSELECT_PARAM].getValue();
        static constexpr float midFreqs[7] = {0.f, 360.f, 700.f, 1600.f, 3200.f, 4800.f, 7200.f};
        float midFreq = midFreqs[midSel];
        float midGainDB = params[MID_PARAM].getValue();

        if (midFreq > 0.f && (midFreq != prevMidFreq || midGainDB != prevMidGain)) {
            midBandL.calcTargetCoeffs(args.sampleRate, midFreq, midGainDB, 0.5f);
            midBandR.calcTargetCoeffs(args.sampleRate, midFreq, midGainDB, 0.5f);
            prevMidFreq = midFreq;
            prevMidGain = midGainDB;
        }

        if (midFreq > 0.f) {
            midBandL.smoothCoeffs();
            midBandR.smoothCoeffs();
            eqOutL = midBandL.process(eqOutL);
            eqOutR = midBandR.process(eqOutR);
        }

        // --- High shelf ---
        static float prevHighGain = -999.f;
        float highGainDB = params[HIGHSHELF_PARAM].getValue();
        float highShelfFreq = 10000.f;

        if (highGainDB != prevHighGain) {
            highShelfL.calcTargetCoeffs(args.sampleRate, highShelfFreq, highGainDB);
            highShelfR.calcTargetCoeffs(args.sampleRate, highShelfFreq, highGainDB);
            prevHighGain = highGainDB;
        }

        highShelfL.smoothCoeffs();
        highShelfR.smoothCoeffs();
        eqOutL = highShelfL.process(eqOutL);
        eqOutR = highShelfR.process(eqOutR);

        // --- Low shelf ---
        static float prevLowFreq = -1.f, prevLowGain = -999.f;
        int lowSel = (int)params[LOWFREQSELECT_PARAM].getValue();
        static constexpr float lowFreqs[5] = {0.f, 35.f, 60.f, 110.f, 220.f};
        float lowFreq = lowFreqs[lowSel];
        float lowGainDB = params[LOWSHELF_PARAM].getValue();

        if (lowFreq > 0.f && (lowFreq != prevLowFreq || lowGainDB != prevLowGain)) {
            lowShelfL.calcTargetCoeffs(args.sampleRate, lowFreq, lowGainDB);
            lowShelfR.calcTargetCoeffs(args.sampleRate, lowFreq, lowGainDB);
            prevLowFreq = lowFreq;
            prevLowGain = lowGainDB;
        }

        if (lowFreq > 0.f) {
            lowShelfL.smoothCoeffs();
            lowShelfR.smoothCoeffs();
            eqOutL = lowShelfL.process(eqOutL);
            eqOutR = lowShelfR.process(eqOutR);
        }
    }

    // Output volume
    float outEQL = clamp(eqOutL * outputVol, -10.f, 10.f);
    float outEQR = clamp(eqOutR * outputVol, -10.f, 10.f);

	// Compressor	
		const float peakReduction = params[PEAKREDUCTION_PARAM].getValue();
		const float gainParam = params[GAIN_PARAM].getValue();
		const float dryWet = params[DRYWET_PARAM].getValue();
		const bool isLimiter = params[RATIO_PARAM].getValue() > 0.5f;
		const bool bypass = params[RATIO_PARAM].getValue() == 1.f;
	
		if (bypass) {
			// Early exit for bypass
			outputs[OUTL_OUTPUT].setVoltage(clamp(outEQL, -10.f, 10.f));
			outputs[OUTR_OUTPUT].setVoltage(clamp(outEQR, -10.f, 10.f));
			lights[CLIPLED_LIGHT].setBrightness(0);

			// Shutoff graph
			for (int i = GRAPH1_LIGHT; i <= GRAPH9_LIGHT; ++i) {
				lights[i].setBrightness(0);
			}		
			return;
		}
	
		const float ratio = isLimiter ? 10.f : 3.f;
		const float inputMono = 0.5f * (outEQL + outEQR);
	
		// Envelope follower
		static float env = 0.f;
		const float rectified = std::fabs(inputMono);
	
		// Pre-compute smoothing coefficients once per call
		const float sampleRate = args.sampleRate;
		const float coeffAtk = std::exp(-1.f / (0.01f * sampleRate));
		const float coeffRelFast = std::exp(-1.f / (0.06f * sampleRate));
		const float coeffRelSlow = std::exp(-1.f / (1.5f * sampleRate));
	
		const float releaseCoeff = (env > 0.1f) ? coeffRelFast : coeffRelSlow;
		env = (rectified > env)
			? coeffAtk * env + (1.f - coeffAtk) * rectified
			: releaseCoeff * env + (1.f - releaseCoeff) * rectified;
	
		// Compression gain calculation
		const float threshold = 1.f - peakReduction;
		float gainReduction = 1.f;
		if (env > threshold) {
			const float over = env - threshold;
			gainReduction = 1.f / (1.f + over * (ratio - 1.f));
		}
	
		// Apply compression and gain
		const float gain = std::pow(10.f, gainParam * 2.f);  // db = 0–40, divide by 20 outLWidthine
		float outL = outEQL * (1.f - dryWet) + outEQL * gainReduction * gain * dryWet;
		float outR = outEQR * (1.f - dryWet) + outEQR * gainReduction * gain * dryWet;
	
		outL = clamp(outL, -10.f, 10.f);
		outR = clamp(outR, -10.f, 10.f);

		outputs[OUTL_OUTPUT].setVoltage(outL);
		outputs[OUTR_OUTPUT].setVoltage(outR);
	
		// Compressor LEDs
		const bool clipping = (std::fabs(outL) >= 9.9f || std::fabs(outR) >= 9.9f);
		lights[CLIPLED_LIGHT].setBrightnessSmooth(clipping ? 1.f : 0.f, args.sampleTime);

		// --- Peak Reduction Display (Graph1 to Graph9, scaled and reversed) ---
		float reductionDb = 20.f * std::log10(clamp(gainReduction, 1e-6f, 1.f));  // Avoid log(0)

		// Scale: 0 dB (no compression) → -24 dB (max compression)
		const float minDb = -24.f;  // Adjust based on actual compression range
		const float maxDb = 0.f;

		float normalized = clamp((reductionDb - maxDb) / (minDb - maxDb), 0.f, 1.f);
		int numSegments = (int)std::round(normalized * 9.f);
		numSegments = clamp(numSegments, 0, 9);

		// Light LEDs in reverse (GRAPH1 = max reduction)
		for (int i = 0; i < 9; ++i) {
			int lightIndex = GRAPH9_LIGHT - i;
			bool lit = i < numSegments;
			lights[lightIndex].setBrightnessSmooth(lit ? 1.f : 0.f, args.sampleTime);
		}
	}

};


struct ListenCloselyWidget : ModuleWidget {
	ListenCloselyWidget(ListenClosely* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/ListenClosely_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(12.256, 26.45)), module, ListenClosely::PEAKREDUCTION_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(58.538, 26.45)), module, ListenClosely::DRYWET_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(35.487, 37.026)), module, ListenClosely::GAIN_PARAM));
		
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(12.164, 47.733)), module, ListenClosely::LOWSHELF_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(58.693, 47.733)), module, ListenClosely::HIGHSHELF_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(35.582, 64.355)), module, ListenClosely::MID_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(12.299, 93.217)), module, ListenClosely::WIDTH_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(58.414, 93.263)), module, ListenClosely::OUTPUTVOL_PARAM));

		addParam(createParamCentered<LEDBezel>(mm2px(Vec(12.221, 69.566)), module, ListenClosely::LOWFREQSELECT_PARAM));
		addParam(createParamCentered<LEDBezel>(mm2px(Vec(58.376, 69.573)), module, ListenClosely::HIGHPASSFREQSELECT_PARAM));
		addParam(createParamCentered<LEDBezel>(mm2px(Vec(35.572, 86.054)), module, ListenClosely::MIDFREQSELECT_PARAM));

		addParam(createParamCentered<_3PosHorizontal>(mm2px(Vec(35.402, 100.566)), module, ListenClosely::PREPOST_PARAM));
		addParam(createParamCentered<_3PosHorizontal>(mm2px(Vec(35.381, 22.267)), module, ListenClosely::RATIO_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.963, 113.811)), module, ListenClosely::INL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.969, 113.811)), module, ListenClosely::INR_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35.529, 113.811)), module, ListenClosely::WIDTHCV_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(51.107, 113.811)), module, ListenClosely::OUTL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(62.117, 113.811)), module, ListenClosely::OUTR_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(32.518, 49.664)), module, ListenClosely::CLIPLED_LIGHT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(15.56, 14.72)), module, ListenClosely::GRAPH1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(20.562, 14.72)), module, ListenClosely::GRAPH2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(25.568, 14.72)), module, ListenClosely::GRAPH3_LIGHT));

		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(30.57, 14.72)), module, ListenClosely::GRAPH4_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(35.572, 14.72)), module, ListenClosely::GRAPH5_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(40.577, 14.72)), module, ListenClosely::GRAPH6_LIGHT));

		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(45.579, 14.72)), module, ListenClosely::GRAPH7_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(50.581, 14.72)), module, ListenClosely::GRAPH8_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(55.587, 14.72)), module, ListenClosely::GRAPH9_LIGHT));

		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(16.457, 77.355)), module, ListenClosely::_35LED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(4.365, 72.754)), module, ListenClosely::_60LED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(7.85, 61.391)), module, ListenClosely::_110LED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(20.182, 65.837)), module, ListenClosely::_220LED_LIGHT));

		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(52.871, 76.901)), module, ListenClosely::OFFLED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(50.807, 69.049)), module, ListenClosely::_50LED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(53.802, 61.548)), module, ListenClosely::_80LED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(63.081, 61.51)), module, ListenClosely::_160LED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(65.551, 69.013)), module, ListenClosely::_300LED_LIGHT));

		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(30.67, 93.354)), module, ListenClosely::_360LED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(28.171, 85.468)), module, ListenClosely::_700LED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(30.163, 78.104)), module, ListenClosely::_16KLED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(41.518, 77.913)), module, ListenClosely::_32KLED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(42.723, 85.395)), module, ListenClosely::_48KLED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(40.754, 93.671)), module, ListenClosely::_72KLED_LIGHT));
	}
};


Model* modelListenClosely = createModel<ListenClosely, ListenCloselyWidget>("ListenClosely");