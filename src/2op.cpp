#include "plugin.hpp"

struct _2op : Module {
	enum ParamId {
		PITCH_PARAM,
		FMAMT_PARAM,
		RATIO_PARAM,
		DECAY_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCTIN_INPUT,
		FMAMTCVIN_INPUT,
		RATIOCVIN_INPUT,
		DECAYCVIN_INPUT,
		GATEIN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	_2op() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PITCH_PARAM, 0.f, 1.f, 0.f, "Pitch", "hz", 0.f, 100.f);
		configParam(FMAMT_PARAM, 0.f, 1.f, 0.f, "FM Amount", "%", 0.f, 100.f);
		configParam(RATIO_PARAM, 0.f, 1.f, 0.f, "Ratio", "%", 0.f, 100.f);
		configParam(DECAY_PARAM, 0.f, 1.f, 0.f, "Decay", "ms", 0.f, 100.f);
		configInput(VOCTIN_INPUT, "1v/Oct");
		configInput(FMAMTCVIN_INPUT, "FM Amount CV");
		configInput(RATIOCVIN_INPUT, "Ratio CV");
		configInput(DECAYCVIN_INPUT, "Deca CV");
		configInput(GATEIN_INPUT, "Gate");
		configOutput(AUDIO_OUTPUT, "Audio");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct _2opWidget : ModuleWidget {
	_2opWidget(_2op* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/2op.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.086, 18.629)), module, _2op::PITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.086, 41.44)), module, _2op::FMAMT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.086, 64.25)), module, _2op::RATIO_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.086, 87.06)), module, _2op::DECAY_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.394, 18.629)), module, _2op::VOCTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.394, 41.44)), module, _2op::FMAMTCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.394, 64.25)), module, _2op::RATIOCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.394, 87.06)), module, _2op::DECAYCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.394, 109.871)), module, _2op::GATEIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.086, 109.871)), module, _2op::AUDIO_OUTPUT));
	}
};


Model* model_2op = createModel<_2op, _2opWidget>("2op");