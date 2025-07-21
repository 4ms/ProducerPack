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
		// Read the input signals
		float l = inputs[INL_INPUT].getVoltage();
		float r = inputs[INR_INPUT].getVoltage();
	
		// Apply the width control first
		float width = params[WIDTH_PARAM].getValue();
		if (inputs[WIDTHCV_INPUT].isConnected()) {
			width += inputs[WIDTHCV_INPUT].getVoltage() / 10.f;  // Offset by the width CV
		}
	
		// Clamp width to [0.0, 1.0]
		if (width < 0.f) width = 0.f;
		if (width > 1.f) width = 1.f;
	
		// Initialize output signals
		float outL = 0.f;
		float outR = 0.f;
	
		// Apply width effect: Mono → Stereo blend or Stereo → Wider differential
		if (width <= 0.5f) {
			// Mono → Stereo blend (when width is <= 50%)
			float t = width * 2.f;  // This will give values between 0 and 1
			float mono = (l + r) * 0.5f;  // Mix both signals to mono
	
			outL = mono * (1.f - t) + l * t;  // Blend mono signal with left input
			outR = mono * (1.f - t) + r * t;  // Blend mono signal with right input
		} else {
			// Stereo → Wider differential (when width is > 50%)
			float diff = (l - r) * 0.5f;
			float gain = 1.0f;
			float t = 1.0f;
	
			if (width <= 0.75f) {
				// 1x diff gain
				t = (width - 0.5f) * 4.f;
				gain = 1.0f;
			} else {
				// 1x to 2x diff gain
				t = 1.0f;
				float boost = (width - 0.75f) * 4.f;
				gain = 1.0f + boost;  // ramps 1.0 → 2.0
			}
	
			float diffL = diff * gain;
			float diffR = -diff * gain;
	
			outL = l + diffL * t;
			outR = r + diffR * t;
		}
	
		// Now apply the pan control (after width)
		float pan = params[PAN_PARAM].getValue();
	
		if (inputs[PANCV_INPUT].isConnected()) {
			// Apply the Pan CV and scale it to the range [-50, 50]
			float panCV = inputs[PANCV_INPUT].getVoltage();
			pan += panCV * 50.f / 5.f;  // Scale CV from [-5V, 5V] to [-50, 50] range
		}
	
		// Clamp pan value to [-50%, 50%]
		pan = clamp(pan, -50.f, 50.f);
	
		// Map pan from [-50%, 50%] to [-1.0, 1.0]
		float panFactor = pan / 50.f;  // Now panFactor ranges from -1.0 to 1.0
	
		// Apply pan: For panning, we adjust the L and R channels
		outL = outL * (1.0f - panFactor);  // Apply pan to Left
		outR = outR * (1.0f + panFactor);  // Apply pan to Right
	
		// Clip the output to the range of [-5V, 5V]
		outL = clamp(outL, -5.f, 5.f);
		outR = clamp(outR, -5.f, 5.f);
	
		// Set the output voltages
		outputs[OUTL_OUTPUT].setVoltage(outL);
		outputs[OUTR_OUTPUT].setVoltage(outR);
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
