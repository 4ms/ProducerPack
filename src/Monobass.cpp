#include "plugin.hpp"


struct Monobass : Module {
	enum ParamId {
		OCTAVE_PARAM,
		FMAMOUNT_PARAM,
		WAVESHAPE_PARAM,
		PHASE_PARAM,
		MIXER_PARAM,
		DETUNE_PARAM,
		CUTOFF_PARAM,
		RESONANCE_PARAM,
		FILTERDECAY_PARAM,
		ENVDEPTH_PARAM,
		AMPDECAY_PARAM,
		GATETRIG_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		FMCV_INPUT,
		PHASECV_INPUT,
		DETUNECV_INPUT,
		RESONANCECV_INPUT,
		ENVDEPTHCV_INPUT,
		GATE_INPUT,
		WAVESHAPECV_INPUT,
		MIXERCV_INPUT,
		CUTOFFCV_INPUT,
		FILTERDECAYCV_INPUT,
		AMPDECAYCV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Monobass() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(OCTAVE_PARAM, 0.f, 3.f, 1.f, "Octave", {"-3", "-2", "-1", "0"});
		configParam(FMAMOUNT_PARAM, 0.f, 1.f, 0.f, "FM Amount", "%", 0.f, 100.f);
		configParam(WAVESHAPE_PARAM, 0.f, 1.f, 0.5f, "Waveshape", "%", 0.f, 100.f);
		configParam(PHASE_PARAM, 0.f, 1.f, 0.f, "Phase", "%", 0.f, 100.f);
		configParam(MIXER_PARAM, 0.f, 1.f, 0.f, "Mixer", "%", 0.f, 100.f);
		configParam(DETUNE_PARAM, 0.f, 1.f, 0.f, "Detune", "%", 0.f, 100.f);
		configParam(CUTOFF_PARAM, 0.f, 1.f, 0.5f, "Cutoff", "hz", 20.f, 350.f);
		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.5f, "Resonance", "%", 0.f, 100.f);
		configParam(FILTERDECAY_PARAM, 0.f, 1.f, 0.5f, "Filter Decay", "ms", 0.f, 100.f);
		configParam(ENVDEPTH_PARAM, 0.f, 1.f, 0.5f, "Envelope Depth", "%", 0.f, 100.f);
		configParam(AMPDECAY_PARAM, 0.f, 1.f, 0.5f, "Amp Decay", "ms", 50.f, 10.f);
		configSwitch(GATETRIG_PARAM, 0.f, 2.f, 0.f, "Gate Behavior", {"Gate", "Trig", "Drone"});
		configInput(VOCT_INPUT, "1v/Octave");
		configInput(FMCV_INPUT, "FM CV");
		configInput(PHASECV_INPUT, "Timbre CV");
		configInput(DETUNECV_INPUT, "Detune CV");
		configInput(RESONANCECV_INPUT, "Resonance CV");
		configInput(ENVDEPTHCV_INPUT, "Envelope Depth CV");
		configInput(GATE_INPUT, "Gate");
		configInput(WAVESHAPECV_INPUT, "Waveshape CV");
		configInput(MIXERCV_INPUT, "Mixer CV");
		configInput(CUTOFFCV_INPUT, "Cutoff CV");
		configInput(FILTERDECAYCV_INPUT, "Filter Decay CV");
		configInput(AMPDECAYCV_INPUT, "Amplitude Decay CV");
		configOutput(AUDIO_OUTPUT, "Audio");
	}

	float lowpassState[4] = {}; // 4-pole filter stages

	float applyLowpassFilter(float input, float cutoffHz, float resonance, float sampleRate) {
		// Calculate normalized cutoff
		cutoffHz = clamp(cutoffHz, 20.f, 5000.f);
		float f = cutoffHz / (sampleRate * 0.5f); // Normalize to Nyquist
		f = clamp(f, 0.f, 0.99f); // Ensure stability
	
		// Pre-warp frequency using bilinear transform approximation
		float g = tanf(float(M_PI) * f);
		float r = resonance * 3.5f; // Scale resonance to be wet but below self-osc
	
		// 4 cascaded 1-pole filters (Moog-style)
		float stageInput = input - r * lowpassState[3]; // Feedback from last stage
		for (int i = 0; i < 4; ++i) {
			lowpassState[i] += g * (stageInput - lowpassState[i]);
			stageInput = lowpassState[i];
		}
		return lowpassState[3]; // Output from last stage
	}
	
	inline float crossfade(float a, float b, float t) {
		return a + (b - a) * t;
	}

	float phase = 0.f;
float phaseDetuned = 0.f;
float phaseSaw = 0.f;  // New phase for the third sawtooth oscillator

