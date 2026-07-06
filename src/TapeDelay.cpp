#include "plugin.hpp"
#include <cmath>
#include <vector>

struct TapeDelay : Module {
	enum ParamId {
		TIME_PARAM,
		FEEDBACK_PARAM,
		DRYWET_PARAM,
		FILTER_PARAM,
		INSTABILITY_PARAM,
		PINGPONG_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TIMECVIN_INPUT,
		FEEDBACKCVIN_INPUT,
		DRYWETCVIN_INPUT,
		FILTERCVIN_INPUT,
		INSTABILITYCVIN_INPUT,
		EXTCLOCK_INPUT,
		AUDIOLEFTIN_INPUT,
		AUDIORIGHTIN_INPUT,
		INPUTS_LEN
	};
	enum OutputId { AUDIOLEFTOUT_OUTPUT, AUDIORIGHTOUT_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	static constexpr float minTimeMs = 20.f;
	static constexpr float maxTimeMs = 2000.f;
	static constexpr float maxWowDepthMs = 8.f;
	static constexpr float maxFlutterDepthMs = 2.f;
	static constexpr float wowRateMinHz = 0.15f;
	static constexpr float wowRateMaxHz = 2.5f;
	static constexpr float flutterRateMinHz = 3.f;
	static constexpr float flutterRateMaxHz = 40.f;
	static constexpr float instabilityFadeZone = 0.05f;
	static constexpr float maxFeedbackGain = 1.15f;
	// The read pointer's effective playback speed is (1 - rate the delay time is
	// changing per sample). Bounding the *speed ratio* (not the raw rate) is what keeps
	// this feeling like tape continuously repitching -- never literally stopping or
	// reversing -- instead of an instant jump or a near-frozen crawl.
	// Lengthening (slowing down): speed must stay > 0 (a real tape can slow down but
	// never runs backward just from a time change), and capping it well above 0 keeps
	// the buffer audibly playing instead of nearly freezing on a jump.
	static constexpr float minTapeSpeedRatio = 0.4f;
	// Shortening (speeding up) has no such floor -- it only ever moves further forward
	// -- so it can wind up much faster while still sounding like a glide, not a jump.
	static constexpr float maxTapeSpeedRatio = 6.f;
	static constexpr float delayGlideMaxIncreasePerSample = 1.f - minTapeSpeedRatio;
	static constexpr float delayGlideMaxDecreasePerSample = maxTapeSpeedRatio - 1.f;
	static constexpr float filterLowpassMinHz = 500.f;
	static constexpr float filterHighpassMaxHz = 1000.f;
	// Small linear crossfade so the ping-pong hard-pan flip doesn't click.
	static constexpr float pingPongCrossfadeSec = 0.01f;

	// Time knob: same exponential ms mapping/display pattern as Spatializer's TimeParamQuantity.
	struct TimeParamQuantity : ParamQuantity {
		std::string getDisplayValueString() override {
			float norm = getValue();
			float timeMs = minTimeMs * std::pow(maxTimeMs / minTimeMs, norm);
			return string::f("%.2fms", timeMs);
		}
	};

	// Filter knob: identical morph (lowpass -> bypass -> highpass) mapping and tooltip
	// as DJFilter's CUTOFF_PARAM, with resonance and slope permanently fixed (see process()).
	struct TapeDelayFilterQuantity : ParamQuantity {
		std::string getDisplayValueString() override {
			float morph = getValue(); // knob value only, no CV

			const float fadeWidth = 0.02f;
			const float dryStart = 0.45f;
			const float dryEnd = 0.55f;

			if (morph >= dryStart + fadeWidth && morph <= dryEnd - fadeWidth) {
				return "Cutoff: BYPASS";
			}

			float cutoffHz;
			if (morph < 0.5f) {
				float t = morph * 2.f;
				cutoffHz = std::pow(2.f, rescale(t, 0.f, 1.f, std::log2(filterLowpassMinHz), std::log2(7000.f)));
			} else {
				float t = (morph - 0.5f) * 2.f;
				cutoffHz = std::pow(2.f, rescale(t, 0.f, 1.f, std::log2(300.f), std::log2(filterHighpassMaxHz)));
			}

			float sampleRate = APP->engine->getSampleRate();
			float safeCutoff = std::min(cutoffHz, 0.5f * sampleRate * 0.49f);
			float f = 2.f * std::sin(M_PI * safeCutoff / sampleRate);
			f = std::min(f, 0.95f); // single (6db) stage stability clamp

			float actualHz = (sampleRate / float(M_PI)) * std::asin(f / 2.f);

			return string::f("Cutoff: %.2fhz", actualHz);
		}

