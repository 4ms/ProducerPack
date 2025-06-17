#include "plugin.hpp"


struct _70sComp : Module {
	enum ParamId {
		PEAK_REDUCTION_PARAM,
		RATIO_PARAM,
		GAIN_PARAM,
		BYPASS_PARAM,
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
        configSwitch(BYPASS_PARAM, 0.f, 1.f, 0.f, "Bypass", {"Off", "On"});
        configInput(AUDIO_L_INPUT, "Audio Left In");
        configInput(AUDIO_R_INPUT, "Audio Right In");
        configOutput(AUDIO_L_OUTPUT, "Audio Left Out");
        configOutput(AUDIO_R_OUTPUT, "Audio Right Out");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct _70sCompWidget : ModuleWidget {
	_70sCompWidget(_70sComp* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/70sComp.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 18.161)), module, _70sComp::PEAK_REDUCTION_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(10.16, 41.466)), module, _70sComp::RATIO_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 59.948)), module, _70sComp::GAIN_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(10.16, 78.016)), module, _70sComp::BYPASS_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(4.513, 98.845)), module, _70sComp::AUDIO_L_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(4.513, 115.624)), module, _70sComp::AUDIO_R_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.662, 98.845)), module, _70sComp::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.662, 115.624)), module, _70sComp::AUDIO_R_OUTPUT));
	}
};


Model* model_70sComp = createModel<_70sComp, _70sCompWidget>("70sComp");