#include "plugin.hpp"

struct SampleRateClock : Module {
	enum ParamId {
		FREQ_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		SQUARE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	SampleRateClock() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_PARAM, 0.f, 1.f, 0.f, "Freq");
		configOutput(SQUARE_OUTPUT, "Square");
	}

	float phase = 0;


	void process(const ProcessArgs& args) override {
		float freq = 1400.f * std::pow(16.f / 1.4f, params[FREQ_PARAM].getValue());
		phase += freq * args.sampleTime;
		if (phase >= 1.f)
			phase -= 1.f;
		outputs[SQUARE_OUTPUT].setVoltage(phase < 0.5f ? 5.f : -5.f);
	}
};


struct SampleRateClockWidget : ModuleWidget {
	SampleRateClockWidget(SampleRateClock* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/SampleRateClock_info.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(5.204, 23.725)), module, SampleRateClock::FREQ_PARAM));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.469, 97.544)), module, SampleRateClock::SQUARE_OUTPUT));
	}
};


Model* modelSampleRateClock = createModel<SampleRateClock, SampleRateClockWidget>("SampleRateClock");