#include "plugin.hpp"


struct DK : Module {
	enum ParamId {
		DECAY_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		DECAYCVIN_INPUT,
		TRIGIN_INPUT,
		AUDIOIN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		DECAYOUT_OUTPUT,
		AUDIOOUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LED_LIGHT,
		LIGHTS_LEN
	};

	DK() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(DECAY_PARAM, 0.f, 1.f, 0.f, "");
		configInput(DECAYCVIN_INPUT, "");
		configInput(TRIGIN_INPUT, "");
		configInput(AUDIOIN_INPUT, "");
		configOutput(DECAYOUT_OUTPUT, "");
		configOutput(AUDIOOUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct DKWidget : ModuleWidget {
	DKWidget(DK* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/DK.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.426, 23.569)), module, DK::DECAY_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.426, 44.255)), module, DK::DECAYCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(4.891, 82.795)), module, DK::TRIGIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(4.891, 100.173)), module, DK::AUDIOIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.191, 82.795)), module, DK::DECAYOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.191, 100.173)), module, DK::AUDIOOUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.426, 60.653)), module, DK::LED_LIGHT));
	}
};


Model* modelDK = createModel<DK, DKWidget>("DK");