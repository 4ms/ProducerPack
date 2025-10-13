#include "plugin.hpp"
//
struct TwoOp : Module {
	enum ParamId { PITCH_PARAM, FMAMT_PARAM, RATIO_PARAM, DECAY_PARAM, RANGE_PARAM, PARAMS_LEN };
	enum InputId { VOCTIN_INPUT, FMAMTCVIN_INPUT, RATIOCVIN_INPUT, DECAYCVIN_INPUT, GATEIN_INPUT, INPUTS_LEN };
	enum OutputId { AUDIO_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	TwoOp() {
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

	void process(const ProcessArgs &args) override {
		// --- Pitch & Frequency ---
		float newPitch = params[PITCH_PARAM].getValue();
		float newVoct = inputs[VOCTIN_INPUT].getVoltage();

		if (newPitch != cachedPitch || newVoct != cachedVoct) {
			cachedPitch = newPitch;
			cachedVoct = newVoct;
			baseFreq = 20.f * pow(1000.f, cachedPitch);
			carrierFreq = baseFreq * exp(cachedVoct);
			carrierFreq = std::clamp(carrierFreq, 20.f, 20000.f);
		}

		// --- Ratio ---
		float newRatio = params[RATIO_PARAM].getValue();
		float newRatioCV = inputs[RATIOCVIN_INPUT].isConnected() ? inputs[RATIOCVIN_INPUT].getVoltage() : 0.f;

		if (newRatio != cachedRatio || newRatioCV != cachedRatioCV) {
			cachedRatio = newRatio;
			cachedRatioCV = newRatioCV;
			float ratio = std::clamp(cachedRatio + cachedRatioCV * 0.2f, 0.f, 1.f);
			modFreq = carrierFreq * (0.1f + 7.9f * ratio);
		}

		// --- FM Amount ---
		float newFmAmt = params[FMAMT_PARAM].getValue();
		float newFmAmtCV = inputs[FMAMTCVIN_INPUT].isConnected() ? inputs[FMAMTCVIN_INPUT].getVoltage() : 0.f;

		if (newFmAmt != cachedFmAmt || newFmAmtCV != cachedFmAmtCV) {
			cachedFmAmt = newFmAmt;
			cachedFmAmtCV = newFmAmtCV;
			float fm = std::clamp(cachedFmAmt + cachedFmAmtCV * 0.2f, 0.f, 1.f);
			fmAmount = fm * 5000.f;
		}

		// --- Gate & Envelope ---
		bool gate = inputs[GATEIN_INPUT].getVoltage() >= 1.f;

		if (inputs[GATEIN_INPUT].isConnected()) {
			if (gate && !lastGate)
				env = 1.f;
			lastGate = gate;

			// --- Decay ---
			float newDecay = params[DECAY_PARAM].getValue();
			float newDecayCV = inputs[DECAYCVIN_INPUT].isConnected() ? inputs[DECAYCVIN_INPUT].getVoltage() : 0.f;
			int newRange = (int)params[RANGE_PARAM].getValue();

			if (newDecay != cachedDecay || newDecayCV != cachedDecayCV || newRange != cachedRange) {
				cachedDecay = newDecay;
				cachedDecayCV = newDecayCV;
				cachedRange = newRange;

				float decay = std::clamp(cachedDecay + cachedDecayCV * 0.2f, 0.f, 1.f);
				const float maxDecayMs[] = {30.f, 200.f, 5000.f};
				float decayMs = 1.f + (maxDecayMs[std::clamp(cachedRange, 0, 2)] - 1.f) * decay;
				decayCoeff = exp(-args.sampleTime / (decayMs * 0.001f));
			}

			env *= decayCoeff;
		} else {
			env = 1.f;
			lastGate = false;
		}

		// --- Modulator Phase ---
		modulatorPhase += modFreq * args.sampleTime;
		if (modulatorPhase >= 1.f)
			modulatorPhase -= 1.f;
		float mod = sin(2.f * M_PI * modulatorPhase);

		// --- Carrier Phase ---
		float modulatedFreq = std::clamp(carrierFreq + mod * fmAmount, 20.f, 20000.f);
		carrierPhase += modulatedFreq * args.sampleTime;
		if (carrierPhase >= 1.f)
			carrierPhase -= 1.f;

		// --- Output ---
		float output = sin(2.f * M_PI * carrierPhase) * 5.f * env;
		outputs[AUDIO_OUTPUT].setVoltage(output);
	}

private:
	// Phases & Envelope
	float carrierPhase = 0.f;
	float modulatorPhase = 0.f;
	float env = 0.f;
	bool lastGate = false;

	// Cached Params & CVs
	float cachedPitch = -1.f;
	float cachedVoct = -1000.f;
	float baseFreq = 0.f;
	float carrierFreq = 0.f;

	float cachedRatio = -1.f;
	float cachedRatioCV = -1000.f;
	float modFreq = 0.f;

	float cachedFmAmt = -1.f;
	float cachedFmAmtCV = -1000.f;
	float fmAmount = 0.f;

	float cachedDecay = -1.f;
	float cachedDecayCV = -1000.f;
	int cachedRange = -1;
	float decayCoeff = 1.f;
};

struct TwoOpWidget : ModuleWidget {
	TwoOpWidget(TwoOp *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/TwoOp_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Knob9mm>(mm2px(Vec(22.74, 19.502)), module, TwoOp::PITCH_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(22.74, 38.499)), module, TwoOp::FMAMT_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(22.74, 57.5)), module, TwoOp::RATIO_PARAM));
		addParam(createParam<Switch3Pos>(mm2px(Vec(6.999, 73.502)), module, TwoOp::RANGE_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(22.74, 76.501)), module, TwoOp::DECAY_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.74, 19.502)), module, TwoOp::VOCTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.74, 38.499)), module, TwoOp::FMAMTCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.74, 57.504)), module, TwoOp::RATIOCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 97.001)), module, TwoOp::DECAYCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.299, 111.003)), module, TwoOp::GATEIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.001, 111.003)), module, TwoOp::AUDIO_OUTPUT));
	}
};

Model *modelTwoOp = createModel<TwoOp, TwoOpWidget>("TwoOp");
