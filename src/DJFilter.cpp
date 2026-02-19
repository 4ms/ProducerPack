#include "helpers/math_lut.hpp"
#include "plugin.hpp"

struct DJFilter : Module {
	enum ParamId { CUTOFF_PARAM, RESONANCE_PARAM, SLOPE_PARAM, PARAMS_LEN };
	enum InputId { INL_INPUT, CUTOFFCV_INPUT, INR_INPUT, RESONANCECV_INPUT, INPUTS_LEN };
	enum OutputId { OUT_L_OUTPUT, OUT_R_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	struct DJFilterCutoffQuantity : ParamQuantity {
		using ParamQuantity::ParamQuantity;

		std::string getDisplayValueString() override {
			float morph = getValue(); // knob value only, no CV

			if (morph <= 0.45f) {
				float t = morph / 0.5f;
				float cutoffHz = Pow2(rescale(t, 0.f, 1.f, Log2(20.f), Log2(2000.f)));
				return string::f("Cutoff: %.2fhz", cutoffHz);
			} else if (morph < 0.55f) {
				return "Cutoff: BYPASS";
			} else {
				float t = (morph - 0.5f) / 0.5f;
				float cutoffHz = Pow2(rescale(t, 0.f, 1.f, Log2(300.f), Log2(7000.f)));
				return string::f("Cutoff: %.2fhz", cutoffHz);
			}
		}

		std::string getLabel() override {
			return ""; // prevents the "#1:" from showing
		}

		void reset() override {
			setValue(0.5f);
		}
	};

	struct Pow2TableRange {
		static constexpr float min = 4.f;
		static constexpr float max = 13.f;
	};

	static inline const auto Pow2 =
		Mapping::LookupTable_t<64, float>::generate<Pow2TableRange>([](float x) { return std::pow(2.f, x); });

	struct Log2TableRange {
			static constexpr float min = 20.f;
			static constexpr float max = 7000.f;
		};
		
	static inline const auto Log2 =
			Mapping::LookupTable_t<64, float>::generate<Log2TableRange>([](float x) { return std::log2(x); });
		
	

	DJFilter() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(CUTOFF_PARAM, 0.f, 1.f, 0.5f, "");

		auto *cutoffQ = new DJFilterCutoffQuantity();
		cutoffQ->module = this;
		cutoffQ->paramId = CUTOFF_PARAM;
		cutoffQ->setValue(0.5f); // <-- Explicitly set default value
		paramQuantities[CUTOFF_PARAM] = cutoffQ;

		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);
		configSwitch(SLOPE_PARAM, 1.f, 4.f, 1.f, "Slope", {"6db/Oct", "12db/Oct", "18db/Oct", "24db/Oct"});

		configInput(INL_INPUT, "Audio Left");
		configInput(CUTOFFCV_INPUT, "Cutoff CV");
		configInput(INR_INPUT, "Audio Right");
		configInput(RESONANCECV_INPUT, "Resonance CV");

		configOutput(OUT_L_OUTPUT, "Audio Left");
		configOutput(OUT_R_OUTPUT, "Audio Right");

