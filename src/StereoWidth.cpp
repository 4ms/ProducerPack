#include "plugin.hpp"

struct StereoWidth : Module {
	enum ParamId { WIDTH_PARAM, PAN_PARAM, PARAMS_LEN };
	enum InputId { WIDTHCV_INPUT, PANCV_INPUT, INL_INPUT, INR_INPUT, INPUTS_LEN };
	enum OutputId { OUTL_OUTPUT, OUTR_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

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

		configBypass(INL_INPUT, OUTL_OUTPUT);
		configBypass(INR_INPUT, OUTR_OUTPUT);
	}
	void process(const ProcessArgs &args) override {
		// --- Recalculate width if param or CV changed (mono CV) ---
		float newWidthParam = params[WIDTH_PARAM].getValue();
		float newWidthCV = inputs[WIDTHCV_INPUT].isConnected() ? inputs[WIDTHCV_INPUT].getVoltage() : 0.f;

		if (newWidthParam != cachedWidthParam || newWidthCV != cachedWidthCV) {
			cachedWidthParam = newWidthParam;
			cachedWidthCV = newWidthCV;

			float width = std::clamp(cachedWidthParam + cachedWidthCV * 0.1f, 0.f, 1.f);
			cachedWidth = width;

			if (width <= 0.5f) {
				widthBlendT = width * 2.f;
				isWidthMonoToStereo = true;
			} else {
				isWidthMonoToStereo = false;
				widthBlendT = (width <= 0.75f) ? (width - 0.5f) * 4.f : 1.f;
				widthBoost = (width > 0.75f) ? (width - 0.75f) * 4.f : 0.f;
				widthGain = 1.f + widthBoost;
			}
		}

		// --- Recalculate pan if param or CV changed (mono CV) ---
		float newPanParam = params[PAN_PARAM].getValue();
		float newPanCV = inputs[PANCV_INPUT].isConnected() ? inputs[PANCV_INPUT].getVoltage() : 0.f;

		if (newPanParam != cachedPanParam || newPanCV != cachedPanCV) {
			cachedPanParam = newPanParam;
			cachedPanCV = newPanCV;

			float pan = std::clamp(cachedPanParam + cachedPanCV * 10.f, -50.f, 50.f) * 0.02f;
			cachedPan = pan;

			panL = 1.f - pan;
			panR = 1.f + pan;
		}

		// --- Channel counts: R normalizes from L when disconnected ---
		const bool rConnected = inputs[INR_INPUT].isConnected();
		const int nL = inputs[INL_INPUT].getChannels();
		const int nR = rConnected ? inputs[INR_INPUT].getChannels() : nL;
		const int n = std::max(nL, nR);

		outputs[OUTL_OUTPUT].setChannels(n);
		outputs[OUTR_OUTPUT].setChannels(n);

		for (int c = 0; c < n; c++) {
			float l = inputs[INL_INPUT].getPolyVoltage(c);
			float r = rConnected ? inputs[INR_INPUT].getPolyVoltage(c) : l;

			float outL, outR;
			if (isWidthMonoToStereo) {
				float mono = 0.5f * (l + r);
				outL = crossfade(mono, l, widthBlendT);
				outR = crossfade(mono, r, widthBlendT);
			} else {
				float diff = 0.5f * (l - r);
				outL = l + widthBlendT * widthGain * diff;
				outR = r - widthBlendT * widthGain * diff;
			}

			outputs[OUTL_OUTPUT].setVoltage(outL * panL, c);
			outputs[OUTR_OUTPUT].setVoltage(outR * panR, c);
		}
	}

private:
	// Cached params
	float cachedWidthParam = -1.f;
	float cachedWidthCV = -1000.f;
	float cachedWidth = 0.f;

	float cachedPanParam = -1.f;
	float cachedPanCV = -1000.f;
	float cachedPan = 0.f;

	// Width-related cached values
	bool isWidthMonoToStereo = true;
	float widthBlendT = 0.f;
	float widthBoost = 0.f;
	float widthGain = 1.f;

	// Pan-related cached values
	float panL = 1.f;
	float panR = 1.f;
};

struct StereoWidthWidget : ModuleWidget {
	StereoWidthWidget(StereoWidth *module) {
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

Model *modelStereoWidth = createModel<StereoWidth, StereoWidthWidget>("StereoWidth");
