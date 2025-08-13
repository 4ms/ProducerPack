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
		CH1_LIGHT,
		LIGHTS_LEN
	};

	SoloPush() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configSwitch(CH1RANGE_PARAM, 0.f, 2.f, 2.f, "Ch. 1 Range", {"0-1v", "0-5v", "-5v/+5v"});
		configParam(CH1OFFSET_PARAM, 0.f, 1.f, 1.0f, "Ch. 1 Offset", "%", 0.f, 100.f);
		configSwitch(CH1BEHAVIOR_PARAM, 0.f, 2.f, 0.f, "Ch. 1 Behavior", {"Gate", "Toggle", "Trig"});
		configSwitch(CH1PUSH_PARAM, 0.f, 1.f, 0.f, "Ch. 1", {"0", "1"});
		configOutput(CH1VOLTAGEOUT_OUTPUT, "Ch. 1 Voltage");
		configOutput(CH1BUTTONOUT_OUTPUT, "Ch. 1 Button");
	}

	void process(const ProcessArgs& args) override {
		static constexpr float rangeScale[3] = {1.f, 5.f, 10.f};
		static constexpr float rangeBias[3] = {0.f, 0.f, -5.f};

		const float buttonValue = params[CH1PUSH_PARAM].getValue();
		const bool buttonPressed = buttonValue > 0.5f;
		const int mode = (int)params[CH1BEHAVIOR_PARAM].getValue();

		if (mode != prevBehaviorMode) {
			toggleState = false;
			trigState = false;
			trigTimeRemaining = 0.f;
			trigLightTimeRemaining = 0.f;
			prevButtonState = false;
			outputs[CH1BUTTONOUT_OUTPUT].setVoltage(0.f);
			outputs[CH1VOLTAGEOUT_OUTPUT].setVoltage(0.f);
			lights[CH1_LIGHT].setBrightness(0.f);
			prevBehaviorMode = mode;
			return;
		}

		const bool prevPressed = prevButtonState;
		prevButtonState = buttonPressed;
		const bool risingEdge = buttonPressed && !prevPressed;

		float logicOut = 0.f;

		switch (mode) {
			case 0:
				logicOut = buttonPressed ? 5.f : 0.f;
				break;
			case 1:
				if (risingEdge)
					toggleState = !toggleState;
				logicOut = toggleState ? 5.f : 0.f;
				break;
			case 2:
				if (risingEdge) {
					trigState = true;
					trigTimeRemaining = 0.005f;
					trigLightTimeRemaining = 0.1f;
				}
				if (trigState) {
					trigTimeRemaining -= args.sampleTime;
					if (trigTimeRemaining <= 0.f)
						trigState = false;
					else
						logicOut = 5.f;
				}
				break;
		}

		outputs[CH1BUTTONOUT_OUTPUT].setVoltage(logicOut);

		if (trigLightTimeRemaining > 0.f) {
			trigLightTimeRemaining -= args.sampleTime;
			lights[CH1_LIGHT].setBrightnessSmooth(1.f, args.sampleTime);
		} else {
			lights[CH1_LIGHT].setBrightnessSmooth((logicOut > 0.f ? 1.f : 0.f), args.sampleTime);
		}

		const int range = (int)params[CH1RANGE_PARAM].getValue();
		const float offset = params[CH1OFFSET_PARAM].getValue();
		const float offsetVoltage = offset * rangeScale[range] + rangeBias[range];
		const float voltageOut = logicOut > 0.f ? offsetVoltage : 0.f;
		outputs[CH1VOLTAGEOUT_OUTPUT].setVoltage(voltageOut);
	}
private:
	bool prevButtonState = false;
	bool toggleState = false;
	bool trigState = false;
	float trigTimeRemaining = 0.f;
	float trigLightTimeRemaining = 0.f;
	int prevBehaviorMode = -1;
};

struct SoloPushWidget : ModuleWidget {
	SoloPushWidget(SoloPush* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/SoloPush_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_3Pos>(mm2px(Vec(10.16, 55.629)), module, SoloPush::CH1RANGE_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(10.16, 74.637)), module, SoloPush::CH1OFFSET_PARAM));
		addParam(createParamCentered<_3Pos>(mm2px(Vec(10.16, 93.644)), module, SoloPush::CH1BEHAVIOR_PARAM));
		addParam(createParamCentered<LEDBezel>(mm2px(Vec(10.16, 112.652)), module, SoloPush::CH1PUSH_PARAM));
		addChild(createLightCentered<LEDBezelLight<WhiteLight>>(mm2px(Vec(10.16, 112.652)), module, SoloPush::CH1_LIGHT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 17.614)), module, SoloPush::CH1VOLTAGEOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 36.622)), module, SoloPush::CH1BUTTONOUT_OUTPUT));
	}
};


Model* modelSoloPush = createModel<SoloPush, SoloPushWidget>("SoloPush");