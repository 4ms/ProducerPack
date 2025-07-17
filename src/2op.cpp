#include "plugin.hpp"

struct _2op : Module {
	enum ParamId {
		PITCH_PARAM,
		FMAMT_PARAM,
		RATIO_PARAM,
		DECAY_PARAM,
		RANGE_PARAM,
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
		configParam(PITCH_PARAM, 0.f, 1.f, 0.5f, "Pitch", "hz", 1000.f, 20.f);
		configParam(FMAMT_PARAM, 0.f, 1.f, 0.f, "FM Amount", "%", 0.f, 100.f);
		configParam(RATIO_PARAM, 0.f, 1.f, 0.f, "Ratio", "x", 80.f, 0.1f);
		configParam(DECAY_PARAM, 0.f, 1.f, 0.5f, "Decay", "%", 0.f, 100.f);
		configSwitch(RANGE_PARAM, 0.f, 2.f, 0.f, "Range", {"Short", "Med", "Long"});
		configInput(VOCTIN_INPUT, "1v/Oct");
		configInput(FMAMTCVIN_INPUT, "FM Amount CV");
		configInput(RATIOCVIN_INPUT, "Ratio CV");
		configInput(DECAYCVIN_INPUT, "Decay CV");
		configInput(GATEIN_INPUT, "Trig");
		configOutput(AUDIO_OUTPUT, "Audio");
	}

float carrierPhase = 0.f;
float modulatorPhase = 0.f;
float env = 0.f;
bool lastGate = false;

void process(const ProcessArgs& args) override {
	float sampleRate = args.sampleRate;

	float pitchParam = params[PITCH_PARAM].getValue();
	float baseFreq = 20.f * std::pow(1000.f, pitchParam);

	float voct = inputs[VOCTIN_INPUT].isConnected() ? inputs[VOCTIN_INPUT].getVoltage() : 0.f;
	float carrierFreqBase = baseFreq * std::pow(2.f, voct);
	carrierFreqBase = clamp(carrierFreqBase, 20.f, 20000.f);

	float ratioParam = params[RATIO_PARAM].getValue();
	float ratioCV = inputs[RATIOCVIN_INPUT].isConnected() ? inputs[RATIOCVIN_INPUT].getVoltage() / 5.f : 0.f;
	float ratio = clamp(ratioParam + ratioCV, 0.f, 1.f);
	float modFreq = carrierFreqBase * (0.1f + 7.9f * ratio);

	float fmParam = params[FMAMT_PARAM].getValue();
	float fmCV = inputs[FMAMTCVIN_INPUT].isConnected() ? inputs[FMAMTCVIN_INPUT].getVoltage() / 5.f : 0.f;
	float fmAmount = clamp(fmParam + fmCV, 0.f, 1.f) * 5000.f;

	bool gateConnected = inputs[GATEIN_INPUT].isConnected();
	bool gate = gateConnected && (inputs[GATEIN_INPUT].getVoltage() >= 1.f);

	if (gateConnected) {
		if (gate && !lastGate) {
			env = 1.f;
		}
		lastGate = gate;

		float decayParam = params[DECAY_PARAM].getValue();
		float decayCV = inputs[DECAYCVIN_INPUT].isConnected() ? inputs[DECAYCVIN_INPUT].getVoltage() / 5.f : 0.f;
		float decayControl = clamp(decayParam + decayCV, 0.f, 1.f);

		// Determine decay range from switch
		int range = static_cast<int>(params[RANGE_PARAM].getValue());
		float maxDecayMs = 200.f;  // Default: Medium

		switch (range) {
			case 0: maxDecayMs = 30.f; break;    // Fast
			case 1: maxDecayMs = 200.f; break;   // Medium
			case 2: maxDecayMs = 5000.f; break;  // Slow
		}

		float decayTimeMs = 1.f + (maxDecayMs - 1.f) * decayControl;
		float decayTimeSec = decayTimeMs / 1000.f;

		float decayCoeff = std::exp(-1.f / (decayTimeSec * sampleRate));
		env *= decayCoeff;

	} else {
		env = 1.f;
		lastGate = false;
	}

	modulatorPhase += modFreq / sampleRate;
	if (modulatorPhase >= 1.f)
		modulatorPhase -= 1.f;

	float modulatorOutput = std::sin(modulatorPhase * 2.f * M_PI);

	float instantaneousFreq = carrierFreqBase + (modulatorOutput * fmAmount);
	instantaneousFreq = clamp(instantaneousFreq, 20.f, 20000.f);

	carrierPhase += instantaneousFreq / sampleRate;
	if (carrierPhase >= 1.f)
		carrierPhase -= 1.f;

	float carrierOutput = std::sin(carrierPhase * 2.f * M_PI);

	outputs[AUDIO_OUTPUT].setVoltage(clamp(carrierOutput * 5.f * env, -5.f, 5.f));
}
};

struct _2opWidget : ModuleWidget {
	_2opWidget(_2op* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/2op_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(22.74, 19.502)), module, _2op::PITCH_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(22.74, 38.499)), module, _2op::FMAMT_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(22.74, 57.5)), module, _2op::RATIO_PARAM));
		addParam(createParam<_3Pos>(mm2px(Vec(6.999, 73.502)), module, _2op::RANGE_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(22.74, 76.501)), module, _2op::DECAY_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.74, 19.502)), module, _2op::VOCTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.74, 38.499)), module, _2op::FMAMTCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.74, 57.504)), module, _2op::RATIOCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 97.001)), module, _2op::DECAYCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.299, 111.003)), module, _2op::GATEIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.001, 111.003)), module, _2op::AUDIO_OUTPUT));
	}
};


Model* model_2op = createModel<_2op, _2opWidget>("2op");