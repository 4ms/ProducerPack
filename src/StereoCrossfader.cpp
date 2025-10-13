#include "plugin.hpp"

struct StereoCrossfader : Module {
	enum ParamId { MIX_PARAM, SHAPE_PARAM, PARAMS_LEN };
	enum InputId { MIXCV_INPUT, INAL_INPUT, INAR_INPUT, INBL_INPUT, INBR_INPUT, INPUTS_LEN };
	enum OutputId { OUTL_OUTPUT, OUTR_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	StereoCrossfader() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MIX_PARAM, 0.f, 1.f, 0.5f, "Mix", "%", 0.f, 100.f);
		configParam(SHAPE_PARAM, 0.f, 1.f, 0.f, "Shape", "%", 0.f, 100.f);
		configInput(MIXCV_INPUT, "Mix CV");
		configInput(INAL_INPUT, "Audio Left A");
		configInput(INAR_INPUT, "Audio Right A");
		configInput(INBL_INPUT, "Audio Left B");
		configInput(INBR_INPUT, "Audio Right B");
		configOutput(OUTL_OUTPUT, "Audio Left");
		configOutput(OUTR_OUTPUT, "Audio Right");
	}

	void process(const ProcessArgs &args) override {
		// --- Read CV input ---
		float mixCV = inputs[MIXCV_INPUT].getVoltage();

		// --- Update mix param only if changed ---
		float newMixParam = params[MIX_PARAM].getValue();
		if (newMixParam != cachedMixParam || mixCV != cachedMixCV) {
			cachedMixParam = newMixParam;
			cachedMixCV = mixCV;

			float offset = rescale(cachedMixParam, 0.f, 1.f, -5.f, 5.f);
			float sum = offset + cachedMixCV;
			float clamped = std::clamp(sum, -5.f, 5.f);
			mix = rescale(clamped, -5.f, 5.f, 0.f, 1.f);

			// Need to recompute gains when mix changes
			updateMixShapeDependent();
		}

		// --- Update shape param only if changed ---
		float newShape = params[SHAPE_PARAM].getValue();
		if (newShape != cachedShape) {
			cachedShape = newShape;
			updateMixShapeDependent();
		}

		// --- Inputs ---
		float aL = inputs[INAL_INPUT].getVoltage();
		float aR = inputs[INAR_INPUT].isConnected() ? inputs[INAR_INPUT].getVoltage() : aL;
		float bL = inputs[INBL_INPUT].getVoltage();
		float bR = inputs[INBR_INPUT].isConnected() ? inputs[INBR_INPUT].getVoltage() : bL;

		// --- Outputs ---
		outputs[OUTL_OUTPUT].setVoltage(gainA * aL + gainB * bL);
		outputs[OUTR_OUTPUT].setVoltage(gainA * aR + gainB * bR);
	}

	// Called when either shape or mix changes
	void updateMixShapeDependent() {
		float shape = cachedShape;
		float k = 9.f * shape + 1.f;
		float denom = log(k + 1e-6f); // add epsilon to avoid log(0)
		float x = mix;

		if (x < 0.5f) {
			x *= 2.f;
			float y = x * (1.f - shape) + log(1.f + (k - 1.f) * x) / denom * shape;
			mixShaped = 0.5f * y;
		} else {
			x = 2.f * (x - 0.5f);
			float y = x * (1.f - shape) + log(1.f + (k - 1.f) * x) / denom * shape;
			mixShaped = 0.5f + 0.5f * y;
		}

		// Equal power gain calculation
		gainA = sin((1.f - mixShaped) * 0.5f * M_PI);
		gainB = sin(mixShaped * 0.5f * M_PI);
	}

private:
	// Cached param values
	float cachedMixParam = -1.f;
	float cachedMixCV = 9999.f; // Unlikely value to force first update
	float cachedShape = -1.f;

	// Cached intermediate values
	float mix = 0.f;
	float mixShaped = 0.f;
	float gainA = 0.f;
	float gainB = 0.f;
};

struct StereoCrossfaderWidget : ModuleWidget {
	StereoCrossfaderWidget(StereoCrossfader *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/StereoCrossfader_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(15.24, 18.803)), module, StereoCrossfader::MIX_PARAM));
		addParam(
			createParamCentered<Davies1900hBlack>(mm2px(Vec(15.24, 60.748)), module, StereoCrossfader::SHAPE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 38.798)), module, StereoCrossfader::MIXCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.303, 82.991)), module, StereoCrossfader::INAL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.001, 82.991)), module, StereoCrossfader::INAR_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.303, 97.014)), module, StereoCrossfader::INBL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.001, 97.014)), module, StereoCrossfader::INBR_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.303, 111.001)), module, StereoCrossfader::OUTL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.001, 111.001)), module, StereoCrossfader::OUTR_OUTPUT));
	}
};

Model *modelStereoCrossfader = createModel<StereoCrossfader, StereoCrossfaderWidget>("StereoCrossfader");
