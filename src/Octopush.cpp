#include "plugin.hpp"

struct OctoPush : Module {
	enum ParamId {
		CH1RANGE_PARAM,
		CH2RANGE_PARAM,
		CH3RANGE_PARAM,
		CH4RANGE_PARAM,
		CH5RANGE_PARAM,
		CH6RANGE_PARAM,
		CH7RANGE_PARAM,
		CH8RANGE_PARAM,
		CH1OFFSET_PARAM,
		CH2OFFSET_PARAM,
		CH3OFFSET_PARAM,
		CH4OFFSET_PARAM,
		CH5OFFSET_PARAM,
		CH6OFFSET_PARAM,
		CH7OFFSET_PARAM,
		CH8OFFSET_PARAM,
		CH1BEHAVIOR_PARAM,
		CH2BEHAVIOR_PARAM,
		CH3BEHAVIOR_PARAM,
		CH4BEHAVIOR_PARAM,
		CH5BEHAVIOR_PARAM,
		CH6BEHAVIOR_PARAM,
		CH7BEHAVIOR_PARAM,
		CH8BEHAVIOR_PARAM,
		CH1PUSH_PARAM,
		CH2PUSH_PARAM,
		CH3PUSH_PARAM,
		CH4PUSH_PARAM,
		CH5PUSH_PARAM,
		CH6PUSH_PARAM,
		CH7PUSH_PARAM,
		CH8PUSH_PARAM,
		PARAMS_LEN
	};
	enum InputId { INPUTS_LEN };
	enum OutputId {
		CH1VOLTAGEOUT_OUTPUT,
		CH2VOLTAGEOUT_OUTPUT,
		CH3VOLTAGEOUT_OUTPUT,
		CH4VOLTAGEOUT_OUTPUT,
		CH5VOLTAGEOUT_OUTPUT,
		CH6VOLTAGEOUT_OUTPUT,
		CH7VOLTAGEOUT_OUTPUT,
		CH8VOLTAGEOUT_OUTPUT,
		CH1BUTTONOUT_OUTPUT,
		CH2BUTTONOUT_OUTPUT,
		CH3BUTTONOUT_OUTPUT,
		CH4BUTTONOUT_OUTPUT,
		CH5BUTTONOUT_OUTPUT,
		CH6BUTTONOUT_OUTPUT,
		CH7BUTTONOUT_OUTPUT,
		CH8BUTTONOUT_OUTPUT,
		SUMOUT_OUTPUT,
		INVERSEOUT_OUTPUT,
		POSITIVEOUT_OUTPUT,
		NEGATIVEOUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId { CH1_LIGHT, CH2_LIGHT, CH3_LIGHT, CH4_LIGHT, CH5_LIGHT, CH6_LIGHT, CH7_LIGHT, CH8_LIGHT, LIGHTS_LEN };

	OctoPush() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		for (int i = 0; i < 8; i++) {
			configSwitch(CH1RANGE_PARAM + i,
						 0.f,
						 2.f,
						 2.f,
						 "Ch. " + std::to_string(i + 1) + " Range",
						 {"0-1v", "0-5v", "-5v/+5v"});
			configParam(
				CH1OFFSET_PARAM + i, 0.f, 1.f, 1.0f, "Ch. " + std::to_string(i + 1) + " Offset", "%", 0.f, 100.f);
			configSwitch(CH1BEHAVIOR_PARAM + i,
						 0.f,
						 2.f,
						 0.f,
						 "Ch. " + std::to_string(i + 1) + " Behavior",
						 {"Gate", "Toggle", "Trig"});
			configSwitch(CH1PUSH_PARAM + i, 0.f, 1.f, 0.f, "Ch. " + std::to_string(i + 1), {"0", "1"});
			configOutput(CH1VOLTAGEOUT_OUTPUT + i, "Ch. " + std::to_string(i + 1) + " Voltage");
			configOutput(CH1BUTTONOUT_OUTPUT + i, "Ch. " + std::to_string(i + 1) + " Button");
		}

		configOutput(SUMOUT_OUTPUT, "Sum");
		configOutput(INVERSEOUT_OUTPUT, "Inverse");
		configOutput(POSITIVEOUT_OUTPUT, "Positive");
		configOutput(NEGATIVEOUT_OUTPUT, "Negative");

		std::fill(std::begin(prevBehaviorMode), std::end(prevBehaviorMode), -1);
	}

