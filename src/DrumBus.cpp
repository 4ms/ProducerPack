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
	enum OutputId {
		AUDIOLEFTOUT_OUTPUT,
		AUDIORIGHTOUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

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

	void process(const ProcessArgs& args) override {
		float leftMix = 0.f;
		float rightMix = 0.f;
	
		for (int ch = 0; ch < 8; ++ch) {
			int volParam = CH1VOL_PARAM + ch * 3;
			int panParam = CH1PAN_PARAM + ch * 3;
			int muteParam = CH1MUTE_PARAM + ch * 3;
			int inputId = CH1IN_INPUT + ch;
	
			if (!inputs[inputId].isConnected()) continue;
			if (params[muteParam].getValue() > 0.5f) continue; // muted
	
			float in = inputs[inputId].getVoltage();
			float vol = params[volParam].getValue();
			float pan = params[panParam].getValue() / 100.f; // -0.5 to 0.5
	
			// Simple constant power pan law
			float leftGain = cosf((0.5f + pan) * M_PI_2);
			float rightGain = sinf((0.5f + pan) * M_PI_2);
	
			leftMix += in * vol * leftGain;
			rightMix += in * vol * rightGain;
		}
	
		float masterVol = params[MASTERVOL_PARAM].getValue();
		leftMix *= masterVol;
		rightMix *= masterVol;
	
		// Clamp to ±10V
		leftMix = clamp(leftMix, -10.f, 10.f);
		rightMix = clamp(rightMix, -10.f, 10.f);
	
		outputs[AUDIOLEFTOUT_OUTPUT].setVoltage(leftMix);
		outputs[AUDIORIGHTOUT_OUTPUT].setVoltage(rightMix);
	}	
};


struct DrumBusWidget : ModuleWidget {
	DrumBusWidget(DrumBus* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/DrumBus_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(25.046, 21.696)), module, DrumBus::CH1VOL_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(38.806, 21.696)), module, DrumBus::CH1PAN_PARAM));
		addParam(createParamCentered<_2Pos>(mm2px(Vec(51.723, 21.696)), module, DrumBus::CH1MUTE_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(25.046, 32.983)), module, DrumBus::CH2VOL_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(38.806, 32.983)), module, DrumBus::CH2PAN_PARAM));
		addParam(createParamCentered<_2Pos>(mm2px(Vec(51.723, 32.983)), module, DrumBus::CH2MUTE_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(25.046, 43.831)), module, DrumBus::CH3VOL_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(38.806, 43.831)), module, DrumBus::CH3PAN_PARAM));
		addParam(createParamCentered<_2Pos>(mm2px(Vec(51.723, 43.831)), module, DrumBus::CH3MUTE_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(25.046, 54.944)), module, DrumBus::CH4VOL_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(38.806, 54.944)), module, DrumBus::CH4PAN_PARAM));
		addParam(createParamCentered<_2Pos>(mm2px(Vec(51.723, 54.944)), module, DrumBus::CH4MUTE_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(25.046, 66.585)), module, DrumBus::CH5VOL_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(38.806, 66.585)), module, DrumBus::CH5PAN_PARAM));
		addParam(createParamCentered<_2Pos>(mm2px(Vec(51.723, 66.585)), module, DrumBus::CH5MUTE_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(25.046, 78.492)), module, DrumBus::CH6VOL_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(38.806, 78.492)), module, DrumBus::CH6PAN_PARAM));
		addParam(createParamCentered<_2Pos>(mm2px(Vec(51.723, 78.492)), module, DrumBus::CH6MUTE_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(25.046, 89.34)), module, DrumBus::CH7VOL_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(38.806, 89.34)), module, DrumBus::CH7PAN_PARAM));
		addParam(createParamCentered<_2Pos>(mm2px(Vec(51.723, 89.34)), module, DrumBus::CH7MUTE_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(25.046, 100.981)), module, DrumBus::CH8VOL_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(38.806, 100.981)), module, DrumBus::CH8PAN_PARAM));
		addParam(createParamCentered<_2Pos>(mm2px(Vec(51.723, 100.981)), module, DrumBus::CH8MUTE_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(42.405, 115.18)), module, DrumBus::MASTERVOL_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.171, 21.696)), module, DrumBus::CH1IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.171, 32.983)), module, DrumBus::CH2IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.171, 43.831)), module, DrumBus::CH3IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.171, 54.944)), module, DrumBus::CH4IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.171, 66.585)), module, DrumBus::CH5IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.171, 78.492)), module, DrumBus::CH6IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.171, 89.34)), module, DrumBus::CH7IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.171, 100.981)), module, DrumBus::CH8IN_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.003, 115.18)), module, DrumBus::AUDIOLEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(29.08, 115.18)), module, DrumBus::AUDIORIGHTOUT_OUTPUT));
	}
};


Model* modelDrumBus = createModel<DrumBus, DrumBusWidget>("DrumBus");