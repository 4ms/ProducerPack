#include "plugin.hpp"

struct DrumBus : Module {
	enum ParamId {
		CH1VOL_PARAM,
		CH1PAN_PARAM,
		CH1MUTE_PARAM,
		CH2VOL_PARAM,
		CH2PAN_PARAM,
		CH2MUTE_PARAM,
		CH3VOL_PARAM,
		CH3PAN_PARAM,
		CH3MUTE_PARAM,
		CH4VOL_PARAM,
		CH4PAN_PARAM,
		CH4MUTE_PARAM,
		CH5VOL_PARAM,
		CH5PAN_PARAM,
		CH5MUTE_PARAM,
		CH6VOL_PARAM,
		CH6PAN_PARAM,
		CH6MUTE_PARAM,
		CH7VOL_PARAM,
		CH7PAN_PARAM,
		CH7MUTE_PARAM,
		CH8VOL_PARAM,
		CH8PAN_PARAM,
		CH8MUTE_PARAM,
		MASTERVOL_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CH1IN_INPUT,
		CH2IN_INPUT,
		CH3IN_INPUT,
		CH4IN_INPUT,
		CH5IN_INPUT,
		CH6IN_INPUT,
		CH7IN_INPUT,
		CH8IN_INPUT,
		INPUTS_LEN
	};
	enum OutputId { AUDIOLEFTOUT_OUTPUT, AUDIORIGHTOUT_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	DrumBus() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(CH1VOL_PARAM, 0.f, 1.f, 0.5f, "Ch. 1 Volume", "%", 0.f, 100.f);
		configParam(CH1PAN_PARAM, -50.f, 50.f, 0.f, "Ch. 1 Pan", "%");
		configSwitch(CH1MUTE_PARAM, 0.f, 1.f, 0.f, "Ch. 1 Mute", {"Off", "On"});

		configParam(CH2VOL_PARAM, 0.f, 1.f, 0.5f, "Ch. 2 Volume", "%", 0.f, 100.f);
		configParam(CH2PAN_PARAM, -50.f, 50.f, 0.f, "Ch. 2 Pan", "%");
		configSwitch(CH2MUTE_PARAM, 0.f, 1.f, 0.f, "Ch. 2 Mute", {"Off", "On"});

		configParam(CH3VOL_PARAM, 0.f, 1.f, 0.5f, "Ch. 3 Volume", "%", 0.f, 100.f);
		configParam(CH3PAN_PARAM, -50.f, 50.f, 0.f, "Ch. 3 Pan", "%");
		configSwitch(CH3MUTE_PARAM, 0.f, 1.f, 0.f, "Ch. 3 Mute", {"Off", "On"});

		configParam(CH4VOL_PARAM, 0.f, 1.f, 0.5f, "Ch. 4 Volume", "%", 0.f, 100.f);
		configParam(CH4PAN_PARAM, -50.f, 50.f, 0.f, "Ch. 4 Pan", "%");
		configSwitch(CH4MUTE_PARAM, 0.f, 1.f, 0.f, "Ch. 4 Mute", {"Off", "On"});

		configParam(CH5VOL_PARAM, 0.f, 1.f, 0.5f, "Ch. 5 Volume", "%", 0.f, 100.f);
		configParam(CH5PAN_PARAM, -50.f, 50.f, 0.f, "Ch. 5 Pan", "%");
		configSwitch(CH5MUTE_PARAM, 0.f, 1.f, 0.f, "Ch. 5 Mute", {"Off", "On"});

		configParam(CH6VOL_PARAM, 0.f, 1.f, 0.5f, "Ch. 6 Volume", "%", 0.f, 100.f);
		configParam(CH6PAN_PARAM, -50.f, 50.f, 0.f, "Ch. 6 Pan", "%");
		configSwitch(CH6MUTE_PARAM, 0.f, 1.f, 0.f, "Ch. 6 Mute", {"Off", "On"});

		configParam(CH7VOL_PARAM, 0.f, 1.f, 0.5f, "Ch. 7 Volume", "%", 0.f, 100.f);
		configParam(CH7PAN_PARAM, -50.f, 50.f, 0.f, "Ch. 7 Pan", "%");
		configSwitch(CH7MUTE_PARAM, 0.f, 1.f, 0.f, "Ch. 7 Mute", {"Off", "On"});

		configParam(CH8VOL_PARAM, 0.f, 1.f, 0.5f, "Ch. 8 Volume", "%", 0.f, 100.f);
		configParam(CH8PAN_PARAM, -50.f, 50.f, 0.f, "Ch. 8 Pan", "%");
		configSwitch(CH8MUTE_PARAM, 0.f, 1.f, 0.f, "Ch. 8 Mute", {"Off", "On"});

		configParam(MASTERVOL_PARAM, 0.f, 1.f, 1.f, "Master Volume", "%", 0.f, 100.f);
		configInput(CH1IN_INPUT, "Ch. 1");
		configInput(CH2IN_INPUT, "Ch. 2");
		configInput(CH3IN_INPUT, "Ch. 3");
		configInput(CH4IN_INPUT, "Ch. 4");
		configInput(CH5IN_INPUT, "Ch. 5");
		configInput(CH6IN_INPUT, "Ch. 6");
		configInput(CH7IN_INPUT, "Ch. 7");
		configInput(CH8IN_INPUT, "Ch. 8");