float limiterEnvelope = 0.f;  // For dynamic peak tracking

float ampEnvelope = 0.f;
bool gateState = false;
bool lastGateState = false;
bool risingEdge = false; 

bool trigAttackPhase = false;  // true = currently in attack ramp on trigger

float filterEnvelope = 0.f;
bool filterTrigAttackPhase = false;

void process(const ProcessArgs& args) override {

	// === GATE & AMPLITUDE ENVELOPE ===
float gateIn = inputs[GATE_INPUT].getVoltage();
bool currentGateHigh = gateIn >= 1.f;

// Attack and decay times (in seconds)
float attackTime = 0.001f; // fixed 1ms attack 

float ampDecayParam = params[AMPDECAY_PARAM].getValue();
float ampDecayCV = inputs[AMPDECAYCV_INPUT].isConnected() ? clamp(inputs[AMPDECAYCV_INPUT].getVoltage() / 5.f, -1.f, 1.f) : 0.f;
float decayCombined = clamp(ampDecayParam + ampDecayCV, 0.f, 1.f);
float decayTime = 0.01f + decayCombined * 0.49f; // decayTime from 10ms to 500ms

// Coefficients for exponential envelope
float attackCoeff = std::exp(-1.f / (attackTime * args.sampleRate));
float decayCoeff = std::exp(-1.f / (decayTime * args.sampleRate));

// === FILTER ENVELOPE DECAY ===
float filterDecayParam = params[FILTERDECAY_PARAM].getValue();
float filterDecayCV = inputs[FILTERDECAYCV_INPUT].isConnected() ? clamp(inputs[FILTERDECAYCV_INPUT].getVoltage() / 5.f, -1.f, 1.f) : 0.f;
float filterDecayCombined = clamp(filterDecayParam + filterDecayCV, 0.f, 1.f);
float filterDecayTime = 0.01f + filterDecayCombined * 0.49f; // 10ms to 500ms
float filterDecayCoeff = std::exp(-1.f / (filterDecayTime * args.sampleRate));

// Envelope processing
int gateMode = (int)params[GATETRIG_PARAM].getValue();

switch (gateMode) {
    case 0: { // === GATE Mode ===
        if (currentGateHigh) {
            ampEnvelope += (1.f - ampEnvelope) * (1.f - attackCoeff);
        } else {
            ampEnvelope *= decayCoeff;
        }
        break;
    }
	case 1: { // === TRIG Mode ===
		risingEdge = (currentGateHigh && !lastGateState);
		if (risingEdge) {
			trigAttackPhase = true;   // start attack phase
		}
	
		if (trigAttackPhase) {
			// Attack ramp up
			ampEnvelope += (1.f - ampEnvelope) * (1.f - attackCoeff);
			// If close enough to 1, switch to decay
			if (ampEnvelope > 0.99f) {
				ampEnvelope = 1.f;
				trigAttackPhase = false;
			}
		} else {
			// Decay phase
			ampEnvelope *= decayCoeff;
		}
		break;
	}
	case 2: { // Drone Mode
	ampEnvelope = 0.75f;
	}
}

switch (gateMode) {
    case 0: { // GATE Mode
        if (gateState) {
            filterEnvelope += (1.f - filterEnvelope) * (1.f - attackCoeff);
        } else {
            filterEnvelope *= filterDecayCoeff;
        }
        break;
    }
    case 1: { // TRIG Mode
        if (risingEdge) {
            filterTrigAttackPhase = true;
        }
        if (filterTrigAttackPhase) {
            filterEnvelope += (1.f - filterEnvelope) * (1.f - attackCoeff);
            if (filterEnvelope > 0.99f) {
                filterEnvelope = 1.f;
                filterTrigAttackPhase = false;
            }
        } else {
            filterEnvelope *= filterDecayCoeff;
        }
        break;
    }
    case 2: { // DRONE Mode
		risingEdge = (currentGateHigh && !lastGateState);
		if (risingEdge) {
            filterTrigAttackPhase = true;
        }
        if (filterTrigAttackPhase) {
            filterEnvelope += (1.f - filterEnvelope) * (1.f - attackCoeff);
            if (filterEnvelope > 0.99f) {
                filterEnvelope = 1.f;
                filterTrigAttackPhase = false;
            }
        } else {
            filterEnvelope *= filterDecayCoeff;
        }
        break;
    }
}

// Update stored gate state
lastGateState = currentGateHigh;
gateState = currentGateHigh;

	const int numVoices = 3;

	// === Base pitch and frequency ===
	float pitchCV = inputs[VOCT_INPUT].getVoltage();
	float octaveParam = params[OCTAVE_PARAM].getValue();
	float freqOffset = octaveParam - 3.f;
	float fmCV = inputs[FMCV_INPUT].getVoltage();
	float fmAmount = params[FMAMOUNT_PARAM].getValue();
	const float maxFMDepth = 0.1f;
	float fmPitchOffset = clamp(fmCV / 5.f, -1.f, 1.f) * fmAmount * maxFMDepth;

	float basePitch = pitchCV + freqOffset + fmPitchOffset;

	float detuneKnob = params[DETUNE_PARAM].getValue();
	float detuneCV = inputs[DETUNECV_INPUT].isConnected() ? clamp(inputs[DETUNECV_INPUT].getVoltage() / 5.f, -1.f, 1.f) : 0.f;
	float totalDetuneControl = clamp(detuneKnob + detuneCV, 0.f, 1.f);
	float detuneSemitones = totalDetuneControl * 0.5f;
	float detunedPitch = basePitch + detuneSemitones / 12.f;

	// === Waveshape and Timbre ===
	float waveshapeParam = params[WAVESHAPE_PARAM].getValue();
	float waveshapeCV = inputs[WAVESHAPECV_INPUT].isConnected() ? clamp(inputs[WAVESHAPECV_INPUT].getVoltage() / 5.f, -1.f, 1.f) : 0.f;
	float shape = clamp(waveshapeParam + waveshapeCV, 0.f, 1.f);

	float phaseParam = params[PHASE_PARAM].getValue();
	float phaseCV = inputs[PHASECV_INPUT].isConnected() ? clamp(inputs[PHASECV_INPUT].getVoltage() / 5.f, -1.f, 1.f) : 0.f;
	float timbre = clamp(phaseParam + phaseCV, 0.f, 1.f);

	// === Frequencies ===
	float freq1 = 261.626f * std::pow(2.f, basePitch);
	float freq2 = 261.626f * std::pow(2.f, detunedPitch);

	// === Advance phases ===
	phase += freq1 / args.sampleRate;
	if (phase >= 1.f) phase -= 1.f;

	phaseDetuned += freq2 / args.sampleRate;
	if (phaseDetuned >= 1.f) phaseDetuned -= 1.f;

	// Advance phase for the sawtooth sub oscillator one octave below oscillator 1
	float freqSaw = freq1 / 2.f;  // One octave below oscillator 1
	phaseSaw += freqSaw / args.sampleRate;
	if (phaseSaw >= 1.f) phaseSaw -= 1.f;

	// === Generate voices ===
	float sum1 = 0.f;
	float sum2 = 0.f;

	for (int i = 0; i < numVoices; ++i) {
		float offset = timbre * (float)i / (float)numVoices;

		// Voice 1
		float p1 = phase + offset;
		p1 -= floorf(p1);
		float triangle1 = (p1 < 0.5f) ? (4.f * p1 - 1.f) : (3.f - 4.f * p1);
		float saw1 = 2.f * p1 - 1.f;
		float square1 = (p1 < 0.5f) ? 1.f : -1.f;
		float out1 = (shape < 0.5f) ? crossfade(triangle1, saw1, shape / 0.5f) :
									   crossfade(saw1, square1, (shape - 0.5f) / 0.5f);
		sum1 += out1;

		// Voice 2 (detuned)
		float p2 = phaseDetuned + offset;
		p2 -= floorf(p2);
		float triangle2 = (p2 < 0.5f) ? (4.f * p2 - 1.f) : (3.f - 4.f * p2);
		float saw2 = 2.f * p2 - 1.f;
		float square2 = (p2 < 0.5f) ? 1.f : -1.f;
		float out2 = (shape < 0.5f) ? crossfade(triangle2, saw2, shape / 0.5f) :
									   crossfade(saw2, square2, (shape - 0.5f) / 0.5f);
		sum2 += out2;
	}

	sum1 /= (float)numVoices;
	sum2 /= (float)numVoices;

	float saw3 = 2.f * phaseSaw - 1.f;

// mixerAmount is 0..1 from knob + CV summed & clamped
float mixerKnob = params[MIXER_PARAM].getValue();
float mixerCV = inputs[MIXERCV_INPUT].isConnected() ? clamp(inputs[MIXERCV_INPUT].getVoltage() / 5.f, -1.f, 1.f) : 0.f;
float mixerAmount = clamp(mixerKnob + mixerCV, 0.f, 1.f);

// Compute voice volumes
float vol1 = 1.f;                              // Voice 1 always full volume
float vol2 = clamp(mixerAmount * 2.f, 0.f, 1.f);         // 0..0.5 maps to 0..1 fade-in for voice 2
float vol3 = clamp((mixerAmount - 0.5f) * 2.f, 0.f, 1.f); // 0.5..1 maps to 0..1 fade-in for voice 3

// Sum volumes for normalization
float sumVol = vol1 + vol2 + vol3;

// Normalize volumes so overall amplitude constant
vol1 /= sumVol;
vol2 /= sumVol;
vol3 /= sumVol;

// Compose final output
float finalOutput = vol1 * sum1 + vol2 * sum2 + vol3 * saw3;

// Get base cutoff knob (0–1) mapped to -5V to +5V
float cutoffParam = params[CUTOFF_PARAM].getValue();
float cutoffOffsetV = rescale(cutoffParam, 0.f, 1.f, -5.f, 5.f);

// External cutoff CV input (already ±5V)
float cutoffCV = inputs[CUTOFFCV_INPUT].getVoltage();

// Filter envelope as 0–10V (envelope 0–1 * depth 0–1 * 10V)
float envDepthParam = params[ENVDEPTH_PARAM].getValue();
float envDepthCV = inputs[ENVDEPTHCV_INPUT].isConnected() ? clamp(inputs[ENVDEPTHCV_INPUT].getVoltage() / 5.f, -1.f, 1.f) : 0.f;
float envDepth = clamp(envDepthParam + envDepthCV, 0.f, 1.f);
float envelopeV = filterEnvelope * envDepth * 10.f;

// Sum all voltages and clamp to ±5V
float cutoffControlV = clamp(cutoffOffsetV + cutoffCV + envelopeV, -5.f, 5.f);

// Map -5V to +5V to cutoff frequency range (20 Hz to 7000 Hz)
float cutoffNorm = rescale(cutoffControlV, -5.f, 5.f, 0.f, 1.f);
float cutoffHz = 20.f * std::pow(350.f, cutoffNorm);  // exponential scale

// Apply filter envelope modulation to cutoff
float envelopeModHz = filterEnvelope * envDepth * 300.f; // up to 300 Hz boost
cutoffHz += envelopeModHz;
cutoffHz = clamp(cutoffHz, 20.f, 7000.f);

// Resonance
float resonanceParam = params[RESONANCE_PARAM].getValue();
float resonanceCV = inputs[RESONANCECV_INPUT].isConnected() ? clamp(inputs[RESONANCECV_INPUT].getVoltage() / 5.f, -1.f, 1.f) : 0.f;
float resonance = clamp(resonanceParam + resonanceCV, 0.f, 1.f);

// Filter the output
float filteredOutput = applyLowpassFilter(finalOutput, cutoffHz, resonance, args.sampleRate);

float resonanceGain = 0.5f + resonance; 
float boostedOutput = ((filteredOutput * resonanceGain) * 2.f) * ampEnvelope;
float outputVoltage = clamp(boostedOutput * 5.f, -10.f, 10.f);

outputs[AUDIO_OUTPUT].setVoltage(outputVoltage);

}
};

