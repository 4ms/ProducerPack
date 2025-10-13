#include "plugin.hpp"

struct SoloPush : Module {
	enum ParamId { CH1RANGE_PARAM, CH1OFFSET_PARAM, CH1BEHAVIOR_PARAM, CH1PUSH_PARAM, PARAMS_LEN };
	enum InputId { INPUTS_LEN };
	enum OutputId { CH1VOLTAGEOUT_OUTPUT, CH1BUTTONOUT_OUTPUT, OUTPUTS_LEN };
	enum LightId { CH1_LIGHT, LIGHTS_LEN };

	SoloPush() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configSwitch(CH1RANGE_PARAM, 0.f, 2.f, 2.f, "Ch. 1 Range", {"0-1v", "0-5v", "-5v/+5v"});
		configParam(CH1OFFSET_PARAM, 0.f, 1.f, 1.0f, "Ch. 1 Offset", "%", 0.f, 100.f);
		configSwitch(CH1BEHAVIOR_PARAM, 0.f, 2.f, 0.f, "Ch. 1 Behavior", {"Gate", "Toggle", "Trig"});
		configSwitch(CH1PUSH_PARAM, 0.f, 1.f, 0.f, "Ch. 1", {"0", "1"});
		configOutput(CH1VOLTAGEOUT_OUTPUT, "Ch. 1 Voltage");
		configOutput(CH1BUTTONOUT_OUTPUT, "Ch. 1 Button");
	}

	void process(const ProcessArgs &args) override {
		const float buttonValue = params[CH1PUSH_PARAM].getValue();
		const bool buttonPressed = buttonValue > 0.5f;
		const bool risingEdge = buttonPressed && !prevButtonState;
		prevButtonState = buttonPressed;

		// Update mode only if changed
		const int newMode = (int)params[CH1BEHAVIOR_PARAM].getValue();
		if (newMode != cachedBehaviorMode) {
			cachedBehaviorMode = newMode;
			resetState();
			return;
		}

		// Update range if changed
		const int newRange = (int)params[CH1RANGE_PARAM].getValue();
		if (newRange != cachedRange) {
			cachedRange = newRange;
			updateOffsetVoltage();
		}

		// Update offset if changed
		const float newOffset = params[CH1OFFSET_PARAM].getValue();
		if (newOffset != cachedOffset) {
			cachedOffset = newOffset;
			updateOffsetVoltage();
		}

		float logicOut = 0.f;

		switch (cachedBehaviorMode) {
			case 0: // Momentary
				logicOut = buttonPressed ? 5.f : 0.f;
				break;

			case 1: // Toggle
				if (risingEdge)
					toggleState = !toggleState;
				logicOut = toggleState ? 5.f : 0.f;
				break;

			case 2: // Trigger
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

		// Output logic
		outputs[CH1BUTTONOUT_OUTPUT].setVoltage(logicOut);
		outputs[CH1VOLTAGEOUT_OUTPUT].setVoltage(logicOut > 0.f ? cachedOffsetVoltage : 0.f);

		// Light logic
		if (trigLightTimeRemaining > 0.f) {
			trigLightTimeRemaining -= args.sampleTime;
			lights[CH1_LIGHT].setBrightnessSmooth(1.f, args.sampleTime);
		} else {
			lights[CH1_LIGHT].setBrightnessSmooth(0.f);
		}
	}

private:
	// Logic state
	bool prevButtonState = false;
	bool toggleState = false;
	bool trigState = false;

	// Trigger timers
	float trigTimeRemaining = 0.f;
	float trigLightTimeRemaining = 0.f;

	// Cached parameter values
	int cachedBehaviorMode = -1;
	int cachedRange = -1;
	float cachedOffset = 0.f;
	float cachedOffsetVoltage = 0.f;

	static constexpr float rangeScale[3] = {1.f, 5.f, 10.f};
	static constexpr float rangeBias[3] = {0.f, 0.f, -5.f};
	// Resets internal state on mode change
	void resetState() {
		toggleState = false;
		trigState = false;
		trigTimeRemaining = 0.f;
		trigLightTimeRemaining = 0.f;
		prevButtonState = false;
		outputs[CH1BUTTONOUT_OUTPUT].setVoltage(0.f);
		outputs[CH1VOLTAGEOUT_OUTPUT].setVoltage(0.f);
		lights[CH1_LIGHT].setBrightness(0.f);
	}

	// Recalculates offset voltage when range or offset changes
	void updateOffsetVoltage() {
		cachedOffsetVoltage = cachedOffset * rangeScale[cachedRange] + rangeBias[cachedRange];
	}
};

struct SoloPushWidget : ModuleWidget {
	SoloPushWidget(SoloPush *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/SoloPush_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(10.16, 55.629)), module, SoloPush::CH1RANGE_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(10.16, 74.637)), module, SoloPush::CH1OFFSET_PARAM));
		addParam(createParamCentered<Switch3Pos>(mm2px(Vec(10.16, 93.644)), module, SoloPush::CH1BEHAVIOR_PARAM));
		addParam(createParamCentered<LEDBezel>(mm2px(Vec(10.16, 112.652)), module, SoloPush::CH1PUSH_PARAM));
		addChild(
			createLightCentered<LEDBezelLight<WhiteLight>>(mm2px(Vec(10.16, 112.652)), module, SoloPush::CH1_LIGHT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 17.614)), module, SoloPush::CH1VOLTAGEOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 36.622)), module, SoloPush::CH1BUTTONOUT_OUTPUT));
	}
};

Model *modelSoloPush = createModel<SoloPush, SoloPushWidget>("SoloPush");
