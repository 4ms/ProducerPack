#include "plugin.hpp"


struct AuxSends : Module {
	enum ParamId {
		ASEND_PARAM,
		BSEND_PARAM,
		CSEND_PARAM,
		ARETURN_PARAM,
		BRETURN_PARAM,
		CRETURN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		ASENDCVIN_INPUT,
		BSENDCVIN_INPUT,
		CSENDCVIN_INPUT,
		ARETURNCVIN_INPUT,
		BRETURNCVIN_INPUT,
		CRETURNCVIN_INPUT,
		ARETURNLEFTIN_INPUT,
		ARETURNRIGHTIN_INPUT,
		BRETURNLEFTIN_INPUT,
		BRETURNRIGHTIN_INPUT,
		CRETURNLEFTIN_INPUT,
		CRETURNRIGHTIN_INPUT,
		AUDIOLEFTIN_INPUT,
		AUDIORIGHTIN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ASENDLEFTOUT_OUTPUT,
		ASENDRIGHTOUT_OUTPUT,
		BSENDLEFTOUT_OUTPUT,
		BSENDRIGHTOUT_OUTPUT,
		CSENDLEFTOUT_OUTPUT,
		CSENDRIGHTOUT_OUTPUT,
		AUDIOLEFTOUT_OUTPUT,
		AUDIORIGHTOUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ASENDLED_LIGHT,
		BSENDLED_LIGHT,
		CSENDLED_LIGHT,
		ARETURNLED_LIGHT,
		BRETURNLED_LIGHT,
		CRETURNLED_LIGHT,
		LIGHTS_LEN
	};

	AuxSends() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(ASEND_PARAM, 0.f, 1.f, 0.f, "");
		configParam(BSEND_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CSEND_PARAM, 0.f, 1.f, 0.f, "");
		configParam(ARETURN_PARAM, 0.f, 1.f, 0.f, "");
		configParam(BRETURN_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CRETURN_PARAM, 0.f, 1.f, 0.f, "");
		configInput(ASENDCVIN_INPUT, "");
		configInput(BSENDCVIN_INPUT, "");
		configInput(CSENDCVIN_INPUT, "");
		configInput(ARETURNCVIN_INPUT, "");
		configInput(BRETURNCVIN_INPUT, "");
		configInput(CRETURNCVIN_INPUT, "");
		configInput(ARETURNLEFTIN_INPUT, "");
		configInput(ARETURNRIGHTIN_INPUT, "");
		configInput(BRETURNLEFTIN_INPUT, "");
		configInput(BRETURNRIGHTIN_INPUT, "");
		configInput(CRETURNLEFTIN_INPUT, "");
		configInput(CRETURNRIGHTIN_INPUT, "");
		configInput(AUDIOLEFTIN_INPUT, "");
		configInput(AUDIORIGHTIN_INPUT, "");
		configOutput(ASENDLEFTOUT_OUTPUT, "");
		configOutput(ASENDRIGHTOUT_OUTPUT, "");
		configOutput(BSENDLEFTOUT_OUTPUT, "");
		configOutput(BSENDRIGHTOUT_OUTPUT, "");
		configOutput(CSENDLEFTOUT_OUTPUT, "");
		configOutput(CSENDRIGHTOUT_OUTPUT, "");
		configOutput(AUDIOLEFTOUT_OUTPUT, "");
		configOutput(AUDIORIGHTOUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct AuxSendsWidget : ModuleWidget {
	AuxSendsWidget(AuxSends* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/AuxSends.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.665, 24.452)), module, AuxSends::ASEND_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.184, 24.452)), module, AuxSends::BSEND_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(47.999, 24.452)), module, AuxSends::CSEND_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(10.375, 67.087)), module, AuxSends::ARETURN_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(30.184, 67.393)), module, AuxSends::BRETURN_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(47.999, 67.743)), module, AuxSends::CRETURN_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.665, 32.96)), module, AuxSends::ASENDCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.184, 32.96)), module, AuxSends::BSENDCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.999, 32.96)), module, AuxSends::CSENDCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.375, 75.519)), module, AuxSends::ARETURNCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.184, 75.519)), module, AuxSends::BRETURNCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.999, 75.519)), module, AuxSends::CRETURNCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.73, 88.118)), module, AuxSends::ARETURNLEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.443, 88.118)), module, AuxSends::ARETURNRIGHTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(26.304, 88.118)), module, AuxSends::BRETURNLEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35.017, 88.118)), module, AuxSends::BRETURNRIGHTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(44.162, 88.118)), module, AuxSends::CRETURNLEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(52.874, 88.118)), module, AuxSends::CRETURNRIGHTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.589, 116.408)), module, AuxSends::AUDIOLEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(18.876, 116.408)), module, AuxSends::AUDIORIGHTIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(6.379, 45.387)), module, AuxSends::ASENDLEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.092, 45.387)), module, AuxSends::ASENDRIGHTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.953, 45.387)), module, AuxSends::BSENDLEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(34.666, 45.387)), module, AuxSends::BSENDRIGHTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(43.811, 45.387)), module, AuxSends::CSENDLEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(52.524, 45.387)), module, AuxSends::CSENDRIGHTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(39.574, 116.408)), module, AuxSends::AUDIOLEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50.983, 116.408)), module, AuxSends::AUDIORIGHTOUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.665, 17.587)), module, AuxSends::ASENDLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(30.184, 17.587)), module, AuxSends::BSENDLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(47.999, 17.587)), module, AuxSends::CSENDLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.375, 59.904)), module, AuxSends::ARETURNLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(30.184, 60.21)), module, AuxSends::BRETURNLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(47.999, 60.56)), module, AuxSends::CRETURNLED_LIGHT));
	}
};


Model* modelAuxSends = createModel<AuxSends, AuxSendsWidget>("AuxSends");