#include "helpers/lut.hh"
#include "plugin.hpp"
#include <cmath> // for std::round

struct Monobass : Module {
	enum ParamId {
		OCTAVE_PARAM,
		FINETUNE_PARAM,
		WAVESHAPE_PARAM,
		TIMBRE_PARAM,
		MIXER_PARAM,
		DETUNE_PARAM,
		CUTOFF_PARAM,
		RESONANCE_PARAM,
		FILTERDECAY_PARAM,
		ENVDEPTH_PARAM,
		AMPDECAY_PARAM,
		GATETRIG_PARAM,
		LFOFREQ_PARAM,
		LFO_RANGE_PARAM,
		LFO_DEPTH_PARAM,
		UNIPOLARBIPOLAR_PARAM,
		LFO_SHAPE_PARAM,
		LFOFREQATT_PARAM,
		FMATT_PARAM,
		TIMBREATT_PARAM,
		DETUNEATT_PARAM,
		RESONANCEATT_PARAM,
		ENVDEPTHATT_PARAM,
		WAVESHAPEATT_PARAM,
		MIXERATT_PARAM,
		CUTOFFATT_PARAM,
		FILTERDECAYATT_PARAM,
		AMPDECAYATT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		GATE_INPUT,
		LFOCV_INPUT,
		FMCV_INPUT,
		TIMBRECV_INPUT,
		DETUNECV_INPUT,
		ENVDEPTHCV_INPUT,
		RESONANCECV_INPUT,
		WAVESHAPECV_INPUT,
		MIXERCV_INPUT,
		CUTOFFCV_INPUT,
		FILTERDECAYCV_INPUT,
		AMPDECAYCV_INPUT,
		VOCT_INPUT,
		LFORESET_INPUT,
		INPUTS_LEN
	};
	enum OutputId { LFO_OUT_OUTPUT, AUDIO_OUTPUT, OUTPUTS_LEN };
	enum LightId { LFOLEDRED_LIGHT, LFOLEDGREEN_LIGHT, LIGHTS_LEN };

