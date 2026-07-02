#include "plugin.hpp"
#include "helpers/math_lut.hpp"
#include <cmath>

// Per-channel filter state. Coefficients are identical across poly channels
// (cutoff/resonance/type are mono), so they live in the shared Biquad below
// while each channel keeps its own delay memory here.
struct BiquadState {
	float x1 = 0.f, x2 = 0.f;
	float y1 = 0.f, y2 = 0.f;
};

struct Biquad {
	float b0 = 1.f, b1 = 0.f, b2 = 0.f;
	float a1 = 0.f, a2 = 0.f;

	void setupLowpass(float cutoff, float resonance, float sampleRate) {
		float w0 = 2.f * M_PI * cutoff / sampleRate;
		float Q = resonance * 5.f + 0.1f;
		float alpha = SinEQ(w0) / (2.f * Q);
		float cosw0 = CosEQ(w0);

		b0 = (1.f - cosw0) * 0.5f;
		b1 = 1.f - cosw0;
		b2 = b0;
		float a0 = 1.f + alpha;
		a1 = -2.f * cosw0;
		a2 = 1.f - alpha;

		b0 /= a0;
		b1 /= a0;
		b2 /= a0;
		a1 /= a0;
		a2 /= a0;
	}

	void setupHighpass(float cutoff, float resonance, float sampleRate) {
		float w0 = 2.f * M_PI * cutoff / sampleRate;
		float Q = resonance * 5.f + 0.1f;
		float alpha = SinEQ(w0) / (2.f * Q);
		float cosw0 = CosEQ(w0);

		b0 = (1.f + cosw0) * 0.5f;
		b1 = -(1.f + cosw0);
		b2 = b0;
		float a0 = 1.f + alpha;
		a1 = -2.f * cosw0;
		a2 = 1.f - alpha;

		b0 /= a0;
		b1 /= a0;
		b2 /= a0;
		a1 /= a0;
		a2 /= a0;
	}

	float processChannel(float in, BiquadState &s) const {
		float out = b0 * in + b1 * s.x1 + b2 * s.x2 - a1 * s.y1 - a2 * s.y2;
		s.x2 = s.x1;
		s.x1 = in;
		s.y2 = s.y1;
		s.y1 = out;
		return out;
	}
};

struct Bitcrusher : Module {
	enum ParamId {
		SAMPLERATE_PARAM,
		BITDEPTH_PARAM,
		DRY_WET_PARAM,
		CUTOFF_PARAM,
		RESONANCE_PARAM,
		FILTERTYPE_PARAM,
		VOLUME_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SAMPLERATECVIN_INPUT,
		BITDEPTHCVIN_INPUT,
		DRY_WETCVIN_INPUT,
		CUTOFFCVIN_INPUT,
		RESONANCECVIN_INPUT,
		VOLUMECVIN_INPUT,
		AUDIOLEFTIN_INPUT,
		AUDIORIGHTIN_INPUT,
		INPUTS_LEN
	};
	enum OutputId { AUDIOLEFTOUT_OUTPUT, AUDIORIGHTOUT_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	const float sampleRateMinHz = 20.f;
	const float sampleRateMaxHz = 15000.f;
	const float cutoffMinHz = 20.f;
	const float cutoffMaxHz = 15000.f;

	float sampleHoldPhase = 0.f;
	float leftSampleHold[PORT_MAX_CHANNELS] = {};
	float rightSampleHold[PORT_MAX_CHANNELS] = {};

	// Shared coefficients (computed once per frame), per-channel filter state.
	Biquad filterCoeffs;
	BiquadState filterStateL[PORT_MAX_CHANNELS];
	BiquadState filterStateR[PORT_MAX_CHANNELS];

	// Cache last parameters to avoid redundant coefficient recalculation
	float lastCutoff = -1.f;
	float lastResonance = -1.f;
	bool lastIsLowpass = true;

	Bitcrusher() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(SAMPLERATE_PARAM, 0.f, 1.f, 0.f, "Clock Frequency", " Hz", 0, (sampleRateMinHz - sampleRateMaxHz), sampleRateMaxHz);

