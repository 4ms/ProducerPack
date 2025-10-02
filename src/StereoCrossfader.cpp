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
		// --- Mix Param + CV ---
		float mixParam = std::clamp(params[MIX_PARAM].getValue(), 0.f, 1.f);
		float cv = inputs[MIXCV_INPUT].getVoltage();
		float offset = rescale(mixParam, 0.f, 1.f, -5.f, 5.f);
		float mix = std::clamp(rescale(clamp(offset + cv, -5.f, 5.f), -5.f, 5.f, 0.f, 1.f), 0.f, 1.f);

		// --- Shape ---
		float shape = std::clamp(params[SHAPE_PARAM].getValue(), 0.f, 1.f);
		float k = 9.f * shape + 1.f;
		float denom = std::log(k + 1e-6f);

		// --- Curved mix (log interpolation) ---
		float x = mix;
		if (x < 0.5f) {
			x *= 2.f;
			float y = x * (1.f - shape) + std::log(1.f + (k - 1.f) * x) / denom * shape;
			mix = 0.5f * y;
		} else {
			x = 2.f * (x - 0.5f);
			float y = x * (1.f - shape) + std::log(1.f + (k - 1.f) * x) / denom * shape;
			mix = 0.5f + 0.5f * y;
		}

		// --- Equal-power gain calculation using a fast approximation of cosine
		// cos(pi * x / 2) ≈ sin((1 - x) * pi/2)
		float gainA = std::sin((1.f - mix) * 0.5f * M_PI);
		float gainB = std::sin(mix * 0.5f * M_PI);

		// --- Inputs ---
		float aL = inputs[INAL_INPUT].getVoltage();
		float aR = inputs[INAR_INPUT].isConnected() ? inputs[INAR_INPUT].getVoltage() : aL;
		float bL = inputs[INBL_INPUT].getVoltage();
		float bR = inputs[INBR_INPUT].isConnected() ? inputs[INBR_INPUT].getVoltage() : bL;

		// --- Outputs ---
		outputs[OUTL_OUTPUT].setVoltage(gainA * aL + gainB * bL);
		outputs[OUTR_OUTPUT].setVoltage(gainA * aR + gainB * bR);
	}
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
