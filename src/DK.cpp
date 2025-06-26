#include "plugin.hpp"


struct DK : Module {
	enum ParamId {
		DECAY_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		DECAYCVIN_INPUT,
		TRIGIN_INPUT,
		AUDIOIN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		DECAYOUT_OUTPUT,
		AUDIOOUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LED_LIGHT,
		LIGHTS_LEN
	};

	DK() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(DECAY_PARAM, 0.f, 1.f, 0.f, "Decay", "ms", 0.f, 500.f);
		configInput(DECAYCVIN_INPUT, "Decay CV");
		configInput(TRIGIN_INPUT, "Trig");
		configInput(AUDIOIN_INPUT, "Audio");
		configOutput(DECAYOUT_OUTPUT, "Decay");
		configOutput(AUDIOOUT_OUTPUT, "Audio");
	}

	void process(const ProcessArgs& args) override {
		const float maxDecayMs = 500.0f;
		const float sampleRate = args.sampleRate;
		static float lastTrig = 0.0f;
		static float envelope = 0.0f;
	
		float trig = inputs[TRIGIN_INPUT].getVoltage();
		float decayParam = params[DECAY_PARAM].getValue();
		float decayCV = inputs[DECAYCVIN_INPUT].isConnected() ? inputs[DECAYCVIN_INPUT].getVoltage() / 5.0f : 0.0f;
		float decayControl = clamp(decayParam + decayCV, 0.0f, 1.0f);
	
		float decayTimeSec = (decayControl * maxDecayMs) / 1000.0f;
		if (decayTimeSec < 0.001f) decayTimeSec = 0.001f;  // avoid zero decay time
	
		bool trigRising = (trig >= 1.0f) && (lastTrig < 1.0f);
		lastTrig = trig;
	
		float decayCoeff = expf(-1.0f / (decayTimeSec * sampleRate));
	
		if (trigRising) {
			envelope = 5.0f;
		} else {
			envelope *= decayCoeff;
			if (envelope < 0.001f) envelope = 0.0f;
		}
	
		outputs[DECAYOUT_OUTPUT].setVoltage(envelope);
	
		float audioIn = inputs[AUDIOIN_INPUT].getVoltage();
		float audioOut = audioIn * (envelope / 5.0f);
		audioOut = clamp(audioOut, -5.0f, 5.0f);
	
		outputs[AUDIOOUT_OUTPUT].setVoltage(audioOut);
	
		lights[LED_LIGHT].setBrightnessSmooth(envelope, args.sampleTime);
	}	
};

struct DKWidget : ModuleWidget {
	DKWidget(DK* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/DK.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.426, 23.569)), module, DK::DECAY_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.426, 44.255)), module, DK::DECAYCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(4.891, 82.795)), module, DK::TRIGIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(4.891, 100.173)), module, DK::AUDIOIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.191, 82.795)), module, DK::DECAYOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.191, 100.173)), module, DK::AUDIOOUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.426, 60.653)), module, DK::LED_LIGHT));
	}
};


Model* modelDK = createModel<DK, DKWidget>("DK");