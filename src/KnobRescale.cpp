#include "plugin.hpp"


struct KnobRescale : Module {
	enum ParamId {
		OFFSET_PARAM,
		THRESHOLD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		INV_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	KnobRescale() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(OFFSET_PARAM, -10.f, 10.f, 0.f, "Offset", "v");
		configParam(THRESHOLD_PARAM, -10.f, 10.f, 0.f, "Threshold", "v");
		configOutput(OUT_OUTPUT, "CV");
		configOutput(INV_OUTPUT, "Inverted");
	}

	void process(const ProcessArgs& args) override {
        const float offset = params[OFFSET_PARAM].getValue();
        const float threshold = params[THRESHOLD_PARAM].getValue();
    
        const float invRange = 1.f / (threshold + 10.f);
        float rescaled = (offset + 10.f) * invRange * 10.f - 5.f;
    
        if (rescaled > 5.f)
            rescaled = 5.f;
        else if (rescaled < -5.f)
            rescaled = -5.f;
    
        outputs[OUT_OUTPUT].setVoltage(rescaled);
		outputs[INV_OUTPUT].setVoltage(rescaled * -1.f);
    }   
};


struct KnobRescaleWidget : ModuleWidget {
	KnobRescaleWidget(KnobRescale* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/KnobRescale_info.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(5.08, 13.127)), module, KnobRescale::OFFSET_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(5.08, 29.719)), module, KnobRescale::THRESHOLD_PARAM));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 99.67)), module, KnobRescale::OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 115.881)), module, KnobRescale::INV_OUTPUT));
	}
};


Model* modelKnobRescale = createModel<KnobRescale, KnobRescaleWidget>("KnobRescale");