		std::string getLabel() override {
			return ""; // prevents the "#1:" from showing
		}

		void reset() override {
			setValue(0.5f);
		}
	};

	TapeDelay() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam<TimeParamQuantity>(TIME_PARAM, 0.f, 1.f, 0.5f, "Time");
		configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.f, "Feedback", "%", 0.f, 100.f);
		configParam(DRYWET_PARAM, 0.f, 1.f, 0.5f, "Dry/Wet", "%", 0.f, 100.f);
		configParam<TapeDelayFilterQuantity>(FILTER_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(INSTABILITY_PARAM, 0.f, 1.f, 0.f, "Instability", "%", 0.f, 100.f);
		configSwitch(PINGPONG_PARAM, 0.f, 1.f, 0.f, "Ping Pong", {"Normal", "Ping Pong"});

		configInput(TIMECVIN_INPUT, "Time CV");
		configInput(FEEDBACKCVIN_INPUT, "Feedback CV");
		configInput(DRYWETCVIN_INPUT, "Dry/Wet CV");
		configInput(FILTERCVIN_INPUT, "Filter CV");
		configInput(INSTABILITYCVIN_INPUT, "Instability CV");
		configInput(EXTCLOCK_INPUT, "Ext. Clock");
		configInput(AUDIOLEFTIN_INPUT, "Audio Left");
		configInput(AUDIORIGHTIN_INPUT, "Audio Right");

		configOutput(AUDIOLEFTOUT_OUTPUT, "Audio Left");
		configOutput(AUDIORIGHTOUT_OUTPUT, "Audio Right");

		configBypass(AUDIOLEFTIN_INPUT, AUDIOLEFTOUT_OUTPUT);
		configBypass(AUDIORIGHTIN_INPUT, AUDIORIGHTOUT_OUTPUT);
	}

	// Single-stage (6db/Oct) state-variable filter, matching DJFilter's SVF core.
	struct SVF {
		float low = 0.f;
		float band = 0.f;
		float high = 0.f;
	};

	std::vector<float> bufferL[PORT_MAX_CHANNELS];
	std::vector<float> bufferR[PORT_MAX_CHANNELS];
	int bufferSize = 0;
	int writeIndex = 0;
	float lastSampleRate = 0.f;

	SVF filtStateL[PORT_MAX_CHANNELS];
	SVF filtStateR[PORT_MAX_CHANNELS];

	// Continuously interpolated random "value noise": glides between random keyframes
	// using smootherstep (zero 1st & 2nd derivative at each keyframe), so there's no
	// corner/step at the retarget points -- an organic, non-repeating wobble instead
	// of a clean sine, and instead of stepped sample-and-hold-style randomness.
	struct SmoothedNoise {
		float prevValue = 0.f;
		float nextValue = 0.f;
		float phase = 1.f; // forces an immediate re-target on the first step

		float step(float rateHz, float sampleRate) {
			phase += rateHz / sampleRate;
			if (phase >= 1.f) {
				phase -= 1.f;
				prevValue = nextValue;
				nextValue = random::uniform() * 2.f - 1.f;
			}
			float t = phase;
			float smooth = t * t * t * (t * (t * 6.f - 15.f) + 10.f);
			return prevValue + (nextValue - prevValue) * smooth;
		}
	};

	SmoothedNoise wowNoise;
	SmoothedNoise flutterNoise;

	// Fixed filter character: resonance permanently at minimum (Q = 0.707, matching
	// DJFilter's RESONANCE_PARAM = 0), single 6db/Oct stage (DJFilter's SLOPE_PARAM = 1).
	const float filterDamp = 1.f / 0.707f;

	float lastMorph = -1.f;
	float lastFilterFreqSR = 0.f;
	float filterFreq = 0.f;

	float smoothedDelaySamples = -1.f;

	// Ping-pong: a single trunk delay line per poly channel, panned alternately
	// left/right once per delay cycle, so successive repeats bounce between speakers.
	bool pingPongSide = false;
	float pingPongElapsed = 0.f;
	float pingPongPan = 0.f; // 0 = left, 1 = right, crossfades between on each flip

	float getNormalizedParam(int paramId, int inputId) {
		float paramValue = params[paramId].getValue();
		float inputCV = inputs[inputId].isConnected() ? std::clamp(inputs[inputId].getVoltage(), -5.f, 5.f) / 10.f : 0.f;
		return std::clamp(paramValue + inputCV, 0.f, 1.f);
	}

