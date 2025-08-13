#include "plugin.hpp"


struct SoloPush : Module {
	enum ParamId {
		CH1RANGE_PARAM,
		CH1OFFSET_PARAM,
		CH1BEHAVIOR_PARAM,
		CH1PUSH_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		CH1VOLTAGEOUT_OUTPUT,
		CH1BUTTONOUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	SoloPush() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(CH1RANGE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CH1OFFSET_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CH1BEHAVIOR_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CH1PUSH_PARAM, 0.f, 1.f, 0.f, "");
		configOutput(CH1VOLTAGEOUT_OUTPUT, "");
		configOutput(CH1BUTTONOUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct SoloPushWidget : ModuleWidget {
	SoloPushWidget(SoloPush* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/SoloPush_info.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 55.629)), module, SoloPush::CH1RANGE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 74.637)), module, SoloPush::CH1OFFSET_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 93.644)), module, SoloPush::CH1BEHAVIOR_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 112.652)), module, SoloPush::CH1PUSH_PARAM));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 17.614)), module, SoloPush::CH1VOLTAGEOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 36.622)), module, SoloPush::CH1BUTTONOUT_OUTPUT));
	}
};


Model* modelSoloPush = createModel<SoloPush, SoloPushWidget>("SoloPush");