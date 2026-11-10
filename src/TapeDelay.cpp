#include "plugin.hpp"
#include <cmath>

struct TapeDelay : Module {
	enum ParamId {
		TIME_PARAM,
		FEEDBACK_PARAM,
		DRYWET_PARAM,
		FILTER_PARAM,
		INSTABILITY_PARAM,
		WIDTH_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TIMECVIN_INPUT,
		FEEDBACKCVIN_INPUT,
		DRYWETCVIN_INPUT,
		FILTERCVIN_INPUT,
		INSTABILITYCVIN_INPUT,
		WIDTHCVIN_INPUT,
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
	// Tape-style dirt in the feedback loop: a touch of even-harmonic asymmetry (real
	// analog circuits are rarely perfectly symmetric), applied with the feedback soft
	// clip so it compounds on every repeat like grime building up on repeated copies.
	// The dry input is recorded clean -- it never shares a saturator with the repeats,
	// so new input can't crush what's already recirculating.
	static constexpr float tapeAsymmetry = 0.06f;
	// AC coupling in the feedback loop (like an analog delay's coupling caps): the
	// asymmetry generates DC, which would otherwise build up around the loop.
	static constexpr float feedbackDcBlockHz = 10.f;
	// Feedback saturation ceiling. This must be well above typical signal levels so
	// raising Feedback actually makes the repeats louder across most of the knob's
	// range -- tanh asymptotically approaches this value no matter how hard it's
	// driven, so too low a threshold makes the loop hit its ceiling almost immediately
	// instead of building up. Real overload/saturation still kicks in hard near the
	// top of the range (self-oscillation), just at a much higher, louder plateau.
	static constexpr float feedbackDriveThreshold = 6.f;
	// Basic white noise floor blended into the wet signal, always present (not tied
	// to feedback), like tape hiss. One shared draw per sample (not per poly channel)
	// for low CPU cost.
	static constexpr float wetNoiseAmount = 0.005f;
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

	// When Ext. Clock is patched, the Time knob (and its CV) instead selects one of
	// these musical divisions/multiplications of the incoming clock period.
	struct ClockDivision {
		float ratio;
		const char *label;
	};
	// Ratios are relative to one clock pulse = a quarter note. Grouped by note value
	// (fastest group first), each as [straight, triplet, dotted] -- the conventional
	// tempo-synced-delay menu order -- left-to-right from fastest (16th) to longest
	// (whole).
	static constexpr int numClockDivisions = 15;
	static constexpr ClockDivision clockDivisions[numClockDivisions] = {
		{0.25f, "1/16"},           {0.25f * 2.f / 3.f, "1/16T"}, {0.25f * 1.5f, "1/16D"},
		{0.5f, "1/8"},             {0.5f * 2.f / 3.f, "1/8T"},   {0.5f * 1.5f, "1/8D"},
		{1.f, "1/4"},              {1.f * 2.f / 3.f, "1/4T"},    {1.f * 1.5f, "1/4D"},
		{2.f, "1/2"},              {2.f * 2.f / 3.f, "1/2T"},    {2.f * 1.5f, "1/2D"},
		{4.f, "1/1"},              {4.f * 2.f / 3.f, "1/1T"},    {4.f * 1.5f, "1/1D"},
	};

	static int clockDivisionIndex(float norm) {
		return std::clamp((int)std::round(norm * (numClockDivisions - 1)), 0, numClockDivisions - 1);
	}