	static float readDelay(const std::vector<float> &buf, int bufSize, int writeIdx, float delaySamples) {
		delaySamples = std::clamp(delaySamples, 0.f, (float)(bufSize - 2));
		float readPos = writeIdx - delaySamples;
		while (readPos < 0.f)
			readPos += bufSize;

		int idx0 = (int)readPos;
		float frac = readPos - idx0;
		int idx1 = (idx0 + 1) % bufSize;
		return buf[idx0] * (1.f - frac) + buf[idx1] * frac;
	}

	static float softClip(float x, float threshold) {
		return threshold * std::tanh(x / threshold);
	}

	static float applyFeedbackFilter(SVF &f, float damp, float freq, float delayed, float lowMix, float bypassMix,
	                                  float highMix) {
		f.high = delayed - f.low - damp * f.band;
		f.band += freq * f.high;
		f.low += freq * f.band;
		return f.low * lowMix + delayed * bypassMix + f.high * highMix;
	}

	void process(const ProcessArgs &args) override {
		const float sampleRate = args.sampleRate;

		// --- (Re)allocate delay buffers to fit the max possible modulated delay time ---
		if (sampleRate != lastSampleRate) {
			lastSampleRate = sampleRate;
			bufferSize = (int)((maxTimeMs + maxWowDepthMs + maxFlutterDepthMs + 10.f) * 0.001f * sampleRate) + 4;
			for (int c = 0; c < PORT_MAX_CHANNELS; c++) {
				bufferL[c].assign(bufferSize, 0.f);
				bufferR[c].assign(bufferSize, 0.f);
				filtStateL[c] = SVF();
				filtStateR[c] = SVF();
			}
			writeIndex = 0;
			smoothedDelaySamples = -1.f;
		}

		// --- Channel counts: each side normalizes from the other when disconnected ---
		const bool lConnected = inputs[AUDIOLEFTIN_INPUT].isConnected();
		const bool rConnected = inputs[AUDIORIGHTIN_INPUT].isConnected();
		const int nL = lConnected ? inputs[AUDIOLEFTIN_INPUT].getChannels() :
		                            (rConnected ? inputs[AUDIORIGHTIN_INPUT].getChannels() : 0);
		const int nR = rConnected ? inputs[AUDIORIGHTIN_INPUT].getChannels() :
		                            (lConnected ? inputs[AUDIOLEFTIN_INPUT].getChannels() : 0);
		const int n = std::max(nL, nR);

		outputs[AUDIOLEFTOUT_OUTPUT].setChannels(n);
		outputs[AUDIORIGHTOUT_OUTPUT].setChannels(n);

		// --- Mono controls (shared across all poly channels) ---
		const float normTime = getNormalizedParam(TIME_PARAM, TIMECVIN_INPUT);
		const float timeMs = minTimeMs * std::pow(maxTimeMs / minTimeMs, normTime);

		const float normFeedback = getNormalizedParam(FEEDBACK_PARAM, FEEDBACKCVIN_INPUT);
		const float feedbackGain = normFeedback * maxFeedbackGain;

		const float dryWet = getNormalizedParam(DRYWET_PARAM, DRYWETCVIN_INPUT);
		const float normInstability = getNormalizedParam(INSTABILITY_PARAM, INSTABILITYCVIN_INPUT);
		const bool pingPong = params[PINGPONG_PARAM].getValue() > 0.5f;

		// Totally off at 0, then smoothstep-ease in over the first 5% of travel so it
		// doesn't switch on abruptly; beyond that it continues the normal linear scaling.
		float instabilityShaped;
		if (normInstability <= 0.f) {
			instabilityShaped = 0.f;
		} else if (normInstability < instabilityFadeZone) {
			float t = normInstability / instabilityFadeZone;
			instabilityShaped = t * t * (3.f - 2.f * t) * instabilityFadeZone;
		} else {
			instabilityShaped = normInstability;
		}

		// --- Wow & flutter: organic random wander (not a clean sine), shared/mono.
		// Both depth and rate grow with the Instability knob, like a more worn-out tape transport.
		const float wowRateHz = rescale(instabilityShaped, 0.f, 1.f, wowRateMinHz, wowRateMaxHz);
		const float flutterRateHz = rescale(instabilityShaped, 0.f, 1.f, flutterRateMinHz, flutterRateMaxHz);
		const float wowValue = wowNoise.step(wowRateHz, sampleRate);
		const float flutterValue = flutterNoise.step(flutterRateHz, sampleRate);

		const float wobbleMs = instabilityShaped * (maxWowDepthMs * wowValue + maxFlutterDepthMs * flutterValue);
		const float delaySamplesTarget =
			std::clamp((timeMs + wobbleMs) * 0.001f * sampleRate, 1.f, (float)(bufferSize - 2));

		// Glide the delay time like tape speeding up/slowing down, not an instant jump:
		// lengthening is capped to avoid the read pointer ever reversing (which clicks),
		// while shortening -- which can never reverse -- is allowed to glide much faster.
		if (smoothedDelaySamples < 0.f) {
			smoothedDelaySamples = delaySamplesTarget;
		} else {
			float diff = delaySamplesTarget - smoothedDelaySamples;
			diff = diff > 0.f ? std::min(diff, delayGlideMaxIncreasePerSample) :
			                     std::max(diff, -delayGlideMaxDecreasePerSample);
			smoothedDelaySamples += diff;
		}
		const float delaySamples = smoothedDelaySamples;

		// --- Filter coefficient (mirrors DJFilter's cutoff morph mapping) ---
		const float filterVal = params[FILTER_PARAM].getValue();
		const float filterCV = inputs[FILTERCVIN_INPUT].isConnected() ? inputs[FILTERCVIN_INPUT].getVoltage() * 0.1f : 0.f;
		const float morph = std::clamp(filterVal + filterCV, 0.f, 1.f);

		if (morph != lastMorph || sampleRate != lastFilterFreqSR) {
			lastMorph = morph;
			lastFilterFreqSR = sampleRate;

			float cutoffHz;
			if (morph < 0.5f) {
				float t = morph * 2.f;
				cutoffHz = std::pow(2.f, rescale(t, 0.f, 1.f, std::log2(filterLowpassMinHz), std::log2(7000.f)));
			} else {
				float t = (morph - 0.5f) * 2.f;
				cutoffHz = std::pow(2.f, rescale(t, 0.f, 1.f, std::log2(300.f), std::log2(filterHighpassMaxHz)));
			}

			float safeCutoff = std::min(cutoffHz, 0.5f * sampleRate * 0.49f);
			filterFreq = 2.f * std::sin(M_PI * safeCutoff / sampleRate);
			filterFreq = std::min(filterFreq, 0.95f); // single-stage stability clamp
		}

		const float fadeWidth = 0.02f;
		const float dryStart = 0.45f;
		const float dryEnd = 0.55f;

		float lowMix = 0.f, bypassMix = 0.f, highMix = 0.f;
		if (morph <= dryStart - fadeWidth) {
			lowMix = 1.f;
		} else if (morph < dryStart + fadeWidth) {
			float t = (morph - (dryStart - fadeWidth)) / (2.f * fadeWidth);
			lowMix = 1.f - t;
			bypassMix = t;
		} else if (morph < dryEnd - fadeWidth) {
			bypassMix = 1.f;
		} else if (morph < dryEnd + fadeWidth) {
			float t = (morph - (dryEnd - fadeWidth)) / (2.f * fadeWidth);
			bypassMix = 1.f - t;
			highMix = t;
		} else {
			highMix = 1.f;
		}

		// --- Ping-pong flip-flop: toggles once per delay cycle so repeats hard-pan
		// to the opposite speaker every other delay, instead of blending both sides.
		if (pingPong) {
			pingPongElapsed += 1.f;
			if (pingPongElapsed >= delaySamples) {
				pingPongElapsed -= delaySamples;
				pingPongSide = !pingPongSide;
			}
		} else {
			pingPongElapsed = 0.f;
		}

		// Crossfade the pan position toward the current side instead of switching
		// instantly, so the flip doesn't click.
		const float pingPongPanTarget = pingPongSide ? 1.f : 0.f;
		const float maxPanStep = 1.f / (pingPongCrossfadeSec * sampleRate);
		pingPongPan += std::clamp(pingPongPanTarget - pingPongPan, -maxPanStep, maxPanStep);

		// --- Per poly channel ---
		for (int c = 0; c < n; c++) {
			const float inL = lConnected ? inputs[AUDIOLEFTIN_INPUT].getPolyVoltage(c) :
			                                (rConnected ? inputs[AUDIORIGHTIN_INPUT].getPolyVoltage(c) : 0.f);
			const float inR = rConnected ? inputs[AUDIORIGHTIN_INPUT].getPolyVoltage(c) :
			                                (lConnected ? inputs[AUDIOLEFTIN_INPUT].getPolyVoltage(c) : 0.f);

			float outL, outR;

			if (pingPong) {
				// Single mono trunk line (reuses bufferL/filtStateL): the tap hard-pans to
				// one speaker and flips to the other every delay cycle (see flip-flop above).
				const float monoIn = (inL + inR) * 0.5f;
				const float delayedTrunk = readDelay(bufferL[c], bufferSize, writeIndex, delaySamples);

				// Filter lives only in the feedback/regeneration path: the tapped delay
				// signal used for the wet output is left unfiltered, so repeats
				// darken/brighten progressively as they recirculate.
				const float regen =
					applyFeedbackFilter(filtStateL[c], filterDamp, filterFreq, delayedTrunk, lowMix, bypassMix, highMix);

				// Feedback is allowed above unity (self-oscillation); soft-clipping at +/-5V
				// lets it overload/saturate like an overdriven tape loop instead of diverging.
				const float fb = softClip(regen * feedbackGain, 5.f);
				bufferL[c][writeIndex] = monoIn + fb;

				const float wetL = delayedTrunk * (1.f - pingPongPan);
				const float wetR = delayedTrunk * pingPongPan;

				outL = rack::math::crossfade(inL, wetL, dryWet);
				outR = rack::math::crossfade(inR, wetR, dryWet);
			} else {
				const float delayedL = readDelay(bufferL[c], bufferSize, writeIndex, delaySamples);
				const float delayedR = readDelay(bufferR[c], bufferSize, writeIndex, delaySamples);

				const float regenL =
					applyFeedbackFilter(filtStateL[c], filterDamp, filterFreq, delayedL, lowMix, bypassMix, highMix);
				const float regenR =
					applyFeedbackFilter(filtStateR[c], filterDamp, filterFreq, delayedR, lowMix, bypassMix, highMix);

				const float fbL = softClip(regenL * feedbackGain, 5.f);
				const float fbR = softClip(regenR * feedbackGain, 5.f);

				bufferL[c][writeIndex] = inL + fbL;
				bufferR[c][writeIndex] = inR + fbR;

				outL = rack::math::crossfade(inL, delayedL, dryWet);
				outR = rack::math::crossfade(inR, delayedR, dryWet);
			}

			outputs[AUDIOLEFTOUT_OUTPUT].setVoltage(softClip(outL, 10.f), c);
			outputs[AUDIORIGHTOUT_OUTPUT].setVoltage(softClip(outR, 10.f), c);
		}

		writeIndex = (writeIndex + 1) % bufferSize;
	}
};