		configOutput(AUDIOLEFTOUT_OUTPUT, "Mix Left");
		configOutput(AUDIORIGHTOUT_OUTPUT, "Mix Right");
	}

	void process(const ProcessArgs &args) override {
		float leftMix = 0.f;
		float rightMix = 0.f;

		const float masterVol = params[MASTERVOL_PARAM].getValue();
		if (masterVol <= 0.f) {
			outputs[AUDIOLEFTOUT_OUTPUT].setVoltage(0.f);
			outputs[AUDIORIGHTOUT_OUTPUT].setVoltage(0.f);
			return;
		}

		for (int ch = 0; ch < 8; ++ch) {
			const int baseParam = CH1VOL_PARAM + ch * 3;
			const int inputId = CH1IN_INPUT + ch;

			if (!inputs[inputId].isConnected())
				continue;

			const float vol = params[baseParam].getValue();
			if (vol <= 0.f)
				continue;

			if (params[baseParam + 2].getValue() > 0.5f)
				continue; // muted

			const float in = inputs[inputId].getVoltage();
			const float panNorm = params[baseParam + 1].getValue() * 0.01f + 0.5f; // -50 to 50 -> 0 to 1

			// Precompute pan law (constant power)
			float panAngle = panNorm * (float)M_PI_2;
			float leftGain = cosf(panAngle);
			float rightGain = sinf(panAngle);

			const float scaledIn = in * vol;
			leftMix += scaledIn * leftGain;
			rightMix += scaledIn * rightGain;
		}

		// Apply master volume
		leftMix *= masterVol;
		rightMix *= masterVol;

		outputs[AUDIOLEFTOUT_OUTPUT].setVoltage(clamp(leftMix, -10.f, 10.f));
		outputs[AUDIORIGHTOUT_OUTPUT].setVoltage(clamp(rightMix, -10.f, 10.f));
	}
};

struct DrumBusWidget : ModuleWidget {
	DrumBusWidget(DrumBus *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/DrumBus_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Knob9mm>(mm2px(Vec(27.651, 13.501)), module, DrumBus::CH1VOL_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(45.649, 13.501)), module, DrumBus::CH1PAN_PARAM));
		addParam(createParamCentered<Switch2Pos>(mm2px(Vec(63.549, 13.499)), module, DrumBus::CH1MUTE_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(27.651, 25.199)), module, DrumBus::CH2VOL_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(45.649, 25.199)), module, DrumBus::CH2PAN_PARAM));
		addParam(createParamCentered<Switch2Pos>(mm2px(Vec(63.549, 25.201)), module, DrumBus::CH2MUTE_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(27.651, 36.901)), module, DrumBus::CH3VOL_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(45.649, 36.901)), module, DrumBus::CH3PAN_PARAM));
		addParam(createParamCentered<Switch2Pos>(mm2px(Vec(63.549, 36.899)), module, DrumBus::CH3MUTE_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(27.651, 48.599)), module, DrumBus::CH4VOL_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(45.649, 48.599)), module, DrumBus::CH4PAN_PARAM));
		addParam(createParamCentered<Switch2Pos>(mm2px(Vec(63.549, 48.601)), module, DrumBus::CH4MUTE_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(27.651, 60.399)), module, DrumBus::CH5VOL_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(45.649, 60.399)), module, DrumBus::CH5PAN_PARAM));
		addParam(createParamCentered<Switch2Pos>(mm2px(Vec(63.549, 60.401)), module, DrumBus::CH5MUTE_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(27.651, 71.999)), module, DrumBus::CH6VOL_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(45.649, 72.073)), module, DrumBus::CH6PAN_PARAM));
		addParam(createParamCentered<Switch2Pos>(mm2px(Vec(63.549, 72.001)), module, DrumBus::CH6MUTE_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(27.651, 83.799)), module, DrumBus::CH7VOL_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(45.649, 83.799)), module, DrumBus::CH7PAN_PARAM));
		addParam(createParamCentered<Switch2Pos>(mm2px(Vec(63.549, 83.801)), module, DrumBus::CH7MUTE_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(27.651, 95.501)), module, DrumBus::CH8VOL_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(45.649, 95.501)), module, DrumBus::CH8PAN_PARAM));
		addParam(createParamCentered<Switch2Pos>(mm2px(Vec(63.549, 95.499)), module, DrumBus::CH8MUTE_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(13.0, 111.002)), module, DrumBus::MASTERVOL_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.599, 13.501)), module, DrumBus::CH1IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.599, 25.199)), module, DrumBus::CH2IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.599, 36.901)), module, DrumBus::CH3IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.599, 48.599)), module, DrumBus::CH4IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.599, 60.301)), module, DrumBus::CH5IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.599, 71.999)), module, DrumBus::CH6IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.599, 83.799)), module, DrumBus::CH7IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.599, 95.501)), module, DrumBus::CH8IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(43.201, 111.002)), module, DrumBus::AUDIOLEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(58.0, 111.002)), module, DrumBus::AUDIORIGHTOUT_OUTPUT));
	}
};

Model *modelDrumBus = createModel<DrumBus, DrumBusWidget>("DrumBus");