	// Time knob: free-running shows the same exponential ms mapping as Spatializer's
	// TimeParamQuantity; once Ext. Clock is patched it shows the selected division instead.
	struct TimeParamQuantity : ParamQuantity {
		std::string getDisplayValueString() override {
			float norm = getValue();

			TapeDelay *m = dynamic_cast<TapeDelay *>(module);
			if (m && m->inputs[EXTCLOCK_INPUT].isConnected()) {
				return clockDivisions[clockDivisionIndex(norm)].label;
			}

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

#ifdef METAMODULE
		// MetaModule's UI reads the raw name/unit/display* fields directly and does not
		// invoke a ParamQuantity's virtual overrides (same fix as DJFilter's cutoff), so
		// give these knobs native scaling instead of relying on getDisplayValueString().
		// Time: same exponential mapping as process(), minTimeMs * (max/min)^norm.
		// (When Ext. Clock is patched the knob picks a clock division instead; the
		// native display can't follow that, so it still shows the free-running ms value.)
		configParam(TIME_PARAM, 0.f, 1.f, 0.5f, "Time", "ms", maxTimeMs / minTimeMs, minTimeMs);
#else
		configParam<TimeParamQuantity>(TIME_PARAM, 0.f, 1.f, 0.5f, "Time");
#endif
		configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.f, "Feedback", "%", 0.f, 100.f);
		configParam(DRYWET_PARAM, 0.f, 1.f, 0.5f, "Dry/Wet", "%", 0.f, 100.f);
#ifdef METAMODULE
		// Filter: lowpass (-100%) -> bypass (0%, center) -> highpass (+100%), as DJFilter.
		configParam(FILTER_PARAM, 0.f, 1.f, 0.5f, "Filter", "%", 0.f, 200.f, -100.f);
#else
		configParam<TapeDelayFilterQuantity>(FILTER_PARAM, 0.f, 1.f, 0.5f, "");
#endif
		configParam(INSTABILITY_PARAM, 0.f, 1.f, 0.f, "Instability", "%", 0.f, 100.f);
		configParam(WIDTH_PARAM, 0.f, 1.f, 0.f, "Width", "%", 0.f, 100.f);

		configInput(TIMECVIN_INPUT, "Time CV");
		configInput(FEEDBACKCVIN_INPUT, "Feedback CV");
		configInput(DRYWETCVIN_INPUT, "Dry/Wet CV");
		configInput(FILTERCVIN_INPUT, "Filter CV");
		configInput(INSTABILITYCVIN_INPUT, "Instability CV");
		configInput(WIDTHCVIN_INPUT, "Width CV");
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

	// Two cross-fed tape lines per poly channel, like a stereo ping-pong tape echo:
	// line A records the (mono-summed) input plus line B's feedback; line B records
	// only line A's feedback. So repeats alternate A, B, A, B... on their own -- no
	// gain modulation (an earlier hard-pan flip-flop chopped sustained repeats and
	// clicked). A + B is the same signal a single mono loop would produce, so Width
	// crossfades from that centered sum (0) to A-left / B-right ping-pong (1).
	// Fixed-size (not std::vector) and sized for a 96kHz worst case, matching this
	// repo's convention (e.g. Spatializer's delayBufferL/R) -- MetaModule's real-time
	// callback must never allocate memory, so the buffer can't be grown at runtime.
	static constexpr float maxSupportedSampleRate = 96000.f;
	static const int maxDelaySamples =
		(int)((maxTimeMs + maxWowDepthMs + maxFlutterDepthMs + 10.f) * 0.001f * maxSupportedSampleRate) + 4;
	float bufferA[PORT_MAX_CHANNELS][maxDelaySamples] = {};
	float bufferB[PORT_MAX_CHANNELS][maxDelaySamples] = {};
	int bufferSize = maxDelaySamples;
	int writeIndex = 0;
	float lastSampleRate = 0.f;
	// High-water mark of poly channels that have been written to since the last clear.
	// On a sample rate change only these channels need wiping (the rest are still
	// zero), so the clear is ~1.6MB for a mono/stereo patch instead of the full
	// ~25MB -- which on MetaModule would otherwise stall the audio callback.
	int channelsUsed = 0;

	// Cached per-block-invariant values, recomputed only when their inputs change.
	float lastNormTime = -1.f;
	float cachedTimeMs = 0.f;

	SVF filtStateA[PORT_MAX_CHANNELS];
	SVF filtStateB[PORT_MAX_CHANNELS];

	// One-pole DC blocker state for each line's feedback path, per poly channel.
	struct DcBlock {
		float x1 = 0.f;
		float y1 = 0.f;
	};
	DcBlock fbDcStateA[PORT_MAX_CHANNELS];
	DcBlock fbDcStateB[PORT_MAX_CHANNELS];
	float dcBlockCoeff = 0.f;

	// Continuously interpolated random "value noise": glides between random keyframes
	// using smootherstep (zero 1st & 2nd derivative at each keyframe), so there's no
	// corner/step at the retarget points -- an organic, non-repeating wobble instead
	// of a clean sine, and instead of stepped sample-and-hold-style randomness.
	struct SmoothedNoise {
		float prevValue = 0.f;
		float nextValue = 0.f;
		float phase = 1.f; // forces an immediate re-target on the first step

