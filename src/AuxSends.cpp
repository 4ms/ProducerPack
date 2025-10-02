#include "plugin.hpp"

struct AuxSends : Module {
	enum ParamId {
		PREPOST1_PARAM,
		ASEND_PARAM,
		PREPOST2_PARAM,
		BSEND_PARAM,
		PREPOST3_PARAM,
		CSEND_PARAM,
		ARETURN_PARAM,
		BRETURN_PARAM,
		CRETURN_PARAM,
		DRYLEVEL_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		ASENDCVIN_INPUT,
		BSENDCVIN_INPUT,
		CSENDCVIN_INPUT,
		ARETURNCVIN_INPUT,
		BRETURNCVIN_INPUT,
		CRETURNCVIN_INPUT,
		ARETURNLEFTIN_INPUT,
		ARETURNRIGHTIN_INPUT,
		BRETURNLEFTIN_INPUT,
		BRETURNRIGHTIN_INPUT,
		CRETURNLEFTIN_INPUT,
		CRETURNRIGHTIN_INPUT,
		AUDIOLEFTIN_INPUT,
		AUDIORIGHTIN_INPUT,
		DRYLEVELCVIN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ASENDLEFTOUT_OUTPUT,
		ASENDRIGHTOUT_OUTPUT,
		BSENDLEFTOUT_OUTPUT,
		BSENDRIGHTOUT_OUTPUT,
		CSENDLEFTOUT_OUTPUT,
		CSENDRIGHTOUT_OUTPUT,
		AUDIOLEFTOUT_OUTPUT,
		AUDIORIGHTOUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ASENDLED_LIGHT,
		BSENDLED_LIGHT,
		CSENDLED_LIGHT,
		ARETURNLED_LIGHT,
		BRETURNLED_LIGHT,
		CRETURNLED_LIGHT,
		LIGHTS_LEN
	};

	AuxSends() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(ASEND_PARAM, 0.f, 1.f, 0.5f, "Send A", "%", 0.f, 100.f);
		configParam(BSEND_PARAM, 0.f, 1.f, 0.5f, "Send B", "%", 0.f, 100.f);
		configParam(CSEND_PARAM, 0.f, 1.f, 0.5f, "Send C", "%", 0.f, 100.f);
		configParam(ARETURN_PARAM, 0.f, 1.f, 0.5f, "Return A", "%", 0.f, 100.f);
		configParam(BRETURN_PARAM, 0.f, 1.f, 0.5f, "Return B", "%", 0.f, 100.f);
		configParam(CRETURN_PARAM, 0.f, 1.f, 0.5f, "Return C", "%", 0.f, 100.f);
		configParam(DRYLEVEL_PARAM, 0.f, 1.f, 1.f, "Dry Level", "%", 0.f, 100.f);

		configSwitch(PREPOST1_PARAM, 0.f, 1.f, 0.f, "Pre/Post A", {"Post fader", "Pre fader"});
		configSwitch(PREPOST2_PARAM, 0.f, 1.f, 0.f, "Pre/Post B", {"Post fader", "Pre fader"});
		configSwitch(PREPOST3_PARAM, 0.f, 1.f, 0.f, "Pre/Post C", {"Post fader", "Pre fader"});

