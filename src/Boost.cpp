#include "plugin.hpp"

struct Boost : Module {
	enum ParamId { GAIN_PARAM, RANGE_PARAM, VOLUME_PARAM, PARAMS_LEN };
	enum InputId { LEFTIN_INPUT, RIGHTIN_INPUT, INPUTS_LEN };
	enum OutputId { LEFTOUT_OUTPUT, RIGHTOUT_OUTPUT, OUTPUTS_LEN };
	enum LightId { RIGHTLEDRED_LIGHT, RIGHTLEDGREEN_LIGHT, LIGHTS_LEN };

	Boost() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(GAIN_PARAM, 0.f, 1.f, 1.f, "Gain", "%", 0.f, 100.f);
		configSwitch(RANGE_PARAM, 0.f, 2.f, 0.f, "Gain Range", {"1x", "5x", "100x"});
		configParam(VOLUME_PARAM, 0.f, 1.f, 1.f, "Volume", "%", 0.f, 100.f);
		configInput(LEFTIN_INPUT, "Audio Left");
		configInput(RIGHTIN_INPUT, "Audio Right");
		configOutput(LEFTOUT_OUTPUT, "Audio Left");
		configOutput(RIGHTOUT_OUTPUT, "Audio Right");

		configBypass(LEFTIN_INPUT, LEFTOUT_OUTPUT);
		configBypass(RIGHTIN_INPUT, RIGHTOUT_OUTPUT);
	}

	// Cached values
	float cachedGainParam = -1.f;
	float cachedVolume = -1.f;
	int cachedRangeSelection = -1;
	float preVolumeGain = 1.f;

	void process(const ProcessArgs &args) override {
		const float gainParam = params[GAIN_PARAM].getValue();
		const float volume = params[VOLUME_PARAM].getValue();
		const int rangeSelection = (int)params[RANGE_PARAM].getValue();

		if (gainParam != cachedGainParam || rangeSelection != cachedRangeSelection) {
			float gainMultiplier = 1.f;
			switch (rangeSelection) {
				case 1: gainMultiplier = 5.f; break;
				case 2: gainMultiplier = 100.f; break;
			}
			preVolumeGain = (rangeSelection == 0) ? gainParam * gainMultiplier
												  : 1.f + gainParam * (gainMultiplier - 1.f);
			cachedGainParam = gainParam;
			cachedRangeSelection = rangeSelection;
		}

		if (volume != cachedVolume)
			cachedVolume = volume;

		const bool rightConnected = inputs[RIGHTIN_INPUT].isConnected();
		const int nL = inputs[LEFTIN_INPUT].getChannels();
		const int nR = rightConnected ? inputs[RIGHTIN_INPUT].getChannels() : nL;
		const int n = std::max(nL, nR);

		outputs[LEFTOUT_OUTPUT].setChannels(n);
		outputs[RIGHTOUT_OUTPUT].setChannels(n);

		bool anyClipping = false;
		float maxOutR = 0.f;

		for (int c = 0; c < n; c++) {
			const float inL = inputs[LEFTIN_INPUT].getPolyVoltage(c);
			const float inR = rightConnected ? inputs[RIGHTIN_INPUT].getPolyVoltage(c) : inL;

			const float outL = std::clamp(inL * preVolumeGain, -5.f, 5.f) * cachedVolume;
			const float boostedR = inR * preVolumeGain;
			const float outR = std::clamp(boostedR, -5.f, 5.f) * cachedVolume;

			if (boostedR < -5.f || boostedR > 5.f)
				anyClipping = true;
			maxOutR = std::max(maxOutR, std::fabs(outR));

			outputs[LEFTOUT_OUTPUT].setVoltage(outL, c);
			outputs[RIGHTOUT_OUTPUT].setVoltage(outR, c);
		}

		const float brightnessR = std::clamp(maxOutR * 0.2f, 0.f, 1.f);
		lights[RIGHTLEDRED_LIGHT].setBrightnessSmooth(anyClipping ? brightnessR : 0.f, args.sampleTime);
		lights[RIGHTLEDGREEN_LIGHT].setBrightnessSmooth(anyClipping ? 0.f : brightnessR, args.sampleTime);
	}
};

struct BoostWidget : ModuleWidget {
	BoostWidget(Boost *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Boost_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Knob9mm>(mm2px(Vec(10.16, 15.501)), module, Boost::GAIN_PARAM));
		addParam(createParam<Switch3PosHorizontal>(mm2px(Vec(6.3, 24.501)), module, Boost::RANGE_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(10.16, 42.5)), module, Boost::VOLUME_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 59.5)), module, Boost::LEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 76.501)), module, Boost::RIGHTIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 93.501)), module, Boost::LEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 110.001)), module, Boost::RIGHTOUT_OUTPUT));

		addChild(
			createLightCentered<MediumLight<RedLight>>(mm2px(Vec(4.801, 103.503)), module, Boost::RIGHTLEDRED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(
			mm2px(Vec(4.801, 103.503)), module, Boost::RIGHTLEDGREEN_LIGHT));
	}
};

Model *modelBoost = createModel<Boost, BoostWidget>("Boost");
