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
				float cutoffHz = std::pow(2.f, rescale(t, 0.f, 1.f, std::log2(20.f), std::log2(2000.f)));
				return string::f("Cutoff: %.2fhz", cutoffHz);
			} else if (morph < 0.55f) {
				return "Cutoff: BYPASS";
			} else {
				float t = (morph - 0.5f) / 0.5f;
				float cutoffHz = std::pow(2.f, rescale(t, 0.f, 1.f, std::log2(300.f), std::log2(7000.f)));
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
	}

	struct SVF {
		float low = 0.f;
		float band = 0.f;
		float high = 0.f;

		void process(float in, float cutoff, float q, float sampleRate) {
			float f = 2.f * sinf(M_PI * cutoff / sampleRate);
			float damp = 1.f / q;

			high = in - low - damp * band;
			band += f * high;
			low += f * band;
		}
	};

	SVF svfL[4], svfR[4];

	void process(const ProcessArgs &args) override {
		const float sampleRate = args.sampleRate;

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

		// Cutoff frequency calculation (log scale)
		float cutoffHz;
		if (morph < 0.5f) {
			float t = morph * 2.f;
			cutoffHz = std::pow(2.f, rescale(t, 0.f, 1.f, std::log2(20.f), std::log2(7000.f)));
		} else {
			float t = (morph - 0.5f) * 2.f;
			cutoffHz = std::pow(2.f, rescale(t, 0.f, 1.f, std::log2(300.f), std::log2(7000.f)));
		}

		float q = rescale(resonanceParam, 0.f, 1.f, 0.707f, 3.f);
		float f = 2.f * sinf(M_PI * cutoffHz / sampleRate);
		float damp = 1.f / q;

		// Dry input saved for mix
		float dryL = inL;
		float dryR = inR;

		// Apply cascaded SVFs
		for (int i = 0; i < stages; ++i) {
			DJFilter::SVF &svfL = this->svfL[i];
			DJFilter::SVF &svfR = this->svfR[i];

			// Left
			svfL.high = inL - svfL.low - damp * svfL.band;
			svfL.band += f * svfL.high;
			svfL.low += f * svfL.band;
			inL = svfL.low;

			// Right
			svfR.high = inR - svfR.low - damp * svfR.band;
			svfR.band += f * svfR.high;
			svfR.low += f * svfR.band;
			inR = svfR.low;
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
		DJFilter::SVF &svfLOut = svfL[stages - 1];
		DJFilter::SVF &svfROut = svfR[stages - 1];

		float outL = svfLOut.low * lowMix + dryL * dryMix + svfLOut.high * highMix;
		float outR = svfROut.low * lowMix + dryR * dryMix + svfROut.high * highMix;

		outputs[OUT_L_OUTPUT].setVoltage(clamp(outL, -5.f, 5.f));
		outputs[OUT_R_OUTPUT].setVoltage(clamp(outR, -5.f, 5.f));
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
