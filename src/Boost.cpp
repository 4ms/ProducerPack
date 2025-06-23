#include "plugin.hpp"


struct Boost : Module {
	enum ParamId {
		GAIN_PARAM,
		RANGE_PARAM,
		VOLUME_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		LEFTIN_INPUT,
		RIGHTIN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		LEFTOUT_OUTPUT,
		RIGHTOUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LEFTLEDRED_LIGHT,
		LEFTLEDGREEN_LIGHT,
		RIGHTLEDRED_LIGHT,
		RIGHTLEDGREEN_LIGHT,
		LIGHTS_LEN
	};

	Boost() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(GAIN_PARAM, 0.f, 1.f, 0.f, "Gain", "%", 0.f, 100.f);
		configSwitch(RANGE_PARAM, 0.f, 2.f, 0.f, "Gain Range", {"1x", "5x", "100x"});
		configParam(VOLUME_PARAM, 0.f, 1.f, 0.f, "Volume", "%", 0.f, 100.f);
		configInput(LEFTIN_INPUT, "Audio Left");
		configInput(RIGHTIN_INPUT, "Audio Right");
		configOutput(LEFTOUT_OUTPUT, "Audio Left");
		configOutput(RIGHTOUT_OUTPUT, "Audio Right");
	}

	void process(const ProcessArgs& args) override {
		int rangeSelection = (int)params[RANGE_PARAM].getValue();
		float rangeMultiplier = 1.f;
		switch (rangeSelection) {
			case 0: rangeMultiplier = 1.f; break;
			case 1: rangeMultiplier = 5.f; break;
			case 2: rangeMultiplier = 100.f; break;
		}
	
		float gainAmount = params[GAIN_PARAM].getValue();
		float volumeAmount = params[VOLUME_PARAM].getValue();
		float preVolumeGain = gainAmount * rangeMultiplier;
	
		bool leftConnected = inputs[LEFTIN_INPUT].isConnected();
		bool rightConnected = inputs[RIGHTIN_INPUT].isConnected();
	
		if (leftConnected) {
			float in = inputs[LEFTIN_INPUT].getVoltage();
			float boosted = in * preVolumeGain;
			bool clipping = (boosted < -5.f || boosted > 5.f);
			float clipped = clamp(boosted, -5.f, 5.f);
			float out = clipped * volumeAmount;
			outputs[LEFTOUT_OUTPUT].setVoltage(out);
	
			float brightness = clamp(fabs(out) / 5.f, 0.f, 1.f);
			lights[LEFTLEDRED_LIGHT].setBrightnessSmooth(clipping ? brightness : 0.f, args.sampleTime);
			lights[LEFTLEDGREEN_LIGHT].setBrightnessSmooth(clipping ? 0.f : brightness, args.sampleTime);
	
			if (!rightConnected) {
				float boostedR = in * preVolumeGain;
				bool clippingR = (boostedR < -5.f || boostedR > 5.f);
				float clippedR = clamp(boostedR, -5.f, 5.f);
				float outR = clippedR * volumeAmount;
				outputs[RIGHTOUT_OUTPUT].setVoltage(outR);
	
				float brightnessR = clamp(fabs(outR) / 5.f, 0.f, 1.f);
				lights[RIGHTLEDRED_LIGHT].setBrightnessSmooth(clippingR ? brightnessR : 0.f, args.sampleTime);
				lights[RIGHTLEDGREEN_LIGHT].setBrightnessSmooth(clippingR ? 0.f : brightnessR, args.sampleTime);
			}
		} else {
			outputs[LEFTOUT_OUTPUT].setVoltage(0.f);
			lights[LEFTLEDRED_LIGHT].setBrightnessSmooth(0.f, args.sampleTime);
			lights[LEFTLEDGREEN_LIGHT].setBrightnessSmooth(0.f, args.sampleTime);
			if (!rightConnected) {
				outputs[RIGHTOUT_OUTPUT].setVoltage(0.f);
				lights[RIGHTLEDRED_LIGHT].setBrightnessSmooth(0.f, args.sampleTime);
				lights[RIGHTLEDGREEN_LIGHT].setBrightnessSmooth(0.f, args.sampleTime);
			}
		}
	
		if (rightConnected) {
			float in = inputs[RIGHTIN_INPUT].getVoltage();
			float boosted = in * preVolumeGain;
			bool clipping = (boosted < -5.f || boosted > 5.f);
			float clipped = clamp(boosted, -5.f, 5.f);
			float out = clipped * volumeAmount;
			outputs[RIGHTOUT_OUTPUT].setVoltage(out);
	
			float brightness = clamp(fabs(out) / 5.f, 0.f, 1.f);
			lights[RIGHTLEDRED_LIGHT].setBrightnessSmooth(clipping ? brightness : 0.f, args.sampleTime);
			lights[RIGHTLEDGREEN_LIGHT].setBrightnessSmooth(clipping ? 0.f : brightness, args.sampleTime);
		}
	}	
};


struct BoostWidget : ModuleWidget {
	BoostWidget(Boost* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Boost.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 20.41)), module, Boost::GAIN_PARAM));
		addParam(createParamCentered<CKSSThreeHorizontal>(mm2px(Vec(10.16, 44.576)), module, Boost::RANGE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 65.889)), module, Boost::VOLUME_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(4.273, 95.881)), module, Boost::LEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(4.273, 109.61)), module, Boost::RIGHTIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.402, 95.881)), module, Boost::LEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.402, 109.61)), module, Boost::RIGHTOUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(4.273, 88)), module, Boost::LEFTLEDRED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(4.273, 88)), module, Boost::LEFTLEDGREEN_LIGHT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(15.402, 88)), module, Boost::RIGHTLEDRED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(15.402, 88)), module, Boost::RIGHTLEDGREEN_LIGHT));
	}
};


Model* modelBoost = createModel<Boost, BoostWidget>("Boost");