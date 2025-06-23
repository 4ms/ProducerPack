#include "plugin.hpp"


struct Boost : Module {
	enum ParamId {
		GAIN_PARAM,
		RANGE_PARAM,
		VOLUME_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		LEFTIN_INPUT,
		RIGHTIN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		LEFTOUT_OUTPUT,
		RIGHTOUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LEFTLED_LIGHT,
		RIGHTLED_LIGHT,
		LIGHTS_LEN
	};

	Boost() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(GAIN_PARAM, 0.f, 1.f, 0.f, "Gain", "%", 0.f, 100.f);
		configSwitch(RANGE_PARAM, 0.f, 2.f, 0.f, "Gain Range", {"1x", "5x", "100x"});
		configParam(VOLUME_PARAM, 0.f, 1.f, 0.f, "Volume", "%", 0.f, 100.f);
		configInput(LEFTIN_INPUT, "Audio Left");
		configInput(RIGHTIN_INPUT, "Audio Right");
		configOutput(LEFTOUT_OUTPUT, "Audio Left");
		configOutput(RIGHTOUT_OUTPUT, "Audio Right");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct BoostWidget : ModuleWidget {
	BoostWidget(Boost* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Boost.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 20.41)), module, Boost::GAIN_PARAM));
		addParam(createParamCentered<CKSSThreeHorizontal>(mm2px(Vec(10.16, 44.576)), module, Boost::RANGE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 65.889)), module, Boost::VOLUME_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(4.273, 95.881)), module, Boost::LEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(4.273, 109.61)), module, Boost::RIGHTIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.402, 95.881)), module, Boost::LEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.402, 109.61)), module, Boost::RIGHTOUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(4.273, 89.152)), module, Boost::LEFTLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(15.402, 89.152)), module, Boost::RIGHTLED_LIGHT));
	}
};


Model* modelBoost = createModel<Boost, BoostWidget>("Boost");