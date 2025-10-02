#include "plugin.hpp"

struct Decay : Module {
	enum ParamId { DECAY_PARAM, RANGE_PARAM, PARAMS_LEN };
	enum InputId { DECAYCVIN_INPUT, TRIGIN_INPUT, AUDIOIN_INPUT, INPUTS_LEN };
	enum OutputId { DECAYOUT_OUTPUT, AUDIOOUT_OUTPUT, OUTPUTS_LEN };
	enum LightId { LED_LIGHT, LIGHTS_LEN };

	Decay() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(DECAY_PARAM, 0.f, 1.f, 0.f, "Decay", "%", 0.f, 100.f);
		configSwitch(RANGE_PARAM, 0.f, 2.f, 0.f, "Range", {"Short", "Med", "Long"});
		configInput(DECAYCVIN_INPUT, "Decay CV");
		configInput(TRIGIN_INPUT, "Trig");
		configInput(AUDIOIN_INPUT, "Audio");
		configOutput(DECAYOUT_OUTPUT, "Envelope");
		configOutput(AUDIOOUT_OUTPUT, "Audio");
	}

	float lastTrig = 0.0f;
	float envelope = 0.0f;

	void process(const ProcessArgs &args) override {

		// Determine maxDecayMs only when the range param changes
		float maxDecayMs = 200.f;
		switch ((int)params[RANGE_PARAM].getValue()) {
			case 0:
				maxDecayMs = 30.f;
				break;
			case 1:
				maxDecayMs = 200.f;
				break;
			case 2:
				maxDecayMs = 5000.f;
				break;
		}

		// Cache input voltages and parameter values
		const float trig = inputs[TRIGIN_INPUT].getVoltage();
		const float decayParam = params[DECAY_PARAM].getValue();
		const float decayCV =
			inputs[DECAYCVIN_INPUT].isConnected() ? inputs[DECAYCVIN_INPUT].getVoltage() * 0.2f : 0.f; // /5.0f
		const float decayControl = clamp(decayParam + decayCV, 0.f, 1.f);

		// Avoid recalculating if decayControl is 0 (envelope decays immediately)
		float decayTimeSec = decayControl * maxDecayMs * 0.001f;
		if (decayTimeSec < 0.001f)
			decayTimeSec = 0.001f;

		// Calculate the decay coefficient only once
		const float decayCoeff = std::exp(-1.f / (decayTimeSec * args.sampleRate));

		// Detect rising edge trigger
		const bool trigRising = (trig >= 1.f && lastTrig < 1.f);
		lastTrig = trig;

		// Envelope logic
		if (trigRising) {
			envelope = 5.f;
		} else {
			envelope *= decayCoeff;
			if (envelope < 0.001f)
				envelope = 0.f;
		}

		outputs[DECAYOUT_OUTPUT].setVoltage(envelope);

		// Audio output with envelope applied
		const float audioIn = inputs[AUDIOIN_INPUT].getVoltage();
		float audioOut = audioIn * (envelope * 0.2f); // /5.0f
		outputs[AUDIOOUT_OUTPUT].setVoltage(clamp(audioOut, -5.f, 5.f));

		// LED update
		lights[LED_LIGHT].setBrightnessSmooth(envelope, args.sampleTime);
	}
};

struct DecayWidget : ModuleWidget {
	DecayWidget(Decay *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Decay_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(10.16, 15.501)), module, Decay::DECAY_PARAM));
		addParam(createParam<_3PosHorizontal>(mm2px(Vec(6.3, 24.5)), module, Decay::RANGE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 42.498)), module, Decay::DECAYCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 59.498)), module, Decay::TRIGIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 76.498)), module, Decay::AUDIOIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 93.499)), module, Decay::DECAYOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 109.998)), module, Decay::AUDIOOUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.581, 24.694)), module, Decay::LED_LIGHT));
	}
};

Model *modelDecay = createModel<Decay, DecayWidget>("Decay");