struct MonobassWidget : ModuleWidget {
	MonobassWidget(Monobass* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Monobass.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.049, 15.362)), module, Monobass::OCTAVE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(51.551, 15.362)), module, Monobass::FMAMOUNT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(80.499, 15.362)), module, Monobass::WAVESHAPE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.049, 34.146)), module, Monobass::PHASE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(51.551, 34.351)), module, Monobass::MIXER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(80.47, 34.124)), module, Monobass::DETUNE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.053, 52.032)), module, Monobass::CUTOFF_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.984, 52.032)), module, Monobass::RESONANCE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(64.341, 52.032)), module, Monobass::FILTERDECAY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(88.563, 52.032)), module, Monobass::ENVDEPTH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(16.997, 71.631)), module, Monobass::AMPDECAY_PARAM));
		addParam(createParamCentered<CKSSThreeHorizontal>(mm2px(Vec(51.551, 71.631)), module, Monobass::GATETRIG_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.819, 99.222)), module, Monobass::VOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.899, 99.222)), module, Monobass::FMCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(37.372, 99.222)), module, Monobass::PHASECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(50.179, 99.222)), module, Monobass::DETUNECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(63.228, 99.222)), module, Monobass::RESONANCECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(76.915, 99.222)), module, Monobass::ENVDEPTHCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.819, 112.786)), module, Monobass::GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.899, 112.786)), module, Monobass::WAVESHAPECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(37.372, 112.786)), module, Monobass::MIXERCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(50.179, 112.786)), module, Monobass::CUTOFFCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(63.228, 112.786)), module, Monobass::FILTERDECAYCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(76.915, 112.786)), module, Monobass::AMPDECAYCV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(89.797, 112.786)), module, Monobass::AUDIO_OUTPUT));
	}
};


Model* modelMonobass = createModel<Monobass, MonobassWidget>("Monobass");