		configSwitch(BITDEPTH_PARAM,
					 0.f,
					 15.f,
					 0.f,
					 "Bit Depth",
					 {"16", "15", "14", "13", "12", "11", "10", "9", "8", "7", "6", "5", "4", "3", "2", "1"});
		configParam(DRY_WET_PARAM, 0.f, 1.f, 1.f, "Dry/Wet", "%", 0.f, 100.f);
		configParam(CUTOFF_PARAM, 0.f, 1.f, 1.f, "Cutoff", "hz", 750.f, 20.f);
		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);
		configSwitch(FILTERTYPE_PARAM, 0.f, 1.f, 0.f, "Filter Type", {"Lowpass", "Highpass"});
		configParam(VOLUME_PARAM, 0.f, 1.f, 1.f, "Volume", "%", 0.f, 100.f);

		configInput(SAMPLERATECVIN_INPUT, "Sample Rate CV");
		configInput(BITDEPTHCVIN_INPUT, "Bit Depth CV");
		configInput(DRY_WETCVIN_INPUT, "Dry/Wet CV");
		configInput(CUTOFFCVIN_INPUT, "Cutoff CV");
		configInput(RESONANCECVIN_INPUT, "Resonance CV");
		configInput(VOLUMECVIN_INPUT, "Volume CV");
		configInput(AUDIOLEFTIN_INPUT, "Audio Left");
		configInput(AUDIORIGHTIN_INPUT, "Audio Right");

		configOutput(AUDIOLEFTOUT_OUTPUT, "Audio Left");
		configOutput(AUDIORIGHTOUT_OUTPUT, "Audio Right");

		configBypass(AUDIOLEFTIN_INPUT, AUDIOLEFTOUT_OUTPUT);
		configBypass(AUDIORIGHTIN_INPUT, AUDIORIGHTOUT_OUTPUT);
	}

	float getNormalizedParam(int paramId, int inputId) {
		float paramValue = params[paramId].getValue();
		float inputCV =
			inputs[inputId].isConnected() ? std::clamp(inputs[inputId].getVoltage(), -5.f, 5.f) / 10.f : 0.f;
		return std::clamp(paramValue + inputCV, 0.f, 1.f);
	}

	void updateFilterCoefficients(float cutoff, float resonance, float sampleRate, bool isLowpass) {
		if (cutoff != lastCutoff || resonance != lastResonance || isLowpass != lastIsLowpass) {
			if (isLowpass)
				filterCoeffs.setupLowpass(cutoff, resonance, sampleRate);
			else
				filterCoeffs.setupHighpass(cutoff, resonance, sampleRate);
			lastCutoff = cutoff;
			lastResonance = resonance;
			lastIsLowpass = isLowpass;
		}
	}

	void process(const ProcessArgs &args) override {
		// --- Channel counts: R normalizes from L when disconnected ---
		const bool rConnected = inputs[AUDIORIGHTIN_INPUT].isConnected();
		const int nL = inputs[AUDIOLEFTIN_INPUT].getChannels();
		const int nR = rConnected ? inputs[AUDIORIGHTIN_INPUT].getChannels() : nL;
		const int n = std::max(nL, nR);

		outputs[AUDIOLEFTOUT_OUTPUT].setChannels(n);
		outputs[AUDIORIGHTOUT_OUTPUT].setChannels(n);

		// --- Mono CVs: computed once, shared across all poly channels ---
		const float srParam = params[SAMPLERATE_PARAM].getValue();
		const float srCV = inputs[SAMPLERATECVIN_INPUT].isConnected() ?
							   std::clamp(inputs[SAMPLERATECVIN_INPUT].getVoltage(), -5.f, 5.f) / 10.f :
							   0.f;
		const float normSampleRate = std::clamp(srParam + srCV, 0.f, 1.f);
		const float sampleRateHz = sampleRateMinHz + (1.f - normSampleRate) * (sampleRateMaxHz - sampleRateMinHz);

		// --- Sample & hold: shared clock, per-channel held values ---
		const float holdInterval = args.sampleRate / sampleRateHz;
		sampleHoldPhase += 1.f;
		const bool shouldSample = sampleHoldPhase >= holdInterval;
		if (shouldSample)
			sampleHoldPhase -= holdInterval;

		// --- Bit depth (mono CV) ---
		const float bitCV = inputs[BITDEPTHCVIN_INPUT].isConnected() ?
								clamp(inputs[BITDEPTHCVIN_INPUT].getVoltage(), -5.f, 5.f) / 5.f * 15.f :
								0.f;
		const int bitDepth = (int)clamp(15.f - (params[BITDEPTH_PARAM].getValue() + bitCV), 0.f, 15.f);

		auto bitcrush = [bitDepth](float in) {
			if (bitDepth >= 15)
				return in;
			float norm = std::clamp((in + 5.f) * 0.1f, 0.f, 1.f);
			if (bitDepth <= 0)
				return norm >= 0.5f ? 5.f : -5.f;
			const float levels = static_cast<float>((1 << bitDepth) - 1);
			return std::round(norm * levels) / levels * 10.f - 5.f;
		};

		// --- Filter coefficients (mono CV, updated once per frame) ---
		const float cutoffNorm = getNormalizedParam(CUTOFF_PARAM, CUTOFFCVIN_INPUT);
		const float resonance = getNormalizedParam(RESONANCE_PARAM, RESONANCECVIN_INPUT);
		const float cutoff = cutoffMinHz + cutoffNorm * (cutoffMaxHz - cutoffMinHz);
		const bool isLowpass = params[FILTERTYPE_PARAM].getValue() < 0.5f;
		updateFilterCoefficients(cutoff, resonance, args.sampleRate, isLowpass);

		// --- Dry/wet and volume (mono CVs) ---
		const float dryWet = getNormalizedParam(DRY_WET_PARAM, DRY_WETCVIN_INPUT);
		const float volume = getNormalizedParam(VOLUME_PARAM, VOLUMECVIN_INPUT);

		// --- Per poly channel ---
		for (int c = 0; c < n; c++) {
			float inL = inputs[AUDIOLEFTIN_INPUT].getPolyVoltage(c);
			float inR = rConnected ? inputs[AUDIORIGHTIN_INPUT].getPolyVoltage(c) : inL;

			if (shouldSample) {
				leftSampleHold[c] = inL;
				rightSampleHold[c] = inR;
			}

			const float filteredL = filterCoeffs.processChannel(bitcrush(leftSampleHold[c]), filterStateL[c]);
			const float filteredR = filterCoeffs.processChannel(bitcrush(rightSampleHold[c]), filterStateR[c]);

			outputs[AUDIOLEFTOUT_OUTPUT].setVoltage(
				std::clamp(rack::math::crossfade(inL, filteredL, dryWet) * volume, -5.f, 5.f), c);
			outputs[AUDIORIGHTOUT_OUTPUT].setVoltage(
				std::clamp(rack::math::crossfade(inR, filteredR, dryWet) * volume, -5.f, 5.f), c);
		}
	}
};