		configInput(ASENDCVIN_INPUT, "Send A CV");
		configInput(BSENDCVIN_INPUT, "Send B CV");
		configInput(CSENDCVIN_INPUT, "Send C CV");
		configInput(ARETURNCVIN_INPUT, "Return A CV");
		configInput(BRETURNCVIN_INPUT, "Return B CV");
		configInput(CRETURNCVIN_INPUT, "Return C CV");
		configInput(ARETURNLEFTIN_INPUT, "Return A Left");
		configInput(ARETURNRIGHTIN_INPUT, "Return A Right");
		configInput(BRETURNLEFTIN_INPUT, "Return B Left");
		configInput(BRETURNRIGHTIN_INPUT, "Return B Right");
		configInput(CRETURNLEFTIN_INPUT, "Return C Left");
		configInput(CRETURNRIGHTIN_INPUT, "Return C Right");
		configInput(AUDIOLEFTIN_INPUT, "Audio Left");
		configInput(AUDIORIGHTIN_INPUT, "Audio Right");
		configInput(DRYLEVELCVIN_INPUT, "Dry Level CV");
		configOutput(ASENDLEFTOUT_OUTPUT, "Send A Left");
		configOutput(ASENDRIGHTOUT_OUTPUT, "Send A Right");
		configOutput(BSENDLEFTOUT_OUTPUT, "Send B Left");
		configOutput(BSENDRIGHTOUT_OUTPUT, "Send B Right");
		configOutput(CSENDLEFTOUT_OUTPUT, "Send C Left");
		configOutput(CSENDRIGHTOUT_OUTPUT, "Send C Right");
		configOutput(AUDIOLEFTOUT_OUTPUT, "Audio Left");
		configOutput(AUDIORIGHTOUT_OUTPUT, "Audio Right");
	}
	void process(const ProcessArgs &args) override {
		// --- Read raw audio input ---
		float rawInL = inputs[AUDIOLEFTIN_INPUT].getVoltage();
		float rawInR = inputs[AUDIORIGHTIN_INPUT].isConnected() ? inputs[AUDIORIGHTIN_INPUT].getVoltage() : rawInL;

		// --- Dry level ---
		float dryCV = std::clamp(inputs[DRYLEVELCVIN_INPUT].getVoltage(), -5.f, 5.f) / 5.f;
		float dryLevel = std::clamp(params[DRYLEVEL_PARAM].getValue() + dryCV, 0.f, 1.f);
		float inL = rawInL * dryLevel;
		float inR = rawInR * dryLevel;

		float outL = inL;
		float outR = inR;

		// --- Shared CV helpers (cached values) ---
		auto computeCV = [](Input &in) {
			return std::clamp(in.getVoltage(), -5.f, 5.f) / 5.f;
		};

		// === SEND A ===
		if (outputs[ASENDLEFTOUT_OUTPUT].isConnected() || outputs[ASENDRIGHTOUT_OUTPUT].isConnected()) {
			float sendCV = computeCV(inputs[ASENDCVIN_INPUT]);
			float sendA = std::clamp(params[ASEND_PARAM].getValue() + sendCV, 0.f, 1.f);
			bool pre = params[PREPOST1_PARAM].getValue() > 0.5f;
			float sendL = (pre ? rawInL : inL) * sendA;
			float sendR = (pre ? rawInR : inR) * sendA;
			outputs[ASENDLEFTOUT_OUTPUT].setVoltage(sendL);
			outputs[ASENDRIGHTOUT_OUTPUT].setVoltage(sendR);
			lights[ASENDLED_LIGHT].setBrightnessSmooth(std::fabs(sendL + sendR) * 0.1f, args.sampleTime);
		}

		// === RETURN A ===
		if (inputs[ARETURNLEFTIN_INPUT].isConnected()) {
			float returnCV = computeCV(inputs[ARETURNCVIN_INPUT]);
			float gain = std::clamp(params[ARETURN_PARAM].getValue() + returnCV, 0.f, 1.f);
			float returnL = inputs[ARETURNLEFTIN_INPUT].getVoltage() * gain;
			float returnR =
				inputs[ARETURNRIGHTIN_INPUT].isConnected() ? inputs[ARETURNRIGHTIN_INPUT].getVoltage() * gain : returnL;
			outL += returnL;
			outR += returnR;
			lights[ARETURNLED_LIGHT].setBrightnessSmooth(std::fabs(returnL + returnR) * 0.1f, args.sampleTime);
		}

		// === SEND B ===
		if (outputs[BSENDLEFTOUT_OUTPUT].isConnected() || outputs[BSENDRIGHTOUT_OUTPUT].isConnected()) {
			float sendCV = computeCV(inputs[BSENDCVIN_INPUT]);
			float sendB = std::clamp(params[BSEND_PARAM].getValue() + sendCV, 0.f, 1.f);
			bool pre = params[PREPOST2_PARAM].getValue() > 0.5f;
			float sendL = (pre ? rawInL : inL) * sendB;
			float sendR = (pre ? rawInR : inR) * sendB;
			outputs[BSENDLEFTOUT_OUTPUT].setVoltage(sendL);
			outputs[BSENDRIGHTOUT_OUTPUT].setVoltage(sendR);
			lights[BSENDLED_LIGHT].setBrightnessSmooth(std::fabs(sendL + sendR) * 0.1f, args.sampleTime);
		}

		// === RETURN B ===
		if (inputs[BRETURNLEFTIN_INPUT].isConnected()) {
			float returnCV = computeCV(inputs[BRETURNCVIN_INPUT]);
			float gain = std::clamp(params[BRETURN_PARAM].getValue() + returnCV, 0.f, 1.f);
			float returnL = inputs[BRETURNLEFTIN_INPUT].getVoltage() * gain;
			float returnR =
				inputs[BRETURNRIGHTIN_INPUT].isConnected() ? inputs[BRETURNRIGHTIN_INPUT].getVoltage() * gain : returnL;
			outL += returnL;
			outR += returnR;
			lights[BRETURNLED_LIGHT].setBrightnessSmooth(std::fabs(returnL + returnR) * 0.1f, args.sampleTime);
		}

		// === SEND C ===
		if (outputs[CSENDLEFTOUT_OUTPUT].isConnected() || outputs[CSENDRIGHTOUT_OUTPUT].isConnected()) {
			float sendCV = computeCV(inputs[CSENDCVIN_INPUT]);
			float sendC = std::clamp(params[CSEND_PARAM].getValue() + sendCV, 0.f, 1.f);
			bool pre = params[PREPOST3_PARAM].getValue() > 0.5f;
			float sendL = (pre ? rawInL : inL) * sendC;
			float sendR = (pre ? rawInR : inR) * sendC;
			outputs[CSENDLEFTOUT_OUTPUT].setVoltage(sendL);
			outputs[CSENDRIGHTOUT_OUTPUT].setVoltage(sendR);
			lights[CSENDLED_LIGHT].setBrightnessSmooth(std::fabs(sendL + sendR) * 0.1f, args.sampleTime);
		}

		// === RETURN C ===
		if (inputs[CRETURNLEFTIN_INPUT].isConnected()) {
			float returnCV = computeCV(inputs[CRETURNCVIN_INPUT]);
			float gain = std::clamp(params[CRETURN_PARAM].getValue() + returnCV, 0.f, 1.f);
			float returnL = inputs[CRETURNLEFTIN_INPUT].getVoltage() * gain;
			float returnR =
				inputs[CRETURNRIGHTIN_INPUT].isConnected() ? inputs[CRETURNRIGHTIN_INPUT].getVoltage() * gain : returnL;
			outL += returnL;
			outR += returnR;
			lights[CRETURNLED_LIGHT].setBrightnessSmooth(std::fabs(returnL + returnR) * 0.1f, args.sampleTime);
		}

		// Final output
		outputs[AUDIOLEFTOUT_OUTPUT].setVoltage(clamp(outL, -10.f, 10.f));
		outputs[AUDIORIGHTOUT_OUTPUT].setVoltage(clamp(outR, -10.f, 10.f));
	}
};

