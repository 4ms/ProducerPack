#include "plugin.hpp"


struct KayOne : Module {
	enum ParamId {
		SPEED_PARAM,
		LENGTH_PARAM,
		LOOP_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SPEEDCVIN_INPUT,
		LENGTHCVIN_INPUT,
		LOOPCVIN_INPUT,
		KICKTRIGIN_INPUT,
		SNARETRIGIN_INPUT,
		TOMLTRIG_INPUT,
		TOMHTRIG_INPUT,
		CLTRIG_INPUT,
		OHTRIG_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		KICKOUT_OUTPUT,
		SNAREOUT_OUTPUT,
		TOMLOUT_OUTPUT,
		TOMHOUT_OUTPUT,
		CLOUT_OUTPUT,
		OHOUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	KayOne() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(SPEED_PARAM, 0.f, 1.f, 0.5f, "Speed", "%", 0.f, 100.f);
		configParam(LENGTH_PARAM, 0.f, 1.f, 1.f, "Length", "%", 0.f, 100.f);
		configSwitch(LOOP_PARAM, 0.f, 1.f, 0.f, "Loop", {"Off", "On"});
		configInput(SPEEDCVIN_INPUT, "Speed CV");
		configInput(LENGTHCVIN_INPUT, "Length CV");
		configInput(LOOPCVIN_INPUT, "Loop Gate");
		configInput(KICKTRIGIN_INPUT, "Kick Trig");
		configInput(SNARETRIGIN_INPUT, "Snare Trig");
		configInput(TOMLTRIG_INPUT, "Tom Lo Trig");
		configInput(TOMHTRIG_INPUT, "Tom Hi Trig");
		configInput(CLTRIG_INPUT, "Closed Hat Trig");
		configInput(OHTRIG_INPUT, "Open Hat Trig");
		configOutput(KICKOUT_OUTPUT, "Kick");
		configOutput(SNAREOUT_OUTPUT, "Snare");
		configOutput(TOMLOUT_OUTPUT, "Tom Lo");
		configOutput(TOMHOUT_OUTPUT, "Tom Hi");
		configOutput(CLOUT_OUTPUT, "Closed Hat");
		configOutput(OHOUT_OUTPUT, "Open Hat");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct KayOneWidget : ModuleWidget {
	KayOneWidget(KayOne* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/KayOne_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(15.24, 15.958)), module, KayOne::SPEED_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(6.221, 35.67)), module, KayOne::LENGTH_PARAM));
		addParam(createParamCentered<_2Pos>(mm2px(Vec(23.632, 35.67)), module, KayOne::LOOP_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 29.153)), module, KayOne::SPEEDCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.221, 49.49)), module, KayOne::LENGTHCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.632, 49.49)), module, KayOne::LOOPCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.221, 63.771)), module, KayOne::KICKTRIGIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.221, 73.417)), module, KayOne::SNARETRIGIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.221, 83.302)), module, KayOne::TOMLTRIG_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.221, 93.19)), module, KayOne::TOMHTRIG_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.221, 103.551)), module, KayOne::CLTRIG_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.221, 114.064)), module, KayOne::OHTRIG_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.632, 63.771)), module, KayOne::KICKOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.632, 73.417)), module, KayOne::SNAREOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.632, 83.302)), module, KayOne::TOMLOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.632, 93.19)), module, KayOne::TOMHOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.632, 103.551)), module, KayOne::CLOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.632, 114.064)), module, KayOne::OHOUT_OUTPUT));
	}
};


Model* modelKayOne = createModel<KayOne, KayOneWidget>("KayOne");