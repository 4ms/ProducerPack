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
		if (freq > nyquist * 0.95f)
			freq = nyquist * 0.95f;

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

		if (fabsf(a0_) < 1e-10f)
			a0_ = 1e-10f;

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

struct MidPeakingEQ {
	float a0 = 1.f, a1 = 0.f, a2 = 0.f, b1 = 0.f, b2 = 0.f;
	float a0_target = 1.f, a1_target = 0.f, a2_target = 0.f, b1_target = 0.f, b2_target = 0.f;
	float x1 = 0.f, x2 = 0.f, y1 = 0.f, y2 = 0.f;
	float smoothing = 0.01f;

	void calcTargetCoeffs(float sampleRate, float freq, float gainDB, float Q = 0.5f) {
		float nyquist = sampleRate * 0.5f;
		if (freq > nyquist * 0.95f)
			freq = nyquist * 0.95f;

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

		if (fabsf(a0_) < 1e-10f)
			a0_ = 1e-10f;

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

struct LowShelf {
	float a0 = 1.f, a1 = 0.f, a2 = 0.f, b1 = 0.f, b2 = 0.f;
	float a0_target = 1.f, a1_target = 0.f, a2_target = 0.f, b1_target = 0.f, b2_target = 0.f;
	float x1 = 0.f, x2 = 0.f, y1 = 0.f, y2 = 0.f;
	float smoothing = 0.01f;

	void calcTargetCoeffs(float sampleRate, float freq, float gainDB) {
		float nyquist = sampleRate * 0.5f;
		if (freq > nyquist * 0.95f)
			freq = nyquist * 0.95f;

		float A = powf(10.f, gainDB / 40.f);
		float w0 = 2.f * M_PI * freq / sampleRate;
		float cosw0 = cosf(w0);
		float sinw0 = sinf(w0);

		float alpha = sinw0 / 1.5f; // approx 6dB/oct broad Q for Neve 1073 style

		float sqrtA = sqrtf(A);

		float b0 = A * ((A + 1.f) - (A - 1.f) * cosw0 + 2.f * sqrtA * alpha);
		float b1_ = 2.f * A * ((A - 1.f) - (A + 1.f) * cosw0);
		float b2 = A * ((A + 1.f) - (A - 1.f) * cosw0 - 2.f * sqrtA * alpha);
		float a0_ = (A + 1.f) + (A - 1.f) * cosw0 + 2.f * sqrtA * alpha;
		float a1_ = -2.f * ((A - 1.f) + (A + 1.f) * cosw0);
		float a2_ = (A + 1.f) + (A - 1.f) * cosw0 - 2.f * sqrtA * alpha;

		if (fabsf(a0_) < 1e-10f)
			a0_ = 1e-10f;

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

struct SeventiesEQ : Module {
	enum ParamId {
		GAIN_PARAM,
		HIGH_SHELF_PARAM,
		MID_PARAM,
		MIDFREQSELECT_PARAM,
		LOW_SHELF_PARAM,
		LOWFREQSELECT_PARAM,
		HIGHPASSFREQSELECT_PARAM,
		OUTPUTVOL_PARAM,
		BYPASS_PARAM,
		PARAMS_LEN
	};
	enum InputId { INL_INPUT, INR_INPUT, INPUTS_LEN };
	enum OutputId { OUTL_OUTPUT, OUTR_OUTPUT, OUTPUTS_LEN };
	enum LightId {
		GAINLED_LIGHT_GREEN,
		GAINLED_LIGHT_RED,
		OUTLED_LIGHT_GREEN,
		OUTLED_LIGHT_RED,
		EQLED_LIGHT_GREEN,
		EQLED_LIGHT_RED,
		LIGHTS_LEN
	};

	Highpass highpassL1, highpassL2, highpassL3;
	Highpass highpassR1, highpassR2, highpassR3;

	HighShelf highShelfL, highShelfR;
	MidPeakingEQ midBandL, midBandR;
	LowShelf lowShelfL, lowShelfR;

	SeventiesEQ() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(GAIN_PARAM, 0.f, 1.f, 0.2f, "Gain", "x", 0.f, 5.f);
		configParam(HIGH_SHELF_PARAM, -15.f, 15.f, 0.f, "High Shelf Gain", "dB");
		configParam(MID_PARAM, -15.f, 15.f, 0.f, "Mid Gain", "dB");
		configSwitch(BYPASS_PARAM, 0.f, 1.f, 0.f, "Bypass", {"Off", "On"}),
			configSwitch(MIDFREQSELECT_PARAM,
						 0.f,
						 6.f,
						 0.f,
						 "Mid Freq Select",
						 {"Off", "360hz", "700hz", "1.6khz", "3.2khz", "4.8khz", "7.2khz"});
		configParam(LOW_SHELF_PARAM, -15.f, 15.f, 0.f, "Low Shelf Gain", "dB");
		configSwitch(LOWFREQSELECT_PARAM, 0.f, 4.f, 0.f, "Low Freq Select", {"Off", "35hz", "60hz", "110hz", "220hz"});
		configSwitch(
			HIGHPASSFREQSELECT_PARAM, 0.f, 4.f, 0.f, "Highpass Freq Select", {"Off", "50hz", "80hz", "160hz", "300hz"});
		configParam(OUTPUTVOL_PARAM, 0.f, 1.f, 0.25f, "Output Level", "x");
		configInput(INL_INPUT, "Audio Left");
		configInput(INR_INPUT, "Audio Right");
		configOutput(OUTL_OUTPUT, "Output Left");
		configOutput(OUTR_OUTPUT, "Output Right");

		configLight(GAINLED_LIGHT_GREEN, "Gain");
		configLight(GAINLED_LIGHT_RED, "Gain");
		configLight(EQLED_LIGHT_GREEN, "EQ");
		configLight(EQLED_LIGHT_RED, "EQ");
		configLight(OUTLED_LIGHT_GREEN, "Output");
		configLight(OUTLED_LIGHT_RED, "Output");
	}

	float freqSelectToFreq(int sel, bool isMid) {
		if (isMid) {
			switch (sel) {
				case 1:
					return 360.f;
				case 2:
					return 700.f;
				case 3:
					return 1600.f;
				case 4:
					return 3200.f;
				case 5:
					return 4800.f;
				case 6:
					return 7200.f;
				default:
					return 0.f;
			}
		} else {
			switch (sel) {
				case 1:
					return 35.f;
				case 2:
					return 60.f;
				case 3:
					return 110.f;
				case 4:
					return 220.f;
				default:
					return 0.f;
			}
		}
	}

	float highpassFreqSelectToFreq(int sel) {
		switch (sel) {
			case 1:
				return 50.f;
			case 2:
				return 80.f;
			case 3:
				return 160.f;
			case 4:
				return 300.f;
			default:
				return 0.f;
		}
	}

	void process(const ProcessArgs &args) override {
		float inL = inputs[INL_INPUT].getVoltage();
		float inR = inputs[INR_INPUT].isConnected() ? inputs[INR_INPUT].getVoltage() : inL;

		float gain = params[GAIN_PARAM].getValue();			  // 0..1 normalized
		float outputVol = params[OUTPUTVOL_PARAM].getValue(); // 0..1
		bool bypass = params[BYPASS_PARAM].getValue() > 0.5f;

		// Apply input gain (1x to 5x)
		float inputGain = 1.f + 4.f * gain;
		inL *= inputGain;
		inR *= inputGain;

		// Clip input signal
		float gainPostAmpLevel = std::max(std::fabs(inL), std::fabs(inR));
		float gainBrightness = gainPostAmpLevel / 5.f;
		bool gainClipping = gainPostAmpLevel >= 5.f;

		inL = std::clamp(inL, -5.f, 5.f);
		inR = std::clamp(inR, -5.f, 5.f);

		lights[GAINLED_LIGHT_GREEN].setBrightnessSmooth(gainClipping ? 0.f : gainBrightness, args.sampleTime);
		lights[GAINLED_LIGHT_RED].setBrightnessSmooth(gainClipping ? gainBrightness : 0.f, args.sampleTime);

		// --- Highpass processing ---
		int hpSel = (int)params[HIGHPASSFREQSELECT_PARAM].getValue();
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

			inL = highpassL1.process(inL, hpAlpha);
			inL = highpassL2.process(inL, hpAlpha);
			inL = highpassL3.process(inL, hpAlpha);
			inR = highpassR1.process(inR, hpAlpha);
			inR = highpassR2.process(inR, hpAlpha);
			inR = highpassR3.process(inR, hpAlpha);
		}

		float eqOutL = inL;
		float eqOutR = inR;

		if (!bypass) {
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
			float highGainDB = params[HIGH_SHELF_PARAM].getValue();
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
			float lowGainDB = params[LOW_SHELF_PARAM].getValue();

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

		// EQ level monitoring
		float eqLevel = std::max(std::fabs(eqOutL), std::fabs(eqOutR));
		float eqBrightness = eqLevel / 5.f;
		bool eqClipping = eqLevel >= 5.f;

		lights[EQLED_LIGHT_GREEN].setBrightnessSmooth(eqClipping ? 0.f : eqBrightness, args.sampleTime);
		lights[EQLED_LIGHT_RED].setBrightnessSmooth(eqClipping ? eqBrightness : 0.f, args.sampleTime);

		// Output volume
		float outL = std::clamp(eqOutL * outputVol, -5.f, 5.f);
		float outR = std::clamp(eqOutR * outputVol, -5.f, 5.f);

		float outLevel = std::max(std::fabs(outL), std::fabs(outR));
		float outBrightness = outLevel / 5.f;
		bool outClipping = outLevel >= 5.f;

		outputs[OUTL_OUTPUT].setVoltage(outL);
		outputs[OUTR_OUTPUT].setVoltage(outR);

		lights[OUTLED_LIGHT_GREEN].setBrightnessSmooth(outClipping ? 0.f : outBrightness, args.sampleTime);
		lights[OUTLED_LIGHT_RED].setBrightnessSmooth(outClipping ? outBrightness : 0.f, args.sampleTime);
	}
};

struct SeventiesEQWidget : ModuleWidget {
	SeventiesEQWidget(SeventiesEQ *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/SeventiesEQ_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(15.286, 15.445)), module, SeventiesEQ::GAIN_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(7.763, 31.53)), module, SeventiesEQ::HIGH_SHELF_PARAM));
		addParam(createParam<Switch2Pos>(mm2px(Vec(20, 28.528)), module, SeventiesEQ::BYPASS_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(7.763, 48.547)), module, SeventiesEQ::MID_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(22.819, 48.547)), module, SeventiesEQ::MIDFREQSELECT_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(7.763, 65.563)), module, SeventiesEQ::LOW_SHELF_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(22.819, 65.563)), module, SeventiesEQ::LOWFREQSELECT_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(7.763, 82.58)), module, SeventiesEQ::HIGHPASSFREQSELECT_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(22.819, 82.58)), module, SeventiesEQ::OUTPUTVOL_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.321, 97.093)), module, SeventiesEQ::INL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.07, 97.093)), module, SeventiesEQ::INR_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.321, 111.109)), module, SeventiesEQ::OUTL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.07, 111.109)), module, SeventiesEQ::OUTR_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(
			mm2px(Vec(26.821, 15.445)), module, SeventiesEQ::GAINLED_LIGHT_GREEN));
		addChild(createLightCentered<MediumLight<RedLight>>(
			mm2px(Vec(26.821, 15.445)), module, SeventiesEQ::GAINLED_LIGHT_RED));
		addChild(createLightCentered<MediumLight<GreenLight>>(
			mm2px(Vec(15.286, 58.558)), module, SeventiesEQ::EQLED_LIGHT_GREEN));
		addChild(createLightCentered<MediumLight<RedLight>>(
			mm2px(Vec(15.286, 58.558)), module, SeventiesEQ::EQLED_LIGHT_RED));
		addChild(createLightCentered<MediumLight<GreenLight>>(
			mm2px(Vec(15.286, 78.576)), module, SeventiesEQ::OUTLED_LIGHT_GREEN));
		addChild(createLightCentered<MediumLight<RedLight>>(
			mm2px(Vec(15.286, 78.576)), module, SeventiesEQ::OUTLED_LIGHT_RED));
	}
};

Model *modelSeventiesEQ = createModel<SeventiesEQ, SeventiesEQWidget>("SeventiesEQ");