struct AuxSendsWidget : ModuleWidget {
	AuxSendsWidget(AuxSends *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/AuxSends_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Knob9mm>(mm2px(Vec(17.681, 13.501)), module, AuxSends::ASEND_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(40.678, 13.501)), module, AuxSends::BSEND_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(63.58, 13.501)), module, AuxSends::CSEND_PARAM));

		addParam(createParamCentered<Switch2Pos>(mm2px(Vec(6.699, 14.702)), module, AuxSends::PREPOST1_PARAM));
		addParam(createParamCentered<Switch2Pos>(mm2px(Vec(29.7, 14.702)), module, AuxSends::PREPOST2_PARAM));
		addParam(createParamCentered<Switch2Pos>(mm2px(Vec(52.701, 14.702)), module, AuxSends::PREPOST3_PARAM));

		addParam(createParamCentered<Knob9mm>(mm2px(Vec(12.679, 61.499)), module, AuxSends::ARETURN_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(35.581, 61.499)), module, AuxSends::BRETURN_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(58.578, 61.499)), module, AuxSends::CRETURN_PARAM));
		addParam(createParamCentered<Knob9mm>(mm2px(Vec(28.998, 111.0)), module, AuxSends::DRYLEVEL_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 30.998)), module, AuxSends::ASENDCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35.581, 30.998)), module, AuxSends::BSENDCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(58.599, 30.998)), module, AuxSends::CSENDCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 79.0)), module, AuxSends::ARETURNCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35.549, 79.0)), module, AuxSends::BRETURNCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(58.599, 79.0)), module, AuxSends::CRETURNCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.549, 94.998)), module, AuxSends::ARETURNLEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(17.55, 94.998)), module, AuxSends::ARETURNRIGHTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.55, 94.998)), module, AuxSends::BRETURNLEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(40.548, 94.998)), module, AuxSends::BRETURNRIGHTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(53.551, 94.998)), module, AuxSends::CRETURNLEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(63.559, 94.998)), module, AuxSends::CRETURNRIGHTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.8, 111.0)), module, AuxSends::AUDIOLEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(17.6, 111.0)), module, AuxSends::AUDIORIGHTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(40.498, 111.0)), module, AuxSends::DRYLEVELCVIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.56, 47.0)), module, AuxSends::ASENDLEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.561, 47.0)), module, AuxSends::ASENDRIGHTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.561, 47.0)), module, AuxSends::BSENDLEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(40.558, 47.0)), module, AuxSends::BSENDRIGHTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(53.558, 47.0)), module, AuxSends::CSENDLEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(63.559, 47.0)), module, AuxSends::CSENDRIGHTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(53.498, 111.0)), module, AuxSends::AUDIOLEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(63.7, 111.0)), module, AuxSends::AUDIORIGHTOUT_OUTPUT));

		addChild(
			createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.679, 22.5)), module, AuxSends::ASENDLED_LIGHT));
		addChild(
			createLightCentered<MediumLight<RedLight>>(mm2px(Vec(35.581, 22.5)), module, AuxSends::BSENDLED_LIGHT));
		addChild(
			createLightCentered<MediumLight<RedLight>>(mm2px(Vec(58.578, 22.5)), module, AuxSends::CSENDLED_LIGHT));
		addChild(
			createLightCentered<MediumLight<RedLight>>(mm2px(Vec(12.7, 70.498)), module, AuxSends::ARETURNLED_LIGHT));
		addChild(
			createLightCentered<MediumLight<RedLight>>(mm2px(Vec(35.598, 70.498)), module, AuxSends::BRETURNLED_LIGHT));
		addChild(
			createLightCentered<MediumLight<RedLight>>(mm2px(Vec(58.599, 70.498)), module, AuxSends::CRETURNLED_LIGHT));
	}
};

Model *modelAuxSends = createModel<AuxSends, AuxSendsWidget>("AuxSends");
