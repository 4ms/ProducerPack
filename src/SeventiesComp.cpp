#include "plugin.hpp"
#include "helpers/math_lut.hpp"

struct SeventiesComp : Module {
	enum ParamId { PEAK_REDUCTION_PARAM, RATIO_PARAM, GAIN_PARAM, BYPASS_PARAM, DRY_WET_PARAM, PARAMS_LEN };
	enum InputId { AUDIO_L_INPUT, AUDIO_R_INPUT, INPUTS_LEN };
	enum OutputId { AUDIO_L_OUTPUT, AUDIO_R_OUTPUT, OUTPUTS_LEN };
	enum LightId { CLIPLED_LIGHT, LIGHTS_LEN };

	SeventiesComp() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PEAK_REDUCTION_PARAM, 0.f, 1.f, 0.5f, "Peak Reduction", "%", 0.f, 100.f);
		configSwitch(RATIO_PARAM, 0.f, 1.f, 0.f, "Comp/Limit", {"Compressor", "Limiter"}); // 3:1 or 10:1
		configParam(GAIN_PARAM, 0.f, 1.f, 0.25f, "Gain", "db", 0.f, 40.f);
		configParam(DRY_WET_PARAM, 0.f, 1.f, 1.f, "Dry/Wet", "%", 0.f, 100.f);
		configSwitch(BYPASS_PARAM, 0.f, 1.f, 0.f, "Bypass", {"Off", "On"});
		configInput(AUDIO_L_INPUT, "Audio Left");
		configInput(AUDIO_R_INPUT, "Audio Right");
		configOutput(AUDIO_L_OUTPUT, "Audio Left");
		configOutput(AUDIO_R_OUTPUT, "Audio Right");

		configBypass(AUDIO_L_INPUT, AUDIO_L_OUTPUT);
		configBypass(AUDIO_R_INPUT, AUDIO_R_OUTPUT);
	}

	struct Pow10TableRange {
		static constexpr float min = 0.f;
		static constexpr float max = 2.f;
	};

	static inline const auto Pow10 =
		Mapping::LookupTable_t<64, float>::generate<Pow10TableRange>([](float x) { return std::pow(10.f, x); });

	void process(const ProcessArgs &args) override {
		const bool rConnected = inputs[AUDIO_R_INPUT].isConnected();
		const int nL = inputs[AUDIO_L_INPUT].getChannels();
		const int nR = rConnected ? inputs[AUDIO_R_INPUT].getChannels() : nL;
		const int n = std::max(nL, nR);

		outputs[AUDIO_L_OUTPUT].setChannels(n);
		outputs[AUDIO_R_OUTPUT].setChannels(n);

		const bool bypass = params[BYPASS_PARAM].getValue() > 0.5f;
		if (bypass) {
			for (int c = 0; c < n; c++) {
				float il = inputs[AUDIO_L_INPUT].getPolyVoltage(c);
				float ir = rConnected ? inputs[AUDIO_R_INPUT].getPolyVoltage(c) : il;
				outputs[AUDIO_L_OUTPUT].setVoltage(il, c);
				outputs[AUDIO_R_OUTPUT].setVoltage(ir, c);
			}
			lights[CLIPLED_LIGHT].setBrightness(0.f);
			return;
		}

		// --- Update cached parameter-derived variables only when changed ---
		if (prevPeakReduction != params[PEAK_REDUCTION_PARAM].getValue()) {
			prevPeakReduction = params[PEAK_REDUCTION_PARAM].getValue();
			threshold = 1.f - prevPeakReduction;
		}

		if (prevGain != params[GAIN_PARAM].getValue()) {
			prevGain = params[GAIN_PARAM].getValue();
			gain = Pow10(prevGain * 2.f);
		}

		if (prevDryWet != params[DRY_WET_PARAM].getValue()) {
			prevDryWet = params[DRY_WET_PARAM].getValue();
			dryWet = prevDryWet;
		}

		if (prevRatioParam != params[RATIO_PARAM].getValue()) {
			prevRatioParam = params[RATIO_PARAM].getValue();
			isLimiter = prevRatioParam > 0.5f;
			ratio = isLimiter ? 10.f : 3.f;
		}

		const float coeffAtk = attackCoeff(args.sampleRate);
		const float coeffRelFast = releaseFastCoeff(args.sampleRate);
		const float coeffRelSlow = releaseSlowCoeff(args.sampleRate);

		bool anyClipping = false;

		for (int c = 0; c < n; c++) {
			float inL = inputs[AUDIO_L_INPUT].getPolyVoltage(c);
			float inR = rConnected ? inputs[AUDIO_R_INPUT].getPolyVoltage(c) : inL;

			// --- Envelope follower (per channel pair, sidechain is stereo average) ---
			const float rectified = std::fabs(0.5f * (inL + inR));
			const float releaseCoeff = (env[c] > 0.1f) ? coeffRelFast : coeffRelSlow;
			env[c] = (rectified > env[c]) ? coeffAtk * env[c] + (1.f - coeffAtk) * rectified
										  : releaseCoeff * env[c] + (1.f - releaseCoeff) * rectified;

			// --- Gain reduction ---
			float gainReduction = 1.f;
			if (env[c] > threshold) {
				const float over = env[c] - threshold;
				gainReduction = 1.f / (1.f + over * (ratio - 1.f));
			}

			// --- Apply gain & dry/wet ---
			float outL = inL * (1.f - dryWet) + inL * gainReduction * gain * dryWet;
			float outR = inR * (1.f - dryWet) + inR * gainReduction * gain * dryWet;

			outL = std::clamp(outL, -10.f, 10.f);
			outR = std::clamp(outR, -10.f, 10.f);

			outputs[AUDIO_L_OUTPUT].setVoltage(outL, c);
			outputs[AUDIO_R_OUTPUT].setVoltage(outR, c);

			if (std::fabs(outL) >= 9.9f || std::fabs(outR) >= 9.9f)
				anyClipping = true;
		}

		lights[CLIPLED_LIGHT].setBrightnessSmooth(anyClipping ? 1.f : 0.f, args.sampleTime);
	}

