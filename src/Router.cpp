#include "plugin.hpp"

struct Router : Module {
	enum ParamId { SELECT_PARAM, SLEW_PARAM, INTERP_PARAM, PARAMS_LEN };
	enum InputId { SELECTCV_INPUT, INPUTS_LEN };
	enum OutputId {
		A1_OUTPUT,
		A2_OUTPUT,
		A3_OUTPUT,
		A4_OUTPUT,
		B1_OUTPUT,
		B2_OUTPUT,
		B3_OUTPUT,
		B4_OUTPUT,
		C1_OUTPUT,
		C2_OUTPUT,
		C3_OUTPUT,
		C4_OUTPUT,
		D1_OUTPUT,
		D2_OUTPUT,
		D3_OUTPUT,
		D4_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId { LIGHTS_LEN };

	Router() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(SELECTCV_INPUT, "Select CV");
		configParam(SELECT_PARAM, 0.f, 5.f, 0.f, "Routing");
		configParam(SLEW_PARAM, 0.f, 5000.f, 0.f, "Slew", "ms");
		configSwitch(INTERP_PARAM, 0.f, 1.f, 0.f, "Stepped/Morph", {"Stepped", "Morph"});

		for (int i = 0; i < OUTPUTS_LEN; i++) {
			configOutput(i, "Matrix Out " + std::to_string(i + 1));
		}
	}

	float routingValues[16] = {}; // Final output voltages
	float slewedSelect = 0.f;

	void process(const ProcessArgs &args) override {
		float selectParam = params[SELECT_PARAM].getValue();
		float selectCV = inputs[SELECTCV_INPUT].isConnected() ? inputs[SELECTCV_INPUT].getVoltage() * 0.5f : 0.f;

		float rawSelect = clamp(selectParam + selectCV, 0.f, 5.f);

		float slewControlMs = clamp(params[SLEW_PARAM].getValue(), 0.f, 5000.f);
		float slewTimeSec = slewControlMs / 1000.f;
		float alpha = (slewTimeSec <= 0.f) ? 1.f : clamp(args.sampleTime / slewTimeSec, 0.f, 1.f);

		slewedSelect += (rawSelect - slewedSelect) * alpha;

		bool morph = params[INTERP_PARAM].getValue() > 0.5f;

		int routeA = clamp((int)std::floor(slewedSelect), 0, 5);
		int routeB = clamp(routeA + 1, 0, 5);
		float t = morph ? clamp(slewedSelect - routeA, 0.f, 1.f) : 0.f;

		// Routing matrix: [route][input][output]
		static const int routingStates[6][4][4] = {
			{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}, // 0: A → B → C
			{{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}, {0, 1, 0, 0}}, // 1: A → C → B
			{{0, 1, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}, {0, 0, 0, 1}}, // 2: B → A → C
			{{0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}}, // 3: B → C → A
			{{0, 0, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {1, 0, 0, 0}}, // 4: C → A → B
			{{0, 0, 1, 0}, {0, 0, 0, 1}, {1, 0, 0, 0}, {0, 1, 0, 0}}  // 5: C → B → A
		};

		int index = 0;
		for (int input = 0; input < 4; ++input) {
			for (int output = 0; output < 4; ++output) {
				int valueA = routingStates[routeA][input][output];
				int valueB = routingStates[routeB][input][output];

				// Interpolate voltage
				float voltage = 5.f * ((1.f - t) * valueA + t * valueB);
				routingValues[index] = voltage;
				outputs[index].setVoltage(voltage);
				++index;
			}
		}
	}
};

struct RouterWidget : ModuleWidget {
	RouterWidget(Router *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Router.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(5.165, 13.794)), module, Router::SELECT_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(14.729, 13.794)), module, Router::SLEW_PARAM));
		addParam(createParamCentered<_2PosHorizontal>(mm2px(Vec(10.16, 21.202)), module, Router::INTERP_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 7.465)), module, Router::SELECTCV_INPUT));

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

Model *modelRouter = createModel<Router, RouterWidget>("Router");