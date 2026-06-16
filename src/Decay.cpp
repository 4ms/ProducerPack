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

	// --- Cached member variables ---
	float envelope[PORT_MAX_CHANNELS] = {};
	float lastTrig[PORT_MAX_CHANNELS] = {};

	float decayCoeff = 1.f;
	float maxDecayMs = 200.f;

	int cachedRangeSelection = -1;
	float cachedDecayParam = -1.f;
	float cachedDecayCV = -1.f;
	float cachedDecayControl = -1.f;

	bool updateDecay = true;

	void process(const ProcessArgs &args) override {
		// --- Mono CVs: decay time is shared across all channels ---
		const bool decayCvConnected = inputs[DECAYCVIN_INPUT].isConnected();
		const float decayCV = decayCvConnected ? inputs[DECAYCVIN_INPUT].getVoltage() * 0.2f : 0.f;
		const float decayParam = params[DECAY_PARAM].getValue();
		const int rangeSelection = (int)params[RANGE_PARAM].getValue();

		if (rangeSelection != cachedRangeSelection) {
			switch (rangeSelection) {
				case 0: maxDecayMs = 30.f; break;
				case 1: maxDecayMs = 200.f; break;
				case 2: maxDecayMs = 5000.f; break;
			}
			cachedRangeSelection = rangeSelection;
			updateDecay = true;
		}

		if (decayParam != cachedDecayParam || decayCV != cachedDecayCV) {
			cachedDecayParam = decayParam;
			cachedDecayCV = decayCV;
			const float newDecayControl = std::clamp(decayParam + decayCV, 0.f, 1.f);
			if (newDecayControl != cachedDecayControl) {
				cachedDecayControl = newDecayControl;
				updateDecay = true;
			}
		}

		if (updateDecay) {
			float decayTimeSec = cachedDecayControl * maxDecayMs * 0.001f;
			if (decayTimeSec < 0.001f)
				decayTimeSec = 0.001f;
			decayCoeff = exp(-1.f / (decayTimeSec * args.sampleRate));
			updateDecay = false;
		}

		// --- Channel count: max of trig and audio inputs ---
		const int nTrig = inputs[TRIGIN_INPUT].getChannels();
		const int nAudio = inputs[AUDIOIN_INPUT].getChannels();
		const int n = std::max(nTrig, nAudio);

		outputs[DECAYOUT_OUTPUT].setChannels(n);
		outputs[AUDIOOUT_OUTPUT].setChannels(n);

		float maxEnv = 0.f;

		for (int c = 0; c < n; c++) {
			const float trig = inputs[TRIGIN_INPUT].getPolyVoltage(c);
			const bool trigRising = (trig >= 1.f && lastTrig[c] < 1.f);
			lastTrig[c] = trig;

			if (trigRising) {
				envelope[c] = 5.f;
			} else {
				envelope[c] *= decayCoeff;
				if (envelope[c] < 0.001f)
					envelope[c] = 0.f;
			}

			outputs[DECAYOUT_OUTPUT].setVoltage(envelope[c], c);
			outputs[AUDIOOUT_OUTPUT].setVoltage(inputs[AUDIOIN_INPUT].getPolyVoltage(c) * (envelope[c] * 0.2f), c);

			maxEnv = std::max(maxEnv, envelope[c]);
		}

		lights[LED_LIGHT].setBrightnessSmooth(maxEnv, args.sampleTime);
	}
};

struct DecayWidget : ModuleWidget {
	DecayWidget(Decay *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Decay_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Knob9mm>(mm2px(Vec(10.16, 15.501)), module, Decay::DECAY_PARAM));
		addParam(createParam<Switch3PosHorizontal>(mm2px(Vec(6.3, 24.5)), module, Decay::RANGE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 42.498)), module, Decay::DECAYCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 59.498)), module, Decay::TRIGIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 76.498)), module, Decay::AUDIOIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 93.499)), module, Decay::DECAYOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 109.998)), module, Decay::AUDIOOUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.581, 24.694)), module, Decay::LED_LIGHT));
	}
};

Model *modelDecay = createModel<Decay, DecayWidget>("Decay");