struct TapeDelayWidget : ModuleWidget {
	TapeDelayWidget(TapeDelay *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/TapeDelay.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies_large>(mm2px(Vec(14.397, 18.5)), module, TapeDelay::TIME_PARAM));
		addParam(createParamCentered<Davies_large>(mm2px(Vec(46.172, 18.5)), module, TapeDelay::FEEDBACK_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(30.286, 41.751)), module, TapeDelay::DRYWET_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(13.406, 58.999)), module, TapeDelay::FILTER_PARAM));
		addParam(
			createParamCentered<Davies1900hBlack>(mm2px(Vec(47.166, 58.999)), module, TapeDelay::INSTABILITY_PARAM));
		addParam(createParamCentered<Switch2Pos>(mm2px(Vec(13.406, 94.998)), module, TapeDelay::PINGPONG_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(13.406, 41.751)), module, TapeDelay::TIMECVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.166, 41.751)), module, TapeDelay::FEEDBACKCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(13.406, 77.9996)), module, TapeDelay::FILTERCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.286, 77.9996)), module, TapeDelay::DRYWETCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.166, 77.9996)), module, TapeDelay::INSTABILITYCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.166, 95.000)), module, TapeDelay::EXTCLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.043, 111.002)), module, TapeDelay::AUDIOLEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.839, 111.002)), module, TapeDelay::AUDIORIGHTIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37.831, 111.002)), module, TapeDelay::AUDIOLEFTOUT_OUTPUT));
		addOutput(
			createOutputCentered<PJ301MPort>(mm2px(Vec(52.627, 111.002)), module, TapeDelay::AUDIORIGHTOUT_OUTPUT));
	}
};

Model *modelTapeDelay = createModel<TapeDelay, TapeDelayWidget>("TapeDelay");