	void process(const ProcessArgs &args) override {
		static constexpr float rangeScale[3] = {1.f, 5.f, 10.f};
		static constexpr float rangeBias[3] = {0.f, 0.f, -5.f};
		float sumVoltages = 0.f;

		for (int ch = 0; ch < 8; ch++) {
			const float buttonValue = params[CH1PUSH_PARAM + ch].getValue();
			const bool buttonPressed = buttonValue > 0.5f;
			const int mode = (int)params[CH1BEHAVIOR_PARAM + ch].getValue();

			if (mode != prevBehaviorMode[ch]) {
				toggleState[ch] = false;
				trigState[ch] = false;
				trigTimeRemaining[ch] = 0.f;
				trigLightTimeRemaining[ch] = 0.f;
				prevButtonState[ch] = false;
				outputs[CH1BUTTONOUT_OUTPUT + ch].setVoltage(0.f);
				outputs[CH1VOLTAGEOUT_OUTPUT + ch].setVoltage(0.f);
				lights[CH1_LIGHT + ch].setBrightness(0.f);
				prevBehaviorMode[ch] = mode;
				continue;
			}

			const bool prevPressed = prevButtonState[ch];
			prevButtonState[ch] = buttonPressed;
			const bool risingEdge = buttonPressed && !prevPressed;

			float logicOut = 0.f;

			switch (mode) {
				case 0:
					logicOut = buttonPressed ? 5.f : 0.f;
					break;
				case 1:
					if (risingEdge)
						toggleState[ch] = !toggleState[ch];
					logicOut = toggleState[ch] ? 5.f : 0.f;
					break;
				case 2:
					if (risingEdge) {
						trigState[ch] = true;
						trigTimeRemaining[ch] = 0.005f;
						trigLightTimeRemaining[ch] = 0.1f;
					}
					if (trigState[ch]) {
						trigTimeRemaining[ch] -= args.sampleTime;
						if (trigTimeRemaining[ch] <= 0.f)
							trigState[ch] = false;
						else
							logicOut = 5.f;
					}
					break;
			}

			outputs[CH1BUTTONOUT_OUTPUT + ch].setVoltage(logicOut);

			if (trigLightTimeRemaining[ch] > 0.f) {
				trigLightTimeRemaining[ch] -= args.sampleTime;
				lights[CH1_LIGHT + ch].setBrightnessSmooth(1.f, args.sampleTime);
			} else {
				lights[CH1_LIGHT + ch].setBrightnessSmooth((logicOut > 0.f ? 1.f : 0.f), args.sampleTime);
			}

			const int range = (int)params[CH1RANGE_PARAM + ch].getValue();
			const float offset = params[CH1OFFSET_PARAM + ch].getValue();
			const float offsetVoltage = offset * rangeScale[range] + rangeBias[range];

			const float voltageOut = logicOut > 0.f ? offsetVoltage : 0.f;
			outputs[CH1VOLTAGEOUT_OUTPUT + ch].setVoltage(voltageOut);

			sumVoltages += voltageOut;
		}

		outputs[SUMOUT_OUTPUT].setVoltage(clamp(sumVoltages, -5.f, 5.f));
		outputs[INVERSEOUT_OUTPUT].setVoltage(sumVoltages != 0.f ? clamp(1.f / sumVoltages, -5.f, 5.f) : 0.f);
		outputs[POSITIVEOUT_OUTPUT].setVoltage(sumVoltages > 0.f ? clamp(sumVoltages, 0.f, 5.f) : 0.f);
		outputs[NEGATIVEOUT_OUTPUT].setVoltage(sumVoltages < 0.f ? clamp(sumVoltages, -5.f, 0.f) : 0.f);
	}

private:
	bool prevButtonState[8] = {};
	bool toggleState[8] = {};
	bool trigState[8] = {};
	float trigTimeRemaining[8] = {};
	float trigLightTimeRemaining[8] = {};
	int prevBehaviorMode[8] = {};
};