struct BitcrusherWidget : ModuleWidget {
	BitcrusherWidget(Bitcrusher *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Bitcrusher_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies_large>(mm2px(Vec(14.499, 18.5)), module, Bitcrusher::SAMPLERATE_PARAM));
		addParam(createParamCentered<Davies_large>(mm2px(Vec(46.5, 18.5)), module, Bitcrusher::BITDEPTH_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(30.502, 41.751)), module, Bitcrusher::DRY_WET_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(13.501, 58.998)), module, Bitcrusher::CUTOFF_PARAM));
		addParam(
			createParamCentered<Davies1900hBlack>(mm2px(Vec(47.502, 58.998)), module, Bitcrusher::RESONANCE_PARAM));
		addParam(createParamCentered<Switch2PosHorizontal>(
			mm2px(Vec(13.501, 95.002)), module, Bitcrusher::FILTERTYPE_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(47.502, 94.999)), module, Bitcrusher::VOLUME_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(13.501, 41.751)), module, Bitcrusher::SAMPLERATECVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.502, 41.751)), module, Bitcrusher::BITDEPTHCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(13.501, 77.963)), module, Bitcrusher::CUTOFFCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.502, 77.963)), module, Bitcrusher::DRY_WETCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.502, 77.963)), module, Bitcrusher::RESONANCECVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.502, 94.999)), module, Bitcrusher::VOLUMECVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.1, 111.001)), module, Bitcrusher::AUDIOLEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.001, 111.001)), module, Bitcrusher::AUDIORIGHTIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.1, 111.001)), module, Bitcrusher::AUDIOLEFTOUT_OUTPUT));
		addOutput(
			createOutputCentered<PJ301MPort>(mm2px(Vec(53.002, 111.001)), module, Bitcrusher::AUDIORIGHTOUT_OUTPUT));
	}
};

Model *modelBitcrusher = createModel<Bitcrusher, BitcrusherWidget>("Bitcrusher");