	Monobass() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(OCTAVE_PARAM, 0.f, 3.f, 1.f, "Octave", {"-3", "-2", "-1", "0"});
		configParam(FINETUNE_PARAM, -0.1f, 0.1f, 0.f, "Fine Tune", "v");
		configParam(WAVESHAPE_PARAM, 0.f, 1.f, 0.5f, "Waveshape", "%", 0.f, 100.f);
		configParam(TIMBRE_PARAM, 0.f, 1.f, 0.f, "Phase", "%", 0.f, 100.f);
		configParam(MIXER_PARAM, 0.f, 1.f, 0.f, "Mixer", "%", 0.f, 100.f);
		configParam(DETUNE_PARAM, 0.f, 1.f, 0.f, "Detune", "%", 0.f, 100.f);
		configParam(CUTOFF_PARAM, 0.f, 1.f, 0.5f, "Cutoff", "hz", 20.f, 350.f);
		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.5f, "Resonance", "%", 0.f, 100.f);
		configParam(FILTERDECAY_PARAM, 0.f, 1.f, 0.5f, "Filter Decay", "ms", 0.f, 100.f);
		configParam(ENVDEPTH_PARAM, 0.f, 1.f, 0.5f, "Envelope Depth", "%", 0.f, 100.f);
		configParam(AMPDECAY_PARAM, 0.f, 1.f, 0.5f, "Amp Decay", "ms", 50.f, 10.f);

		configSwitch(GATETRIG_PARAM, 0.f, 2.f, 0.f, "Gate Behavior", {"Gate", "Trig", "Drone"});

		configParam(LFOFREQ_PARAM, 0.f, 1.f, 0.f, "LFO Frequency", "%", 0.f, 100.f);
		configSwitch(LFO_RANGE_PARAM, 0.f, 1.f, 0.f, "LFO Frequency Range", {"Slow", "Fast"});
		configParam(LFO_DEPTH_PARAM, 0.f, 1.f, 0.f, "LFO Depth", "%", 0.f, 100.f);
		configSwitch(LFO_SHAPE_PARAM,
					 0.f,
					 5.f,
					 0.f,
					 "LFO Shape",
					 {"Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Random"});
		configSwitch(
			UNIPOLARBIPOLAR_PARAM, 0.f, 1.f, 0.f, "LFO Output Voltage Range", {"Bipolar +/-5v", "Unipolar 0-5v"});

		configParam(LFOFREQATT_PARAM, -100.f, 100.f, 0.f, "LFO Frequency Attenuverter", "%");
		configParam(FMATT_PARAM, 0.f, 1.f, 0.f, "FM Amount", "%", 0.f, 100.f);
		configParam(TIMBREATT_PARAM, -100.f, 100.f, 0.f, "Phase Attenuverter", "%");
		configParam(DETUNEATT_PARAM, -100.f, 100.f, 0.f, "Detune Attenuverter", "%");
		configParam(RESONANCEATT_PARAM, -100.f, 100.f, 0.f, "Resonance Attenuverter", "%");
		configParam(ENVDEPTHATT_PARAM, -100.f, 100.f, 0.f, "Envelope Depth Attenuverter", "%");
		configParam(WAVESHAPEATT_PARAM, -100.f, 100.f, 0.f, "Waveshape Attenuverter", "%");
		configParam(MIXERATT_PARAM, -100.f, 100.f, 0.f, "Mixer Attenuverter", "%");
		configParam(CUTOFFATT_PARAM, -100.f, 100.f, 0.f, "Cutoff Frequency Attenuverter", "%");
		configParam(FILTERDECAYATT_PARAM, -100.f, 100.f, 0.f, "Filter Decay Attenuverter", "%");
		configParam(AMPDECAYATT_PARAM, -100.f, 100.f, 0.f, "Amplitude Decay Attenuverter", "%");

		configInput(GATE_INPUT, "Gate");
		configInput(LFOCV_INPUT, "LFO Frequency CV");
		configInput(FMCV_INPUT, "FM CV");
		configInput(TIMBRECV_INPUT, "Phase CV");
		configInput(DETUNECV_INPUT, "Detune CV");
		configInput(ENVDEPTHCV_INPUT, "Envelope Depth CV");
		configInput(RESONANCECV_INPUT, "Resonance CV");
		configInput(WAVESHAPECV_INPUT, "Waveshape CV");
		configInput(MIXERCV_INPUT, "Mixer CV");
		configInput(CUTOFFCV_INPUT, "Cutoff CV");
		configInput(FILTERDECAYCV_INPUT, "Filter Decay CV");
		configInput(AMPDECAYCV_INPUT, "Amplitude Decay CV");
		configInput(VOCT_INPUT, "1v/Octave");
		configInput(LFORESET_INPUT, "LFO Reset");

		configOutput(LFO_OUT_OUTPUT, "LFO");
		configOutput(AUDIO_OUTPUT, "Audio");
	}

	float lowpassState[4] = {}; // 4-pole filter stages

	float applyLowpassFilter(float input, float cutoffHz, float resonance, float sampleRate) {
		// Calculate normalized cutoff
		cutoffHz = std::clamp(cutoffHz, 20.f, 5000.f);
		float f = cutoffHz / (sampleRate * 0.5f); // Normalize to Nyquist
		f = std::clamp(f, 0.f, 0.99f);			  // Ensure stability

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

	inline float equalPowerCrossfade(float a, float b, float t) {
		t = std::clamp(t, 0.f, 1.f);
		float angle = t * (float)M_PI / 2.f;
		return a * cos(angle) + b * sin(angle);
	}

	float phase = 0.f;
	float phaseDetuned = 0.f;
	float phaseSaw = 0.f; // New phase for the third sawtooth oscillator

	float limiterEnvelope = 0.f; // For dynamic peak tracking

	float ampEnvelope = 0.f;
	bool gateState = false;
	bool lastGateState = false;
	bool risingEdge = false;

	bool trigAttackPhase = false; // true = currently in attack ramp on trigger

	float filterEnvelope = 0.f;
	bool filterTrigAttackPhase = false;

	float sum1 = 0;
	float sum2 = 0;

	float agcEnvelope = 1.f;

	float lfoPhase = 0.f;
	float lfoValue = 0.f;
	float lfoRandomValue = 0.f;
	float lfoFreq = 0.f;

	bool lfoResetEnabled;		// From the UI or a physical switch
	bool prevGateState = false; // To detect rising edge

	struct Pow2TableRange {
		static constexpr float min = -15.f;
		static constexpr float max = 12.f;
	};

	static inline const auto Pow2 =
		Mapping::LookupTable_t<64, float>::generate<Pow2TableRange>([](float x) { return std::pow(2.f, x); });

	void process(const ProcessArgs &args) override {
		float gateIn = inputs[GATE_INPUT].getVoltage();
		float lfoReset = inputs[LFORESET_INPUT].getVoltage();
		bool currentGateHigh = gateIn >= 1.f;

		static bool prevGateIn = false;
		bool gateRisingEdge = currentGateHigh && !prevGateIn;
		prevGateIn = currentGateHigh;

		// === LFO Frequency Computation ===
		float freqParam = params[LFOFREQ_PARAM].getValue(); // 0–1
		float freqAtt = params[LFOFREQATT_PARAM].getValue() / 100.f;
		float freqCV = inputs[LFOCV_INPUT].isConnected() ? inputs[LFOCV_INPUT].getVoltage() / 5.f : 0.f;
		float freqMod = std::clamp(freqParam + freqCV * freqAtt, 0.f, 1.f);

		bool LFORange = (params[LFO_RANGE_PARAM].getValue() > 0.5f);

		if (LFORange) {
			// Fast range: 5 Hz to 20 Hz
			lfoFreq = 5.f + freqMod * (20.f - 1.f); // 5–20 Hz
		} else {
			// Slow range: 0.01 Hz to 5 Hz
			lfoFreq = 0.01f + freqMod * (1.f - 0.01f); // 0.01–5 Hz
		}

		bool currentGateState = lfoReset >= 1.f; // Gate HIGH when voltage >= 1V (your threshold)
		if (!prevGateState && currentGateState) {
			lfoPhase = 0.f; // Rising edge detected, reset phase
		}
		prevGateState = currentGateState;

		bool isRandomShape = ((int)params[LFO_SHAPE_PARAM].getValue() == 5);
		bool isLfoFreqZero = (freqParam + freqCV * freqAtt) <= 0.0001f;

		if (isRandomShape && isLfoFreqZero) {
			// LFO is in stepped random mode and frequency is 0 – gate-triggered
			if (gateRisingEdge) { lfoRandomValue = 2.f * ((float)rand() / (float)RAND_MAX) - 1.f; }
		} else {
			// Free-running mode
			lfoPhase += lfoFreq / args.sampleRate;
			if (lfoPhase >= 1.f) {
				lfoPhase -= 1.f;
				if (isRandomShape) { lfoRandomValue = 2.f * ((float)rand() / (float)RAND_MAX) - 1.f; }
			}
		}

		// === LFO Depth Control ===
		float amp = params[LFO_DEPTH_PARAM].getValue(); // 0–1

		// === LFO Shape ===
		int LFOshape = (int)params[LFO_SHAPE_PARAM].getValue();
		float t = lfoPhase;
		float out = 0.f;

		switch (LFOshape) {
			case 0: {			 // Basic sine approximation, no libraries
				float phase = t; // t in [0, 1)
				float x = 2.0f * phase - 1.0f;
				float abs_x = x < 0.0f ? -x : x;
				float y = 4.0f * x * (1.0f - abs_x);
				out = y;
				break;
			}

			case 1: // Triangle
				if (t < 0.5f)
					out = 4.f * t - 1.f;
				else
					out = 3.f - 4.f * t;
				break;
			case 2: // Ramp Up (Saw)
				out = 2.f * t - 1.f;
				break;
			case 3: // Ramp Down
				out = 1.f - 2.f * t;
				break;
			case 4: // Square
				out = (t < 0.5f) ? 1.f : -1.f;
				break;
			case 5: // Stepped Random
				out = lfoRandomValue;
				break;
		}

		// LFO unipolar / bipolar switch
		bool LFOoffsetSwitch = params[UNIPOLARBIPOLAR_PARAM].getValue() > 0.5f;
		float outputValue;
		if (LFOoffsetSwitch) {
			// Unipolar: map [-1, 1] to [0, 5]
			outputValue = ((out + 1.f) * 0.5f) * amp * 5.f;
		} else {
			// Bipolar: map [-1, 1] to [-5, 5]
			outputValue = out * amp * 5.f;
		}

		// Send to output
		outputs[LFO_OUT_OUTPUT].setVoltage(outputValue);

		float maxVoltage = 5.f;
		float positiveHalf = std::max(outputValue, 0.f) / maxVoltage;
		float negativeHalf = std::max(-outputValue, 0.f) / maxVoltage;

		positiveHalf = std::clamp(positiveHalf, 0.f, 1.f);
		negativeHalf = std::clamp(negativeHalf, 0.f, 1.f);

		lights[LFOLEDRED_LIGHT].setBrightnessSmooth(negativeHalf, args.sampleTime);	  // Red
		lights[LFOLEDGREEN_LIGHT].setBrightnessSmooth(positiveHalf, args.sampleTime); // Green

		// === AMPLITUDE ENVELOPE ===
		// Attack and decay times (in seconds)
		float attackTime = 0.001f; // fixed 1ms attack

		float ampDecayParam = params[AMPDECAY_PARAM].getValue();
		float ampDecayCV = inputs[AMPDECAYCV_INPUT].isConnected() ?
							   std::clamp(inputs[AMPDECAYCV_INPUT].getVoltage() / 5.f, -1.f, 1.f) :
							   outputValue;
		float ampDecayAtt = params[AMPDECAYATT_PARAM].getValue() / 100.f;
		float decayCombined = std::clamp(ampDecayParam + (ampDecayCV * ampDecayAtt), 0.f, 1.f);
		float decayTime = 0.01f + decayCombined * 0.49f; // decayTime from 10ms to 500ms

		// Coefficients for exponential envelope
		float attackCoeff = exp(-1.f / (attackTime * args.sampleRate));
		float decayCoeff = exp(-1.f / (decayTime * args.sampleRate));

		// === FILTER ENVELOPE DECAY ===
		float filterDecayParam = params[FILTERDECAY_PARAM].getValue();
		float filterDecayCV = inputs[FILTERDECAYCV_INPUT].isConnected() ?
								  std::clamp(inputs[FILTERDECAYCV_INPUT].getVoltage() / 5.f, -1.f, 1.f) :
								  outputValue;
		float filterDecayAtt = params[FILTERDECAYATT_PARAM].getValue() / 100.f;
		float filterDecayCombined = std::clamp(filterDecayParam + (filterDecayCV * filterDecayAtt), 0.f, 1.f);
		float filterDecayTime = 0.01f + filterDecayCombined * 0.49f; // 10ms to 500ms
		float filterDecayCoeff = exp(-1.f / (filterDecayTime * args.sampleRate));

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
					trigAttackPhase = true; // start attack phase
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
				if (risingEdge) { filterTrigAttackPhase = true; }
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
				if (risingEdge) { filterTrigAttackPhase = true; }
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

		float pitchCV = inputs[VOCT_INPUT].getVoltage();
		float octaveParam = params[OCTAVE_PARAM].getValue();
		float freqOffset = octaveParam - 3.f;
		float fmCVRaw = inputs[FMCV_INPUT].isConnected() ? inputs[FMCV_INPUT].getVoltage() : outputValue;
		float fmCV = std::clamp(fmCVRaw / 5.f, -1.f, 1.f);
		float fmAmount = params[FMATT_PARAM].getValue();
		const float maxFMDepth = 0.1f;
		float fmPitchOffset = fmCV * fmAmount * maxFMDepth;

		float fineTune = params[FINETUNE_PARAM].getValue();

		// [-11, 11] + [-0.1, 0.1] + [-3, 0] + [-0.1, 0.1] => [-14.2, 11.2]
		float basePitch = pitchCV + fineTune + freqOffset + fmPitchOffset;

		float detuneKnob = params[DETUNE_PARAM].getValue();
		float detuneCV = inputs[DETUNECV_INPUT].isConnected() ?
							 std::clamp(inputs[DETUNECV_INPUT].getVoltage() / 5.f, -1.f, 1.f) :
							 outputValue;
		float detuneAtt = params[DETUNEATT_PARAM].getValue() / 100.f;
		float totalDetuneControl = std::clamp(detuneKnob + detuneCV * detuneAtt, 0.f, 1.f);
		float detuneSemitones = totalDetuneControl * 0.5f;
		float detunedPitch = basePitch + detuneSemitones / 12.f;

		// === Waveshape and Timbre ===
		float waveshapeParam = params[WAVESHAPE_PARAM].getValue();
		float waveshapeCV = inputs[WAVESHAPECV_INPUT].isConnected() ?
								clamp(inputs[WAVESHAPECV_INPUT].getVoltage() / 5.f, -1.f, 1.f) :
								outputValue;
		float waveshapeAtt = params[WAVESHAPEATT_PARAM].getValue() / 100.f;
		float shape = std::clamp(waveshapeParam + waveshapeCV * waveshapeAtt, 0.f, 1.f);

		// Automatic Gain Compensation calculation
		float shapeGain = 1.f;

		// Morph triangle -> saw
		if (shape < 0.5f) {
			float t = shape / 0.5f;
			float trianglePeak = 0.8f; // example approximation
			float sawPeak = 1.0f;
			float currentPeak = (1.f - t) * trianglePeak + t * sawPeak;
			shapeGain = 1.f / currentPeak;
		} else {
			float t = (shape - 0.5f) / 0.5f;
			float sawPeak = 1.0f;
			float squarePeak = 1.0f;
			float currentPeak = (1.f - t) * sawPeak + t * squarePeak;
			shapeGain = 1.f / currentPeak;
		}

		float phaseParam = params[TIMBRE_PARAM].getValue();
		float phaseCV = inputs[TIMBRECV_INPUT].isConnected() ?
							clamp(inputs[TIMBRECV_INPUT].getVoltage() / 5.f, -1.f, 1.f) :
							outputValue;
		float phaseAtt = params[TIMBREATT_PARAM].getValue() / 100.f;
		float timbre = std::clamp(phaseParam + phaseCV * phaseAtt, 0.f, 1.f);

		// === Frequencies ===
		float freq1 = 261.626f * Pow2(basePitch);
		float freq2 = 261.626f * Pow2(detunedPitch);

		// === Advance phases ===
		phase += freq1 / args.sampleRate;
		if (phase >= 1.f)
			phase -= 1.f;

		phaseDetuned += freq2 / args.sampleRate;
		if (phaseDetuned >= 1.f)
			phaseDetuned -= 1.f;

		// Advance phase for the sawtooth sub oscillator one octave below oscillator 1
		float freqSaw = freq1 / 2.f; // One octave below oscillator 1
		phaseSaw += freqSaw / args.sampleRate;
		if (phaseSaw >= 1.f)
			phaseSaw -= 1.f;

		// === Generate voices ===
		sum1 = 0.f;
		sum2 = 0.f;

		for (int i = 0; i < numVoices; ++i) {
			float offset = timbre * (float)i / (float)numVoices;

			float p1 = phase + offset;
			p1 -= floorf(p1);
			float triangle1 = (p1 < 0.5f) ? (4.f * p1 - 1.f) : (3.f - 4.f * p1);
			float saw1 = 2.f * p1 - 1.f;
			float square1 = (p1 < 0.5f) ? 1.f : -1.f;

			// Normalize amplitudes for perceptual consistency
			triangle1 *= 0.7f; // triangle has peak amplitude of 1, but perceived quieter
			saw1 *= 0.8f;	   // saw is brighter but slightly quieter than square
			square1 *= 0.6f;   // square tends to sound louder due to more harmonic content

			float raw1 = (shape < 0.5f) ? equalPowerCrossfade(triangle1, saw1, shape / 0.5f) :
										  equalPowerCrossfade(saw1, square1, (shape - 0.5f) / 0.5f);
			raw1 *= shapeGain; // Apply gain compensation
			sum1 += raw1;

			float p2 = phaseDetuned + offset;
			p2 -= floorf(p2);
			float triangle2 = (p2 < 0.5f) ? (4.f * p2 - 1.f) : (3.f - 4.f * p2);
			float saw2 = 2.f * p2 - 1.f;
			float square2 = (p2 < 0.5f) ? 1.f : -1.f;

			// Normalize amplitudes for perceptual consistency
			triangle2 *= 0.7f; // triangle has peak amplitude of 1, but perceived quieter
			saw2 *= 0.8f;	   // saw is brighter but slightly quieter than square
			square2 *= 0.6f;   // square tends to sound louder due to more harmonic content

			float raw2 = (shape < 0.5f) ? equalPowerCrossfade(triangle2, saw2, shape / 0.5f) :
										  equalPowerCrossfade(saw2, square2, (shape - 0.5f) / 0.5f);
			raw2 *= shapeGain; // Apply gain compensation
			sum2 += raw2;
		}

		sum1 /= (float)numVoices;
		sum2 /= (float)numVoices;

		float saw3 = 2.f * phaseSaw - 1.f;

		// mixerAmount is 0..1 from knob + CV summed & clamped
		float mixerKnob = params[MIXER_PARAM].getValue();
		float mixerCV = inputs[MIXERCV_INPUT].isConnected() ?
							clamp(inputs[MIXERCV_INPUT].getVoltage() / 5.f, -1.f, 1.f) :
							outputValue;
		float mixerAtt = params[MIXERATT_PARAM].getValue() / 100.f;
		float mixerAmount = std::clamp(mixerKnob + (mixerCV * mixerAtt), 0.f, 1.f);

		// Compute voice volumes
		float vol1 = 1.f;											   // Voice 1 always full volume
		float vol2 = std::clamp(mixerAmount * 2.f, 0.f, 1.f);		   // 0..0.5 maps to 0..1 fade-in for voice 2
		float vol3 = std::clamp((mixerAmount - 0.5f) * 2.f, 0.f, 1.f); // 0.5..1 maps to 0..1 fade-in for voice 3

		// Sum volumes for normalization
		float sumVol = vol1 + vol2 + vol3;

		// Normalize volumes so overall amplitude constant
		vol1 /= sumVol;
		vol2 /= sumVol;
		vol3 /= sumVol;

		// Combine voices
		float finalOutput = vol1 * sum1 + vol2 * sum2 + vol3 * saw3;

		// === AGC: Normalize to ~4Vpp ===
		// Simple peak-following envelope
		float targetVpp = 4.f;				// Desired peak-to-peak
		float targetPeak = targetVpp / 2.f; // ±2V

		// Fast attack, slow release
		float agcAttack = 1.f - exp(-1.f / (0.001f * args.sampleRate)); // 1ms attack
		float agcRelease = 1.f - exp(-1.f / (0.1f * args.sampleRate));	// 100ms release

		float absSample = std::fabs(finalOutput);
		if (absSample > agcEnvelope)
			agcEnvelope += agcAttack * (absSample - agcEnvelope);
		else
			agcEnvelope += agcRelease * (absSample - agcEnvelope);

		// Prevent divide by 0
		float gain = (agcEnvelope > 1e-6f) ? (targetPeak / agcEnvelope) : 1.f;
		gain = std::clamp(gain, 0.1f, 10.f); // Prevent wild gain swings

		finalOutput *= (gain * 0.5);

		// Filter envelope as 0–10V (envelope 0–1 * depth 0–1 * 10V)
		float envDepthParam = params[ENVDEPTH_PARAM].getValue();
		float envDepthCV = inputs[ENVDEPTHCV_INPUT].isConnected() ?
							   std::clamp(inputs[ENVDEPTHCV_INPUT].getVoltage() / 5.f, -1.f, 1.f) :
							   outputValue;
		float envDepthAtt = params[ENVDEPTHATT_PARAM].getValue() / 100.f;
		float envDepth = std::clamp(envDepthParam + (envDepthCV * envDepthAtt), 0.f, 1.f);
		float envelopeV = filterEnvelope * envDepth * 10.f;

		float cutoffParam = params[CUTOFF_PARAM].getValue();
		float cutoffOffsetV = rescale(cutoffParam, 0.f, 1.f, -5.f, 5.f);
		float cutoffCVRaw = inputs[CUTOFFCV_INPUT].isConnected() ? inputs[CUTOFFCV_INPUT].getVoltage() : outputValue;
		float cutoffCV = cutoffCVRaw;
		float cutoffAtt = params[CUTOFFATT_PARAM].getValue() / 100.f;
		float cutoffControlV = std::clamp(cutoffOffsetV + (cutoffCV * cutoffAtt) + envelopeV, -5.f, 5.f);
		float cutoffNorm = rescale(cutoffControlV, -5.f, 5.f, 0.f, 1.f);
		float cutoffHz = 20.f * pow(350.f, cutoffNorm);
		float envelopeModHz = filterEnvelope * envDepth * 300.f;
		cutoffHz += envelopeModHz;
		cutoffHz = std::clamp(cutoffHz, 20.f, 7000.f);

		// Resonance
		float resonanceParam = params[RESONANCE_PARAM].getValue();
		float resonanceCV = inputs[RESONANCECV_INPUT].isConnected() ?
								clamp(inputs[RESONANCECV_INPUT].getVoltage() / 5.f, -1.f, 1.f) :
								outputValue;
		float resonanceAtt = params[RESONANCEATT_PARAM].getValue() / 100.f;
		float resonance = std::clamp(resonanceParam + resonanceCV * resonanceAtt, 0.f, 1.f);

		// Filter the output
		float filteredOutput = applyLowpassFilter(finalOutput, cutoffHz, resonance, args.sampleRate);

		float resonanceGain = 0.5f + resonance;
		float boostedOutput = ((filteredOutput * resonanceGain) * 2.f) * ampEnvelope;
		float outputVoltage = std::clamp(boostedOutput * 5.f, -10.f, 10.f);

		outputs[AUDIO_OUTPUT].setVoltage(outputVoltage);
	}
};

