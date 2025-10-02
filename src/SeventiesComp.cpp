#include "plugin.hpp"

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
	}

	void process(const ProcessArgs &args) override {
		float inL = inputs[AUDIO_L_INPUT].getVoltage();
		float inR = inputs[AUDIO_R_INPUT].isConnected() ? inputs[AUDIO_R_INPUT].getVoltage() : inL;

		const float peakReduction = params[PEAK_REDUCTION_PARAM].getValue();
		const float gainParam = params[GAIN_PARAM].getValue();
		const float dryWet = params[DRY_WET_PARAM].getValue();
		const bool isLimiter = params[RATIO_PARAM].getValue() > 0.5f;
		const bool bypass = params[BYPASS_PARAM].getValue() > 0.5f;

		if (bypass) {
			// Early exit for bypass
			outputs[AUDIO_L_OUTPUT].setVoltage(clamp(inL, -5.f, 5.f));
			outputs[AUDIO_R_OUTPUT].setVoltage(clamp(inR, -5.f, 5.f));
			lights[CLIPLED_LIGHT].setBrightnessSmooth((std::fabs(inL) >= 4.9f || std::fabs(inR) >= 4.9f) ? 1.f : 0.f,
													  args.sampleTime);
			return;
		}

		const float ratio = isLimiter ? 10.f : 3.f;
		const float inputMono = 0.5f * (inL + inR);

		// Envelope follower
		static float env = 0.f;
		const float rectified = std::fabs(inputMono);

		// Pre-compute smoothing coefficients once per call
		const float sampleRate = args.sampleRate;
		const float coeffAtk = std::exp(-1.f / (0.01f * sampleRate));
		const float coeffRelFast = std::exp(-1.f / (0.06f * sampleRate));
		const float coeffRelSlow = std::exp(-1.f / (1.5f * sampleRate));

		const float releaseCoeff = (env > 0.1f) ? coeffRelFast : coeffRelSlow;
		env = (rectified > env) ? coeffAtk * env + (1.f - coeffAtk) * rectified :
								  releaseCoeff * env + (1.f - releaseCoeff) * rectified;

		// Compression gain calculation
		const float threshold = 1.f - peakReduction;
		float gainReduction = 1.f;
		if (env > threshold) {
			const float over = env - threshold;
			gainReduction = 1.f / (1.f + over * (ratio - 1.f));
		}

		// Apply compression and gain
		const float gain = std::pow(10.f, gainParam * 2.f); // db = 0–40, divide by 20 inline
		float outL = inL * (1.f - dryWet) + inL * gainReduction * gain * dryWet;
		float outR = inR * (1.f - dryWet) + inR * gainReduction * gain * dryWet;

		outL = clamp(outL, -5.f, 5.f);
		outR = clamp(outR, -5.f, 5.f);
		outputs[AUDIO_L_OUTPUT].setVoltage(outL);
		outputs[AUDIO_R_OUTPUT].setVoltage(outR);

		const bool clipping = (std::fabs(outL) >= 4.9f || std::fabs(outR) >= 4.9f);
		lights[CLIPLED_LIGHT].setBrightnessSmooth(clipping ? 1.f : 0.f, args.sampleTime);
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
