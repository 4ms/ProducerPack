#include "plugin.hpp"

struct Spatializer : Module {
	enum ParamId {
		RANGE_PARAM,
		TIME_PARAM,
		WIDTH_PARAM,
		MIDSIDE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TIMECV_INPUT,
		WIDTHCV_INPUT,
		MIDSIDECV_INPUT,
		INL_INPUT,
		INR_INPUT,
		RETURNL_INPUT,
		RETURNM_INPUT,
		RETURNR_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SENDL_OUTPUT,
		SENDM_OUTPUT,
		SENDR_OUTPUT,
		OUTL_OUTPUT,
		OUTR_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LEDL_LIGHT,
		LEDR_LIGHT,
		LEDM_LIGHT,
		LIGHTS_LEN
	};

	struct TimeParamQuantity : rack::engine::ParamQuantity {
		std::string getDisplayValueString() override {
			Spatializer* m = dynamic_cast<Spatializer*>(module);
			if (m) {
				bool isMillisecondsMode = m->params[Spatializer::RANGE_PARAM].getValue() < 0.5f;
				float normTime = getValue();
				if (isMillisecondsMode) {
					float timeMs = rescale(normTime, 0.f, 1.f, 1.f, 30.f);
					return rack::string::f("%.2fms", timeMs);
				} else {
					float timeSamples = rescale(normTime, 0.f, 1.f, 1.f, 50.f);
					return rack::string::f("%.0fsmps", timeSamples);
				}
			}
			return rack::string::f("%.2f", getValue());
		}
	};	
	
	Spatializer() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(RANGE_PARAM, 0.f, 1.f, 0.f, "Range", {"Milliseconds", "Samples"});

		configParam<TimeParamQuantity>(TIME_PARAM, 0.f, 1.f, 0.5f, "Time");

		configParam(WIDTH_PARAM, 0.f, 1.f, 1.f, "Width", "%", 0.f, 100.f);
		configParam(MIDSIDE_PARAM, 0.f, 1.f, 0.33f, "Mid/Side", "%", 0.f, 100.f);
		configInput(TIMECV_INPUT, "Time CV");
		configInput(WIDTHCV_INPUT, "Width CV");
		configInput(MIDSIDECV_INPUT, "Mid/Side");
		configInput(INL_INPUT, "Left");
		configInput(INR_INPUT, "Right");
		configInput(RETURNL_INPUT, "Return Left");
		configInput(RETURNM_INPUT, "Return Mid");
		configInput(RETURNR_INPUT, "Return Right");
		configOutput(SENDL_OUTPUT, "Send Left");
		configOutput(SENDM_OUTPUT, "Send Mid");
		configOutput(SENDR_OUTPUT, "Send Right");
		configOutput(OUTL_OUTPUT, "Left");
		configOutput(OUTR_OUTPUT, "Right");
	}

	static const int maxDelaySamples = 2880; // Enough for 30ms at ~96kHz
	float delayBufferL[maxDelaySamples] = {};
	float delayBufferR[maxDelaySamples] = {};
	int delayIndex = 0;

	float slewedTime = 0.f;  // Smoothed time value (normalized 0–1)
	const float timeSlewRate = 0.0005f;  // Adjust for smoothing speed

		// Helper function for crossfade (used for the dry/wet mix)
		inline float crossfade(float a, float b, float x) {
			return a * (1.f - x) + b * x;
		}
	
	void process(const ProcessArgs& args) override {}
};


struct SpatializerWidget : ModuleWidget {
	SpatializerWidget(Spatializer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Spatializer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.973, 19.886)), module, Spatializer::TIME_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(42.186, 19.748)), module, Spatializer::RANGE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(36.027, 38.875)), module, Spatializer::WIDTH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(36.027, 60.211)), module, Spatializer::MIDSIDE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.63, 19.886)), module, Spatializer::TIMECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(16.039, 38.875)), module, Spatializer::WIDTHCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(16.039, 60.211)), module, Spatializer::MIDSIDECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.497, 91.066)), module, Spatializer::INL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.497, 107.019)), module, Spatializer::INR_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.489, 107.019)), module, Spatializer::RETURNL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.862, 107.019)), module, Spatializer::RETURNM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34.953, 107.019)), module, Spatializer::RETURNR_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.489, 91.066)), module, Spatializer::SENDL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.862, 91.066)), module, Spatializer::SENDM_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(34.953, 91.066)), module, Spatializer::SENDR_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(44.275, 91.066)), module, Spatializer::OUTL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(44.275, 107.019)), module, Spatializer::OUTR_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.039, 73.563)), module, Spatializer::LEDL_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(25.862, 73.563)), module, Spatializer::LEDM_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(36.027, 73.563)), module, Spatializer::LEDR_LIGHT));
	}
};

Model* modelSpatializer = createModel<Spatializer, SpatializerWidget>("Spatializer");