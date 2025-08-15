#include "plugin.hpp"


struct Router : Module {
	enum ParamId {
		SELECT_PARAM,
		INTERP_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		A1_OUTPUT,
		C1_OUTPUT,
		A2_OUTPUT,
		C2_OUTPUT,
		A3_OUTPUT,
		C3_OUTPUT,
		A4_OUTPUT,
		C4_OUTPUT,
		B1_OUTPUT,
		D1_OUTPUT,
		B2_OUTPUT,
		D2_OUTPUT,
		B3_OUTPUT,
		D3_OUTPUT,
		B4_OUTPUT,
		D4_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Router() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(SELECT_PARAM, 0.f, 5.f, 1.f, "Routing", {"Crush,Dist,Verb","Crush,Verb,Dist","Dist,Crush,Verb","Dist,Verb,Crush","Verb,Crush,Dist","Verb,Dist,Crush"});
		configSwitch(INTERP_PARAM, 0.f, 1.f, 0.f, "Stepped/Morph", {"Stepped", "Morph"});
		configOutput(A1_OUTPUT, "A1");
		configOutput(C1_OUTPUT, "C1");
		configOutput(A2_OUTPUT, "A2");
		configOutput(C2_OUTPUT, "C2");
		configOutput(A3_OUTPUT, "A3");
		configOutput(C3_OUTPUT, "C3");
		configOutput(A4_OUTPUT, "A4");
		configOutput(C4_OUTPUT, "C4");
		configOutput(B1_OUTPUT, "B1");
		configOutput(D1_OUTPUT, "D1");
		configOutput(B2_OUTPUT, "B2");
		configOutput(D2_OUTPUT, "D2");
		configOutput(B3_OUTPUT, "B3");
		configOutput(D3_OUTPUT, "D3");
		configOutput(B4_OUTPUT, "B4");
		configOutput(D4_OUTPUT, "D4");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct RouterWidget : ModuleWidget {
	RouterWidget(Router* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Router.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(5.165, 18.027)), module, Router::SELECT_PARAM));
		addParam(createParamCentered<_2Pos>(mm2px(Vec(15.155, 18.027)), module, Router::INTERP_PARAM));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.165, 33.352)), module, Router::A1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.155, 33.352)), module, Router::C1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.165, 45.503)), module, Router::A2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.155, 45.503)), module, Router::C2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.165, 57.653)), module, Router::A3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.155, 57.653)), module, Router::C3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.165, 69.803)), module, Router::A4_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.155, 69.803)), module, Router::C4_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.165, 81.953)), module, Router::B1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.155, 81.953)), module, Router::D1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.165, 94.103)), module, Router::B2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.155, 94.103)), module, Router::D2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.165, 106.253)), module, Router::B3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.155, 106.253)), module, Router::D3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.165, 118.403)), module, Router::B4_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.155, 118.403)), module, Router::D4_OUTPUT));
	}
};


Model* modelRouter = createModel<Router, RouterWidget>("Router");