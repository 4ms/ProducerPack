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
		configParam(SELECT_PARAM, 0.f, 1.f, 0.f, "");
		configParam(INTERP_PARAM, 0.f, 1.f, 0.f, "");
		configOutput(A1_OUTPUT, "");
		configOutput(C1_OUTPUT, "");
		configOutput(A2_OUTPUT, "");
		configOutput(C2_OUTPUT, "");
		configOutput(A3_OUTPUT, "");
		configOutput(C3_OUTPUT, "");
		configOutput(A4_OUTPUT, "");
		configOutput(C4_OUTPUT, "");
		configOutput(B1_OUTPUT, "");
		configOutput(D1_OUTPUT, "");
		configOutput(B2_OUTPUT, "");
		configOutput(D2_OUTPUT, "");
		configOutput(B3_OUTPUT, "");
		configOutput(D3_OUTPUT, "");
		configOutput(B4_OUTPUT, "");
		configOutput(D4_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct RouterWidget : ModuleWidget {
	RouterWidget(Router* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Router.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(5.165, 18.027)), module, Router::SELECT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.155, 18.027)), module, Router::INTERP_PARAM));

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