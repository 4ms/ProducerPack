#include "plugin.hpp"

struct StereoWidth : Module {
	enum ParamId {
		WIDTH_PARAM,
		PAN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		WIDTHCV_INPUT,
		PANCV_INPUT,
		INL_INPUT,
		INR_INPUT,
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

	StereoWidth() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(WIDTH_PARAM, 0.f, 1.f, 0.5f, "Width", "%", 0.f, 200.f);
		configParam(PAN_PARAM, -50.f, 50.f, 0.f, "Pan", "%");
		configInput(WIDTHCV_INPUT, "Width CV");
		configInput(PANCV_INPUT, "Pan CV");
		configInput(INL_INPUT, "Audio Left");
		configInput(INR_INPUT, "Audio Right");
		configOutput(OUTL_OUTPUT, "Audio Left");
		configOutput(OUTR_OUTPUT, "Audio Right");
	}

	void process(const ProcessArgs& args) override {
	// Input voltages
	float l = inputs[INL_INPUT].getVoltage();
	float r = inputs[INR_INPUT].getVoltage();

	// Width parameter + CV
	float width = params[WIDTH_PARAM].getValue();
	if (inputs[WIDTHCV_INPUT].isConnected())
		width += inputs[WIDTHCV_INPUT].getVoltage() * 0.1f; // same as /10.f

	width = clamp(width, 0.f, 1.f);

	float outL = 0.f, outR = 0.f;

	if (width <= 0.5f) {
		// Blend mono → stereo
		float t = width * 2.f;
		float mono = 0.5f * (l + r);
		outL = crossfade(mono, l, t);
		outR = crossfade(mono, r, t);
	} else {
		// Stereo widening
		float t = (width <= 0.75f) ? (width - 0.5f) * 4.f : 1.f;
		float boost = (width > 0.75f) ? (width - 0.75f) * 4.f : 0.f;
		float gain = 1.f + boost;

		float diff = 0.5f * (l - r);
		outL = l + t * gain * diff;
		outR = r - t * gain * diff;
	}

	// Pan control + CV
	float pan = params[PAN_PARAM].getValue();
	if (inputs[PANCV_INPUT].isConnected())
		pan += inputs[PANCV_INPUT].getVoltage() * 10.f; // scaled from 5V to 50%

	pan = clamp(pan, -50.f, 50.f) * 0.02f; // [-1.0, 1.0] = pan / 50

	// Apply pan - efficient gain calculation
	float panL = 1.f - pan;
	float panR = 1.f + pan;

	outL *= panL;
	outR *= panR;

	// Clamp final output voltages
	outputs[OUTL_OUTPUT].setVoltage(clamp(outL, -5.f, 5.f));
	outputs[OUTR_OUTPUT].setVoltage(clamp(outR, -5.f, 5.f));
}
};

struct StereoWidthWidget : ModuleWidget {
	StereoWidthWidget(StereoWidth* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/StereoWidth_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(15.24, 18.803)), module, StereoWidth::WIDTH_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(15.24, 56.762)), module, StereoWidth::PAN_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 38.805)), module, StereoWidth::WIDTHCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 76.747)), module, StereoWidth::PANCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.303, 97.014)), module, StereoWidth::INL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.001, 97.014)), module, StereoWidth::INR_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.303, 111.001)), module, StereoWidth::OUTL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.001, 111.001)), module, StereoWidth::OUTR_OUTPUT));
	}
};

Model* modelStereoWidth = createModel<StereoWidth, StereoWidthWidget>("StereoWidth");
