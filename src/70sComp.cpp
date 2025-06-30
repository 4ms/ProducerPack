#include "plugin.hpp"


struct _70sComp : Module {
	enum ParamId {
		PEAK_REDUCTION_PARAM,
		RATIO_PARAM,
		GAIN_PARAM,
		BYPASS_PARAM,
		DRY_WET_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		AUDIO_L_INPUT,
		AUDIO_R_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_L_OUTPUT,
		AUDIO_R_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	_70sComp() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(PEAK_REDUCTION_PARAM, 0.f, 1.f, 0.f, "Peak Reduction", "%", 0.f, 100.f);
        configSwitch(RATIO_PARAM, 0.f, 1.f, 0.f, "Comp/Limit", {"Compressor", "Limiter"}); // 3:1 or 10:1
        configParam(GAIN_PARAM, 0.f, 1.f, 0.f, "Gain", "db", 0.f, 40.f);
		configParam(DRY_WET_PARAM, 0.f, 1.f, 0.f, "Dry/Wet", "%", 0.f, 100.f);
        configSwitch(BYPASS_PARAM, 0.f, 1.f, 0.f, "Bypass", {"Off", "On"});
        configInput(AUDIO_L_INPUT, "Audio Left In");
        configInput(AUDIO_R_INPUT, "Audio Right In");
        configOutput(AUDIO_L_OUTPUT, "Audio Left Out");
        configOutput(AUDIO_R_OUTPUT, "Audio Right Out");
	}

	void process(const ProcessArgs& args) override {
		float inL = inputs[AUDIO_L_INPUT].getVoltage();
		float inR = inputs[AUDIO_R_INPUT].isConnected() ? inputs[AUDIO_R_INPUT].getVoltage() : inL;
	
		float peakReduction = params[PEAK_REDUCTION_PARAM].getValue();
		float gainParam = params[GAIN_PARAM].getValue();
		float dryWet = params[DRY_WET_PARAM].getValue();
		bool isLimiter = params[RATIO_PARAM].getValue() > 0.5f;
		bool bypass = params[BYPASS_PARAM].getValue() > 0.5f;
	
		float ratio = isLimiter ? 10.f : 3.f;
	
		float inputMono = 0.5f * (inL + inR);
	
		static float env = 0.f;
		float rectified = std::fabs(inputMono);
	
		float sampleRate = args.sampleRate;
		const float attackTime = 0.01f;
		const float releaseFast = 0.06f;
		const float releaseSlow = 1.5f;
	
		float coeffAtk = std::exp(-1.f / (attackTime * sampleRate));
		float coeffRelFast = std::exp(-1.f / (releaseFast * sampleRate));
		float coeffRelSlow = std::exp(-1.f / (releaseSlow * sampleRate));
	
		if (rectified > env) {
			env = coeffAtk * env + (1.f - coeffAtk) * rectified;
		} else {
			float relCoeff = (env > 0.1f) ? coeffRelFast : coeffRelSlow;
			env = relCoeff * env + (1.f - relCoeff) * rectified;
		}
	
		float threshold = 1.f - peakReduction;
		float gainReduction = 1.f;
		if (env > threshold) {
			float over = env - threshold;
			gainReduction = 1.f / (1.f + over * (ratio - 1.f));
		}
	
		float compressedL = inL * gainReduction;
		float compressedR = inR * gainReduction;
	
		float gainDb = gainParam * 40.f;
		float gain = std::pow(10.f, gainDb / 20.f);
		compressedL *= gain;
		compressedR *= gain;
	
		float outL = inL * (1.f - dryWet) + compressedL * dryWet;
		float outR = inR * (1.f - dryWet) + compressedR * dryWet;
	
		if (bypass) {
			outL = inL;
			outR = inR;
		}
	
		outL = clamp(outL, -5.f, 5.f);
		outR = clamp(outR, -5.f, 5.f);
	
		outputs[AUDIO_L_OUTPUT].setVoltage(outL);
		outputs[AUDIO_R_OUTPUT].setVoltage(outR);
	}
};	


struct _70sCompWidget : ModuleWidget {
	_70sCompWidget(_70sComp* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/70sComp.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 18.161)), module, _70sComp::PEAK_REDUCTION_PARAM));
		
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 51.009)), module, _70sComp::GAIN_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 66.209)), module, _70sComp::DRY_WET_PARAM));

		addParam(createParamCentered<CKSS>(mm2px(Vec(10.16, 83.21)), module, _70sComp::BYPASS_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(10.16, 36.562)), module, _70sComp::RATIO_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(4.513, 98.845)), module, _70sComp::AUDIO_L_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(4.513, 115.624)), module, _70sComp::AUDIO_R_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.662, 98.845)), module, _70sComp::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.662, 115.624)), module, _70sComp::AUDIO_R_OUTPUT));
	}
};

Model* model_70sComp = createModel<_70sComp, _70sCompWidget>("70sComp");