		configBypass(INL_INPUT, OUT_L_OUTPUT);
		configBypass(INR_INPUT, OUT_R_OUTPUT);
	}

	struct SVF {
		float low = 0.f;
		float band = 0.f;
		float high = 0.f;

		void process(float in, float cutoff, float q, float sampleRate) {
			float f = 2.f * Sin(M_PI * cutoff / sampleRate);
			float damp = 1.f / q;

			high = in - low - damp * band;
			band += f * high;
			low += f * band;
		}
	};

	SVF svfL[4], svfR[4];

	const float log2_20 = Log2(20.f);
	const float log2_300 = Log2(300.f);
	const float log2_7000 = Log2(7000.f);

	// morph will be different than lastMorph => forces finalFreq to be calc the first time its run
	float lastMorph = -1.f;

	float lastSampleRate = 0.f;

	float finalFreq = 300.f;

	void process(const ProcessArgs &args) override {

		// Cache inputs
		float inL = inputs[INL_INPUT].getVoltage();
		float inR = inputs[INR_INPUT].isConnected() ? inputs[INR_INPUT].getVoltage() : inL;

		// Parameters + CV (cutoff and resonance)
		float cutoffVal = params[CUTOFF_PARAM].getValue();
		float cutoffCV = inputs[CUTOFFCV_INPUT].isConnected() ? inputs[CUTOFFCV_INPUT].getVoltage() * 0.1f : 0.f; // /10
		float morph = std::clamp(cutoffVal + cutoffCV, 0.f, 1.f);

		float resVal = params[RESONANCE_PARAM].getValue();
		float resCV = inputs[RESONANCECV_INPUT].isConnected() ? inputs[RESONANCECV_INPUT].getVoltage() * 0.1f : 0.f;
		float resonanceParam = std::clamp(resVal + resCV, 0.f, 1.f);

		int stages = std::clamp((int)params[SLOPE_PARAM].getValue(), 1, 4);

		float cutoffHz;

		if (morph != lastMorph || args.sampleRate != lastSampleRate) {
			lastMorph = morph;

			if (args.sampleRate != lastSampleRate) {
				for (int i = 0; i < 4; ++i) {
					svfL[i] = {};
					svfR[i] = {};
				}
			}

			lastSampleRate = args.sampleRate;

			if (morph < 0.5f) {
				float t = morph * 2.f;
				cutoffHz = Pow2(rescale(t, 0.f, 1.f, log2_20, log2_7000));
			} else {
				float t = (morph - 0.5f) * 2.f;
				cutoffHz = Pow2(rescale(t, 0.f, 1.f, log2_300, log2_7000));
			}

			float nyquist = 0.5f * args.sampleRate;
			float safeCutoff = std::min(cutoffHz, nyquist * 0.49f);

			finalFreq = 2.f * Sin(M_PI * safeCutoff / args.sampleRate);

			// CRITICAL: SVF stability clamp (cascade-aware)
			float maxF = (stages >= 4) ? 0.55f :
			             (stages == 3) ? 0.65f :
			             (stages == 2) ? 0.75f :
			                             0.95f;

			finalFreq = std::min(finalFreq, maxF);

			if (!std::isfinite(finalFreq) || finalFreq <= 0.f) {
				finalFreq = 0.f;
				for (int i = 0; i < 4; ++i) {
					svfL[i] = {};
					svfR[i] = {};
				}
			}
		}

		float q = rescale(resonanceParam, 0.f, 1.f, 0.707f, 3.f);
		float damp = 1.f / q;

		// Dry input saved for mix
		float dryL = inL;
		float dryR = inR;

		float inL_stage = inL;
		float inR_stage = inR;
		
		for (int i = 0; i < stages; ++i) {
			SVF &sL = svfL[i];
			SVF &sR = svfR[i];
		
			// Left channel
			sL.high = inL_stage - sL.low - damp * sL.band;
			sL.band += finalFreq * sL.high;
			sL.low += finalFreq * sL.band;
			inL_stage = sL.high;    // <-- cascade HIGH output
		
			// Right channel
			sR.high = inR_stage - sR.low - damp * sR.band;
			sR.band += finalFreq * sR.high;
			sR.low += finalFreq * sR.band;
			inR_stage = sR.high;    // <-- cascade HIGH output
		}
		

		// Mix logic
		const float fadeWidth = 0.02f;
		const float dryStart = 0.45f;
		const float dryEnd = 0.55f;

		float lowMix = 0.f;
		float dryMix = 0.f;
		float highMix = 0.f;

		if (morph <= dryStart - fadeWidth) {
			lowMix = 1.f;
		} else if (morph < dryStart + fadeWidth) {
			float t = (morph - (dryStart - fadeWidth)) / (2.f * fadeWidth);
			lowMix = 1.f - t;
			dryMix = t;
		} else if (morph < dryEnd - fadeWidth) {
			dryMix = 1.f;
		} else if (morph < dryEnd + fadeWidth) {
			float t = (morph - (dryEnd - fadeWidth)) / (2.f * fadeWidth);
			dryMix = 1.f - t;
			highMix = t;
		} else {
			highMix = 1.f;
		}

		// Output mixing
		SVF &sL = svfL[stages - 1];
		SVF &sR = svfR[stages - 1];
		
		float outL = sL.low * lowMix + dryL * dryMix + sL.high * highMix;
		float outR = sR.low * lowMix + dryR * dryMix + sR.high * highMix;
		

		outputs[OUT_L_OUTPUT].setVoltage(clamp(outL, -10.f, 10.f));
		outputs[OUT_R_OUTPUT].setVoltage(clamp(outR, -10.f, 10.f));
	}
};

struct DJFilterWidget : ModuleWidget {
	DJFilterWidget(DJFilter *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/DJFilter_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(15.24, 18.803)), module, DJFilter::CUTOFF_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(15.24, 41.501)), module, DJFilter::RESONANCE_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(15.24, 63.747)), module, DJFilter::SLOPE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.303, 82.991)), module, DJFilter::CUTOFFCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.001, 83.001)), module, DJFilter::RESONANCECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.303, 97.014)), module, DJFilter::INL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.001, 97.014)), module, DJFilter::INR_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.303, 111.001)), module, DJFilter::OUT_L_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.001, 111.001)), module, DJFilter::OUT_R_OUTPUT));
	}
};

Model *modelDJFilter = createModel<DJFilter, DJFilterWidget>("DJFilter");