		float step(float rateHz, float sampleTime) {
			phase += rateHz * sampleTime;
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

	// Ext. Clock tempo sync: measures the incoming clock period between rising edges.
	dsp::SchmittTrigger clockTrigger;
	float samplesSinceClockEdge = 0.f;
	float clockPeriodSamples = -1.f; // -1 until a full period has been measured
	bool sawFirstClockEdge = false;

	// Fixed filter character: a small, permanent amount of resonance (Q = 0.85, just
	// above DJFilter's flat minimum of 0.707) for a touch of life without honking, plus
	// a single 6db/Oct stage (matching DJFilter's SLOPE_PARAM = 1).
	const float filterDamp = 1.f / 0.85f;

	float lastMorph = -1.f;
	float lastFilterFreqSR = 0.f;
	float filterFreq = 0.f;

	float smoothedDelaySamples = -1.f;

	float getNormalizedParam(int paramId, int inputId) {
		float paramValue = params[paramId].getValue();
		float inputCV = inputs[inputId].isConnected() ? std::clamp(inputs[inputId].getVoltage(), -5.f, 5.f) / 10.f : 0.f;
		return std::clamp(paramValue + inputCV, 0.f, 1.f);
	}

	// Read position for a fractional delay, shared by every poly channel (they all use
	// the same delay time), so it's computed once per sample instead of per channel.
	// Integer and fractional parts are split *before* subtracting from writeIndex: a
	// single float readPos near the top of the ~197k-sample buffer only has ~1/64 sample
	// resolution, which quantizes the interpolation and adds zipper noise under wow/flutter.
	struct ReadTap {
		int idx0;
		int idx1;
		float frac;
	};

	static ReadTap makeReadTap(int bufSize, int writeIdx, float delaySamples) {
		delaySamples = std::clamp(delaySamples, 1.f, (float)(bufSize - 2));
		const int whole = (int)delaySamples;
		const float fracPart = delaySamples - whole;
		// readPos = (writeIdx - whole - 1) + (1 - fracPart)
		int idx0 = writeIdx - whole - 1;
		if (idx0 < 0)
			idx0 += bufSize;
		int idx1 = idx0 + 1;
		if (idx1 >= bufSize)
			idx1 -= bufSize;
		return {idx0, idx1, 1.f - fracPart};
	}

	static float readDelay(const float *buf, const ReadTap &tap) {
		return buf[tap.idx0] + (buf[tap.idx1] - buf[tap.idx0]) * tap.frac;
	}

	// Pade [7/6] rational approximation of tanh: one divide instead of a libm tanh
	// call (which is slow on MetaModule's Cortex-A7). Error < 2e-6 for |x| < 3 and the
	// curve meets +/-1 smoothly at the clamp point, so it's inaudible as a saturator.
	static float fastTanh(float x) {
		constexpr float clampX = 4.97f;
		if (x > clampX)
			return 1.f;
		if (x < -clampX)
			return -1.f;
		const float x2 = x * x;
		const float num = x * (135135.f + x2 * (17325.f + x2 * (378.f + x2)));
		const float den = 135135.f + x2 * (62370.f + x2 * (3150.f + x2 * 28.f));
		return num / den;
	}

	static float softClip(float x, float threshold, float invThreshold) {
		return threshold * fastTanh(x * invThreshold);
	}

	static float applyFeedbackFilter(SVF &f, float damp, float freq, float delayed, float lowMix, float bypassMix,
	                                  float highMix) {
		f.high = delayed - f.low - damp * f.band;
		f.band += freq * f.high;
		f.low += freq * f.band;
		return f.low * lowMix + delayed * bypassMix + f.high * highMix;
	}

	// Feedback path for one line: gain -> soft clip -> tape dirt -> AC coupling.
	// Feedback is allowed above unity (self-oscillation); the soft clip lets it
	// overload/saturate like an overdriven tape loop instead of diverging. Asymmetry is
	// applied to the already-clipped (bounded) signal, and DC is blocked last, so
	// nothing that re-enters the loop carries an offset.
	float feedbackStage(float regen, float gain, float invDrive, DcBlock &dc) const {
		const float sat = softClip(regen * gain, feedbackDriveThreshold, invDrive);
		const float colored = sat + tapeAsymmetry * sat * sat;
		const float out = colored - dc.x1 + dcBlockCoeff * dc.y1;
		dc.x1 = colored;
		dc.y1 = out;
		return out;
	}

	void process(const ProcessArgs &args) override {
		const float sampleRate = args.sampleRate;
		const float sampleTime = args.sampleTime;

		// --- Clear delay buffers/filter state on sample rate change (fixed-size array,
		// no reallocation -- just wipes stale content so old-rate audio doesn't play
		// back at the wrong pitch). Only channels that have actually been written are
		// cleared; on the very first call nothing has been written yet, so it's free. ---
		if (sampleRate != lastSampleRate) {
			lastSampleRate = sampleRate;
			for (int c = 0; c < channelsUsed; c++) {
				std::fill_n(bufferA[c], maxDelaySamples, 0.f);
				std::fill_n(bufferB[c], maxDelaySamples, 0.f);
			}
			for (int c = 0; c < PORT_MAX_CHANNELS; c++) {
				filtStateA[c] = SVF();
				filtStateB[c] = SVF();
				fbDcStateA[c] = DcBlock();
				fbDcStateB[c] = DcBlock();
			}
			dcBlockCoeff = 1.f - 2.f * float(M_PI) * feedbackDcBlockHz * sampleTime;
			channelsUsed = 0;
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
		channelsUsed = std::max(channelsUsed, n);

		outputs[AUDIOLEFTOUT_OUTPUT].setChannels(n);
		outputs[AUDIORIGHTOUT_OUTPUT].setChannels(n);

		// --- Mono controls (shared across all poly channels) ---
		const float normTime = getNormalizedParam(TIME_PARAM, TIMECVIN_INPUT);

		// --- Ext. Clock tempo sync: measure the period between rising edges ---
		const bool clockConnected = inputs[EXTCLOCK_INPUT].isConnected();
		if (clockConnected) {
			if (clockTrigger.process(inputs[EXTCLOCK_INPUT].getVoltage())) {
				if (sawFirstClockEdge)
					clockPeriodSamples = samplesSinceClockEdge;
				sawFirstClockEdge = true;
				samplesSinceClockEdge = 0.f;
			} else {
				samplesSinceClockEdge += 1.f;
			}
		} else {
			clockTrigger.reset();
			samplesSinceClockEdge = 0.f;
			sawFirstClockEdge = false;
			clockPeriodSamples = -1.f;
		}

		// Free-running: Time knob/CV picks an absolute ms value. Clock-synced: the same
		// knob/CV instead picks a musical division/multiplication of the clock period.
		float delayBaseSamples;
		if (clockConnected && clockPeriodSamples > 0.f) {
			delayBaseSamples = clockPeriodSamples * clockDivisions[clockDivisionIndex(normTime)].ratio;
		} else {
			// std::pow is expensive on MetaModule; only recompute when the knob/CV moves.
			if (normTime != lastNormTime) {
				lastNormTime = normTime;
				cachedTimeMs = minTimeMs * std::pow(maxTimeMs / minTimeMs, normTime);
			}
			delayBaseSamples = cachedTimeMs * 0.001f * sampleRate;
		}

		const float normFeedback = getNormalizedParam(FEEDBACK_PARAM, FEEDBACKCVIN_INPUT);
		const float feedbackGain = normFeedback * maxFeedbackGain;

		// Reverse exponential taper: knob/CV response ramps in fast at first, then
		// flattens out approaching full wet, instead of a flat linear response.
		const float dryWetLinear = getNormalizedParam(DRYWET_PARAM, DRYWETCVIN_INPUT);
		const float dryWet = 1.f - (1.f - dryWetLinear) * (1.f - dryWetLinear);
		const float normInstability = getNormalizedParam(INSTABILITY_PARAM, INSTABILITYCVIN_INPUT);
		const float widthAmount = getNormalizedParam(WIDTH_PARAM, WIDTHCVIN_INPUT);

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
		const float wowValue = wowNoise.step(wowRateHz, sampleTime);
		const float flutterValue = flutterNoise.step(flutterRateHz, sampleTime);

		const float wobbleMs = instabilityShaped * (maxWowDepthMs * wowValue + maxFlutterDepthMs * flutterValue);
		const float delaySamplesTarget =
			std::clamp(delayBaseSamples + wobbleMs * 0.001f * sampleRate, 1.f, (float)(bufferSize - 2));

		const bool tempoSynced = clockConnected && clockPeriodSamples > 0.f;
		if (smoothedDelaySamples < 0.f || tempoSynced) {
			// Tempo sync: snap straight to the target so division/tempo changes stay
			// locked to the beat instead of gliding/repitching like free-running mode.
			smoothedDelaySamples = delaySamplesTarget;
		} else {
			// Free-running: glide the delay time like tape speeding up/slowing down,
			// not an instant jump. Lengthening is capped to avoid the read pointer ever
			// reversing (which clicks); shortening can never reverse, so it glides faster.
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

		// Basic white noise floor for the wet signal: one draw per sample, shared
		// across all poly channels (not per-channel) to keep it cheap.
		const float wetNoise = (random::uniform() * 2.f - 1.f) * wetNoiseAmount;

		// Same delay time for every poly channel, so resolve the read indices once.
		const ReadTap tap = makeReadTap(bufferSize, writeIndex, delaySamples);

		constexpr float invFeedbackDrive = 1.f / feedbackDriveThreshold;
		constexpr float outputClip = 10.f;
		constexpr float invOutputClip = 1.f / outputClip;

		// --- Per poly channel ---
		for (int c = 0; c < n; c++) {
			const float inL = lConnected ? inputs[AUDIOLEFTIN_INPUT].getPolyVoltage(c) :
			                                (rConnected ? inputs[AUDIORIGHTIN_INPUT].getPolyVoltage(c) : 0.f);
			const float inR = rConnected ? inputs[AUDIORIGHTIN_INPUT].getPolyVoltage(c) :
			                                (lConnected ? inputs[AUDIOLEFTIN_INPUT].getPolyVoltage(c) : 0.f);

			// The input is summed to mono and recorded onto line A only.
			const float monoIn = (inL + inR) * 0.5f;

			// Filter shapes the whole wet signal -- both the wet output tap and what
			// gets fed back -- so every repeat sounds filtered, not just the ones that
			// have been through the feedback loop.
			const float regenA = applyFeedbackFilter(filtStateA[c], filterDamp, filterFreq, readDelay(bufferA[c], tap),
			                                         lowMix, bypassMix, highMix);
			const float regenB = applyFeedbackFilter(filtStateB[c], filterDamp, filterFreq, readDelay(bufferB[c], tap),
			                                         lowMix, bypassMix, highMix);

			// Cross-feed: each line's repeat is recorded onto the other line.
			const float fbIntoB = feedbackStage(regenA, feedbackGain, invFeedbackDrive, fbDcStateA[c]);
			const float fbIntoA = feedbackStage(regenB, feedbackGain, invFeedbackDrive, fbDcStateB[c]);

			// Record heads: clean input + B's feedback onto A; A's feedback onto B.
			bufferA[c][writeIndex] = monoIn + fbIntoA;
			bufferB[c][writeIndex] = fbIntoB;

			// Width crossfades from centered (both outputs get A + B, i.e. every repeat)
			// to ping-pong (A's repeats left, B's repeats right). Plain constant gains --
			// nothing is modulated over time, so there's nothing to click.
			const float crossAmount = 1.f - widthAmount;
			const float wetL = regenA + crossAmount * regenB + wetNoise;
			const float wetR = regenB + crossAmount * regenA + wetNoise;

			const float outL = rack::math::crossfade(inL, wetL, dryWet);
			const float outR = rack::math::crossfade(inR, wetR, dryWet);

			outputs[AUDIOLEFTOUT_OUTPUT].setVoltage(softClip(outL, outputClip, invOutputClip), c);
			outputs[AUDIORIGHTOUT_OUTPUT].setVoltage(softClip(outR, outputClip, invOutputClip), c);
		}

		if (++writeIndex >= bufferSize)
			writeIndex = 0;
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
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(13.406, 94.998)), module, TapeDelay::WIDTH_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(13.406, 41.751)), module, TapeDelay::TIMECVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.166, 41.751)), module, TapeDelay::FEEDBACKCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(13.406, 77.9996)), module, TapeDelay::FILTERCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.286, 77.9996)), module, TapeDelay::DRYWETCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.166, 77.9996)), module, TapeDelay::INSTABILITYCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.286, 94.998)), module, TapeDelay::WIDTHCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.166, 95.000)), module, TapeDelay::EXTCLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.043, 111.002)), module, TapeDelay::AUDIOLEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.839, 111.002)), module, TapeDelay::AUDIORIGHTIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37.831, 111.002)), module, TapeDelay::AUDIOLEFTOUT_OUTPUT));
		addOutput(
			createOutputCentered<PJ301MPort>(mm2px(Vec(52.627, 111.002)), module, TapeDelay::AUDIORIGHTOUT_OUTPUT));
	}
};

Model *modelTapeDelay = createModel<TapeDelay, TapeDelayWidget>("TapeDelay");
