#include "plugin.hpp"

struct _2op : Module {
	enum ParamId {
		PITCH_PARAM,
		FMAMT_PARAM,
		RATIO_PARAM,
		DECAY_PARAM,
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
		configParam(DECAY_PARAM, 0.f, 1.f, 0.5f, "Decay", "ms", 0.f, 500.f);
		configInput(VOCTIN_INPUT, "1v/Oct");
		configInput(FMAMTCVIN_INPUT, "FM Amount CV");
		configInput(RATIOCVIN_INPUT, "Ratio CV");
		configInput(DECAYCVIN_INPUT, "Deca CV");
		configInput(GATEIN_INPUT, "Gate");
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
		float decayTimeMs = 1.f + (499.f * clamp(decayParam + decayCV, 0.f, 1.f)); // max 500 ms
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
		setPanel(createPanel(asset::plugin(pluginInstance, "res/2op.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.086, 18.629)), module, _2op::PITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.086, 41.44)), module, _2op::FMAMT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.086, 64.25)), module, _2op::RATIO_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(22.086, 87.06)), module, _2op::DECAY_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.394, 18.629)), module, _2op::VOCTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.394, 41.44)), module, _2op::FMAMTCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.394, 64.25)), module, _2op::RATIOCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.394, 87.06)), module, _2op::DECAYCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.394, 109.871)), module, _2op::GATEIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.086, 109.871)), module, _2op::AUDIO_OUTPUT));
	}
};


Model* model_2op = createModel<_2op, _2opWidget>("2op");