struct MonobassWidget : ModuleWidget {
	MonobassWidget(Monobass *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Monobass_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(16.358, 15.999)), module, Monobass::OCTAVE_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(39.359, 15.999)), module, Monobass::DETUNE_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(62.361, 15.999)), module, Monobass::FINETUNE_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(87.859, 15.999)), module, Monobass::CUTOFF_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(110.501, 15.999)), module, Monobass::ENVDEPTH_PARAM));
		addParam(
			createParamCentered<Davies1900hBlack>(mm2px(Vec(133.502, 15.999)), module, Monobass::FILTERDECAY_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(156.499, 15.999)), module, Monobass::AMPDECAY_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(16.358, 41.0)), module, Monobass::TIMBRE_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(39.359, 41.0)), module, Monobass::WAVESHAPE_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(62.361, 41.0)), module, Monobass::MIXER_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(87.859, 41.0)), module, Monobass::RESONANCE_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(110.501, 41.0)), module, Monobass::LFO_DEPTH_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(133.502, 41.0)), module, Monobass::LFO_SHAPE_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(156.499, 41.0)), module, Monobass::LFOFREQ_PARAM));

		addParam(createParamCentered<Knob9mm>(mm2px(Vec(33.599, 63.299)), module, Monobass::TIMBREATT_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(50.56, 63.299)), module, Monobass::WAVESHAPEATT_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(67.599, 63.299)), module, Monobass::FMATT_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(94.901, 63.299)), module, Monobass::CUTOFFATT_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(111.901, 63.299)), module, Monobass::FILTERDECAYATT_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(134.902, 63.289)), module, Monobass::LFOFREQATT_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(41.981, 79.001)), module, Monobass::DETUNEATT_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(58.984, 79.001)), module, Monobass::MIXERATT_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(86.395, 79.001)), module, Monobass::RESONANCEATT_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(103.364, 79.001)), module, Monobass::ENVDEPTHATT_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(120.403, 79.001)), module, Monobass::AMPDECAYATT_PARAM));

		addParam(createParam<Switch3PosHorizontal>(mm2px(Vec(9.5, 57.009)), module, Monobass::GATETRIG_PARAM));

		addParam(createParam<Switch2Pos>(mm2px(Vec(142.981, 74.789)), module, Monobass::LFO_RANGE_PARAM));
		addParam(createParam<Switch2Pos>(mm2px(Vec(155.998, 74.789)), module, Monobass::UNIPOLARBIPOLAR_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.005, 77.964)), module, Monobass::LFORESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.005, 95.501)), module, Monobass::GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.002, 95.501)), module, Monobass::DETUNECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(58.999, 95.501)), module, Monobass::MIXERCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(86.399, 95.501)), module, Monobass::RESONANCECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(103.399, 95.501)), module, Monobass::ENVDEPTHCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(120.4, 95.501)), module, Monobass::AMPDECAYCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.005, 111.002)), module, Monobass::VOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(33.602, 111.002)), module, Monobass::TIMBRECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(50.588, 111.002)), module, Monobass::WAVESHAPECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(67.592, 111.002)), module, Monobass::FMCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(94.897, 111.002)), module, Monobass::CUTOFFCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(111.901, 111.002)), module, Monobass::FILTERDECAYCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(134.902, 111.002)), module, Monobass::LFOCV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(158.009, 95.497)), module, Monobass::LFO_OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(157.999, 111.002)), module, Monobass::AUDIO_OUTPUT));

		addChild(
			createLightCentered<MediumLight<RedLight>>(mm2px(Vec(156.492, 54.998)), module, Monobass::LFOLEDRED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(
			mm2px(Vec(156.492, 54.998)), module, Monobass::LFOLEDGREEN_LIGHT));
	}
};

Model *modelMonobass = createModel<Monobass, MonobassWidget>("Monobass");
