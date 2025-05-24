#include "plugin.hpp"


struct StereoCrossfader : Module {
	enum ParamId {
		MIX_PARAM,
		SHAPE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		MIXCV_INPUT,
		INAL_INPUT,
		INAR_INPUT,
		INBL_INPUT,
		INBR_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTL_OUTPUT,
		OUTR_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	StereoCrossfader() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MIX_PARAM, 0.f, 1.f, 0.f, "Mix", "%", 0.f, 100.f);
		configParam(SHAPE_PARAM, 0.f, 1.f, 0.f, "Shape", "%", 0.f, 100.f);
		configInput(MIXCV_INPUT, "Mix CV");
		configInput(INAL_INPUT, "Left A");
		configInput(INAR_INPUT, "Right A");
		configInput(INBL_INPUT, "Left B");
		configInput(INBR_INPUT, "Right B");
		configOutput(OUTL_OUTPUT, "Left");
		configOutput(OUTR_OUTPUT, "Right");
	}

	void process(const ProcessArgs& args) override {
		float mixKnob = clamp(params[MIX_PARAM].getValue(), 0.f, 1.f);
		float shape = clamp(params[SHAPE_PARAM].getValue(), 0.f, 1.f);
	
		float k = 9.f * shape + 1.f;
	
		auto logCurve = [&](float x) {
			float epsilon = 1e-6f;
			float denom = logf(k + epsilon);
			return logf(1.f + (k - 1.f) * x) / denom;
		};
	
		float curvedMix;
	
		if (mixKnob < 0.5f) {
			float x = mixKnob / 0.5f;
			float linearY = x;
			float logY = logCurve(x);
			float y = linearY * (1.f - shape) + logY * shape;
			curvedMix = 0.5f * y;
		} else {
			float x = (mixKnob - 0.5f) / 0.5f;
			float linearY = x;
			float logY = logCurve(x);
			float y = linearY * (1.f - shape) + logY * shape;
			curvedMix = 0.5f + 0.5f * y;
		}
	
		curvedMix = clamp(curvedMix, 0.f, 1.f);
	
		float offset = rescale(curvedMix, 0.f, 1.f, -5.f, 5.f);
		float cv = clamp(inputs[MIXCV_INPUT].getVoltage(), -5.f, 5.f);
		float total = clamp(offset + cv, -5.f, 5.f);
		float mix = rescale(total, -5.f, 5.f, 0.f, 1.f);
		mix = clamp(mix, 0.f, 1.f);
	
		float gainA = cosf(mix * 0.5f * M_PI);
		float gainB = cosf((1.f - mix) * 0.5f * M_PI);
	
		float aL = inputs[INAL_INPUT].getVoltage();
		float aR = inputs[INAR_INPUT].isConnected() ? inputs[INAR_INPUT].getVoltage() : aL;
	
		float bL = inputs[INBL_INPUT].getVoltage();
		float bR = inputs[INBR_INPUT].isConnected() ? inputs[INBR_INPUT].getVoltage() : bL;
	
		outputs[OUTL_OUTPUT].setVoltage(gainA * aL + gainB * bL);
		outputs[OUTR_OUTPUT].setVoltage(gainA * aR + gainB * bR);
	}	
};

struct StereoCrossfaderWidget : ModuleWidget {
	StereoCrossfaderWidget(StereoCrossfader* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/StereoCrossfader.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(9.998, 27.297)), module, StereoCrossfader::MIX_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(9.998, 54.284)), module, StereoCrossfader::SHAPE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.998, 43.966)), module, StereoCrossfader::MIXCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.705, 83.693)), module, StereoCrossfader::INAL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.13, 83.693)), module, StereoCrossfader::INAR_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.705, 96.132)), module, StereoCrossfader::INBL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.13, 96.132)), module, StereoCrossfader::INBR_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.705, 108.82)), module, StereoCrossfader::OUTL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.13, 108.82)), module, StereoCrossfader::OUTR_OUTPUT));
	}
};


Model* modelStereoCrossfader = createModel<StereoCrossfader, StereoCrossfaderWidget>("StereoCrossfader");