private:
	// --- Member variables for caching ---
	float env[PORT_MAX_CHANNELS] = {};

	float prevPeakReduction = 0.f;
	float prevGain = 0.f;
	float prevDryWet = 0.f;
	float prevRatioParam = 0.f;

	float threshold = 0.f;
	float gain = 0.f;
	float dryWet = 0.f;
	bool isLimiter = false;
	float ratio = 3.f;

	// Precompute attack/release coefficients per sampleRate
	float attackCoeff(float sampleRate) {
		return std::exp(-1.f / (0.01f * sampleRate));
	}
	float releaseFastCoeff(float sampleRate) {
		return std::exp(-1.f / (0.06f * sampleRate));
	}
	float releaseSlowCoeff(float sampleRate) {
		return std::exp(-1.f / (1.5f * sampleRate));
	}
};

struct SeventiesCompWidget : ModuleWidget {
	SeventiesCompWidget(SeventiesComp *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/SeventiesComp_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hBlack>(
			mm2px(Vec(15.262, 18.804)), module, SeventiesComp::PEAK_REDUCTION_PARAM));
		addParam(createParam<Switch2Pos>(mm2px(Vec(5.25, 35.77)), module, SeventiesComp::RATIO_PARAM));
		addParam(createParam<Switch2Pos>(mm2px(Vec(19.268, 35.77)), module, SeventiesComp::BYPASS_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(15.262, 56.014)), module, SeventiesComp::GAIN_PARAM));
		addParam(
			createParamCentered<Davies1900hBlack>(mm2px(Vec(15.262, 78.018)), module, SeventiesComp::DRY_WET_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.309, 97.023)), module, SeventiesComp::AUDIO_L_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.034, 97.023)), module, SeventiesComp::AUDIO_R_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.309, 111.029)), module, SeventiesComp::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.034, 111.029)), module, SeventiesComp::AUDIO_R_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(
			mm2px(Vec(25.772, 50.442)), module, SeventiesComp::CLIPLED_LIGHT));
	}
};
Model *modelSeventiesComp = createModel<SeventiesComp, SeventiesCompWidget>("SeventiesComp");