struct OctoPushWidget : ModuleWidget {
	OctoPushWidget(OctoPush *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/OctoPush_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(7.188, 55.629)), module, OctoPush::CH1RANGE_PARAM));
		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(20.656, 55.629)), module, OctoPush::CH2RANGE_PARAM));
		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(34.124, 55.629)), module, OctoPush::CH3RANGE_PARAM));
		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(47.592, 55.629)), module, OctoPush::CH4RANGE_PARAM));
		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(61.061, 55.629)), module, OctoPush::CH5RANGE_PARAM));
		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(74.529, 55.629)), module, OctoPush::CH6RANGE_PARAM));
		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(87.997, 55.629)), module, OctoPush::CH7RANGE_PARAM));
		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(101.465, 55.629)), module, OctoPush::CH8RANGE_PARAM));

		addParam(createParamCentered<Knob9mm>(mm2px(Vec(7.188, 74.637)), module, OctoPush::CH1OFFSET_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(20.656, 74.637)), module, OctoPush::CH2OFFSET_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(34.124, 74.637)), module, OctoPush::CH3OFFSET_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(47.592, 74.637)), module, OctoPush::CH4OFFSET_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(61.061, 74.637)), module, OctoPush::CH5OFFSET_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(74.529, 74.637)), module, OctoPush::CH6OFFSET_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(87.997, 74.637)), module, OctoPush::CH7OFFSET_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(101.465, 74.637)), module, OctoPush::CH8OFFSET_PARAM));

		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(7.188, 93.644)), module, OctoPush::CH1BEHAVIOR_PARAM));
		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(20.656, 93.644)), module, OctoPush::CH2BEHAVIOR_PARAM));
		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(34.124, 93.644)), module, OctoPush::CH3BEHAVIOR_PARAM));
		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(47.592, 93.644)), module, OctoPush::CH4BEHAVIOR_PARAM));
		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(61.061, 93.644)), module, OctoPush::CH5BEHAVIOR_PARAM));
		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(74.529, 93.644)), module, OctoPush::CH6BEHAVIOR_PARAM));
		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(87.997, 93.644)), module, OctoPush::CH7BEHAVIOR_PARAM));
		addParam(
			createParamCentered<Switch3Pos>(mm2px(Vec(101.465, 93.644)), module, OctoPush::CH8BEHAVIOR_PARAM));

		addParam(createParamCentered<LEDBezel>(mm2px(Vec(7.188, 112.652)), module, OctoPush::CH1PUSH_PARAM));
		addChild(
			createLightCentered<LEDBezelLight<WhiteLight>>(mm2px(Vec(7.188, 112.652)), module, OctoPush::CH1_LIGHT));

		addParam(createParamCentered<LEDBezel>(mm2px(Vec(20.656, 112.652)), module, OctoPush::CH2PUSH_PARAM));
		addChild(
			createLightCentered<LEDBezelLight<WhiteLight>>(mm2px(Vec(20.656, 112.652)), module, OctoPush::CH2_LIGHT));

		addParam(createParamCentered<LEDBezel>(mm2px(Vec(34.124, 112.652)), module, OctoPush::CH3PUSH_PARAM));
		addChild(
			createLightCentered<LEDBezelLight<WhiteLight>>(mm2px(Vec(34.124, 112.652)), module, OctoPush::CH3_LIGHT));

		addParam(createParamCentered<LEDBezel>(mm2px(Vec(47.592, 112.652)), module, OctoPush::CH4PUSH_PARAM));
		addChild(
			createLightCentered<LEDBezelLight<WhiteLight>>(mm2px(Vec(47.592, 112.652)), module, OctoPush::CH4_LIGHT));

		addParam(createParamCentered<LEDBezel>(mm2px(Vec(61.061, 112.652)), module, OctoPush::CH5PUSH_PARAM));
		addChild(
			createLightCentered<LEDBezelLight<WhiteLight>>(mm2px(Vec(61.061, 112.652)), module, OctoPush::CH5_LIGHT));

		addParam(createParamCentered<LEDBezel>(mm2px(Vec(74.529, 112.652)), module, OctoPush::CH6PUSH_PARAM));
		addChild(
			createLightCentered<LEDBezelLight<WhiteLight>>(mm2px(Vec(74.529, 112.652)), module, OctoPush::CH6_LIGHT));

		addParam(createParamCentered<LEDBezel>(mm2px(Vec(87.997, 112.652)), module, OctoPush::CH7PUSH_PARAM));
		addChild(
			createLightCentered<LEDBezelLight<WhiteLight>>(mm2px(Vec(87.997, 112.652)), module, OctoPush::CH7_LIGHT));

		addParam(createParamCentered<LEDBezel>(mm2px(Vec(101.465, 112.652)), module, OctoPush::CH8PUSH_PARAM));
		addChild(
			createLightCentered<LEDBezelLight<WhiteLight>>(mm2px(Vec(101.465, 112.652)), module, OctoPush::CH8_LIGHT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.188, 17.614)), module, OctoPush::CH1VOLTAGEOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.656, 17.614)), module, OctoPush::CH2VOLTAGEOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(34.124, 17.614)), module, OctoPush::CH3VOLTAGEOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(47.592, 17.614)), module, OctoPush::CH4VOLTAGEOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(61.061, 17.614)), module, OctoPush::CH5VOLTAGEOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(74.529, 17.614)), module, OctoPush::CH6VOLTAGEOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(87.997, 17.614)), module, OctoPush::CH7VOLTAGEOUT_OUTPUT));
		addOutput(
			createOutputCentered<PJ301MPort>(mm2px(Vec(101.465, 17.614)), module, OctoPush::CH8VOLTAGEOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.188, 36.622)), module, OctoPush::CH1BUTTONOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.656, 36.622)), module, OctoPush::CH2BUTTONOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(34.124, 36.622)), module, OctoPush::CH3BUTTONOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(47.592, 36.622)), module, OctoPush::CH4BUTTONOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(61.061, 36.622)), module, OctoPush::CH5BUTTONOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(74.529, 36.622)), module, OctoPush::CH6BUTTONOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(87.997, 36.622)), module, OctoPush::CH7BUTTONOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(101.465, 36.622)), module, OctoPush::CH8BUTTONOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(112.187, 55.629)), module, OctoPush::SUMOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(124.268, 55.629)), module, OctoPush::INVERSEOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(112.187, 74.637)), module, OctoPush::POSITIVEOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(124.268, 74.637)), module, OctoPush::NEGATIVEOUT_OUTPUT));
	}
};

Model *modelOctoPush = createModel<OctoPush, OctoPushWidget>("OctoPush");
