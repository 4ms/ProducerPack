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

	unsigned lightThrottle = 0;
	static constexpr unsigned lightThrottleAmount = 256;
	static_assert(((lightThrottleAmount - 1) & lightThrottleAmount) == 0, "lightThrottleAmount must be a power of 2");
	static_assert(lightThrottleAmount > 1);

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

		configBypass(AUDIOLEFTIN_INPUT, AUDIOLEFTOUT_OUTPUT);
		configBypass(AUDIOLEFTIN_INPUT, AUDIORIGHTOUT_OUTPUT);
	}

	void process(const ProcessArgs &args) override {
		// --- Channel count: max of main audio and all return inputs ---
		const bool rMainConn  = inputs[AUDIORIGHTIN_INPUT].isConnected();
		const bool aRetLConn  = inputs[ARETURNLEFTIN_INPUT].isConnected();
		const bool aRetRConn  = inputs[ARETURNRIGHTIN_INPUT].isConnected();
		const bool bRetLConn  = inputs[BRETURNLEFTIN_INPUT].isConnected();
		const bool bRetRConn  = inputs[BRETURNRIGHTIN_INPUT].isConnected();
		const bool cRetLConn  = inputs[CRETURNLEFTIN_INPUT].isConnected();
		const bool cRetRConn  = inputs[CRETURNRIGHTIN_INPUT].isConnected();

		const int nMainL = inputs[AUDIOLEFTIN_INPUT].getChannels();
		const int nMainR = rMainConn ? inputs[AUDIORIGHTIN_INPUT].getChannels() : nMainL;
		int n = std::max(nMainL, nMainR);
		if (aRetLConn) n = std::max(n, inputs[ARETURNLEFTIN_INPUT].getChannels());
		if (aRetRConn) n = std::max(n, inputs[ARETURNRIGHTIN_INPUT].getChannels());
		if (bRetLConn) n = std::max(n, inputs[BRETURNLEFTIN_INPUT].getChannels());
		if (bRetRConn) n = std::max(n, inputs[BRETURNRIGHTIN_INPUT].getChannels());
		if (cRetLConn) n = std::max(n, inputs[CRETURNLEFTIN_INPUT].getChannels());
		if (cRetRConn) n = std::max(n, inputs[CRETURNRIGHTIN_INPUT].getChannels());

		outputs[ASENDLEFTOUT_OUTPUT].setChannels(n);
		outputs[ASENDRIGHTOUT_OUTPUT].setChannels(n);
		outputs[BSENDLEFTOUT_OUTPUT].setChannels(n);
		outputs[BSENDRIGHTOUT_OUTPUT].setChannels(n);
		outputs[CSENDLEFTOUT_OUTPUT].setChannels(n);
		outputs[CSENDRIGHTOUT_OUTPUT].setChannels(n);
		outputs[AUDIOLEFTOUT_OUTPUT].setChannels(n);
		outputs[AUDIORIGHTOUT_OUTPUT].setChannels(n);

		// --- Mono CVs: precomputed once, shared across all poly channels ---
		auto computeCVKnobSum = [](Input &in, Param &param) {
			return std::clamp(param.getValue() + in.getVoltage() / 5.f, 0.f, 1.f);
		};

		float dryCV   = std::clamp(inputs[DRYLEVELCVIN_INPUT].getVoltage(), -5.f, 5.f) / 5.f;
		float dryLevel = std::clamp(params[DRYLEVEL_PARAM].getValue() + dryCV, 0.f, 1.f);

		const bool aSendConn = outputs[ASENDLEFTOUT_OUTPUT].isConnected() || outputs[ASENDRIGHTOUT_OUTPUT].isConnected();
		const bool bSendConn = outputs[BSENDLEFTOUT_OUTPUT].isConnected() || outputs[BSENDRIGHTOUT_OUTPUT].isConnected();
		const bool cSendConn = outputs[CSENDLEFTOUT_OUTPUT].isConnected() || outputs[CSENDRIGHTOUT_OUTPUT].isConnected();

		const float sendALevel = aSendConn ? computeCVKnobSum(inputs[ASENDCVIN_INPUT], params[ASEND_PARAM]) : 0.f;
		const float sendBLevel = bSendConn ? computeCVKnobSum(inputs[BSENDCVIN_INPUT], params[BSEND_PARAM]) : 0.f;
		const float sendCLevel = cSendConn ? computeCVKnobSum(inputs[CSENDCVIN_INPUT], params[CSEND_PARAM]) : 0.f;
		const bool preA = params[PREPOST1_PARAM].getValue() > 0.5f;
		const bool preB = params[PREPOST2_PARAM].getValue() > 0.5f;
		const bool preC = params[PREPOST3_PARAM].getValue() > 0.5f;
		const float gainRetA = aRetLConn ? computeCVKnobSum(inputs[ARETURNCVIN_INPUT], params[ARETURN_PARAM]) : 0.f;
		const float gainRetB = bRetLConn ? computeCVKnobSum(inputs[BRETURNCVIN_INPUT], params[BRETURN_PARAM]) : 0.f;
		const float gainRetC = cRetLConn ? computeCVKnobSum(inputs[CRETURNCVIN_INPUT], params[CRETURN_PARAM]) : 0.f;

		// LED accumulators (max across poly channels, updated on throttle)
		float maxSendA = 0.f, maxRetA = 0.f;
		float maxSendB = 0.f, maxRetB = 0.f;
		float maxSendC = 0.f, maxRetC = 0.f;

		// --- Per poly channel ---
		for (int c = 0; c < n; c++) {
			float rawInL = inputs[AUDIOLEFTIN_INPUT].getPolyVoltage(c);
			float rawInR = rMainConn ? inputs[AUDIORIGHTIN_INPUT].getPolyVoltage(c) : rawInL;
			float inL = rawInL * dryLevel;
			float inR = rawInR * dryLevel;
			float outL = inL;
			float outR = inR;

			// === SEND A ===
			if (aSendConn) {
				float sendL = (preA ? rawInL : inL) * sendALevel;
				float sendR = (preA ? rawInR : inR) * sendALevel;
				outputs[ASENDLEFTOUT_OUTPUT].setVoltage(sendL, c);
				outputs[ASENDRIGHTOUT_OUTPUT].setVoltage(sendR, c);
				maxSendA = std::max(maxSendA, std::fabs(sendL + sendR));
			}

			// === RETURN A ===
			if (aRetLConn) {
				float returnL = inputs[ARETURNLEFTIN_INPUT].getPolyVoltage(c) * gainRetA;
				float returnR = aRetRConn ? inputs[ARETURNRIGHTIN_INPUT].getPolyVoltage(c) * gainRetA : returnL;
				outL += returnL;
				outR += returnR;
				maxRetA = std::max(maxRetA, std::fabs(returnL + returnR));
			}

			// === SEND B ===
			if (bSendConn) {
				float sendL = (preB ? rawInL : inL) * sendBLevel;
				float sendR = (preB ? rawInR : inR) * sendBLevel;
				outputs[BSENDLEFTOUT_OUTPUT].setVoltage(sendL, c);
				outputs[BSENDRIGHTOUT_OUTPUT].setVoltage(sendR, c);
				maxSendB = std::max(maxSendB, std::fabs(sendL + sendR));
			}

			// === RETURN B ===
			if (bRetLConn) {
				float returnL = inputs[BRETURNLEFTIN_INPUT].getPolyVoltage(c) * gainRetB;
				float returnR = bRetRConn ? inputs[BRETURNRIGHTIN_INPUT].getPolyVoltage(c) * gainRetB : returnL;
				outL += returnL;
				outR += returnR;
				maxRetB = std::max(maxRetB, std::fabs(returnL + returnR));
			}

			// === SEND C ===
			if (cSendConn) {
				float sendL = (preC ? rawInL : inL) * sendCLevel;
				float sendR = (preC ? rawInR : inR) * sendCLevel;
				outputs[CSENDLEFTOUT_OUTPUT].setVoltage(sendL, c);
				outputs[CSENDRIGHTOUT_OUTPUT].setVoltage(sendR, c);
				maxSendC = std::max(maxSendC, std::fabs(sendL + sendR));
			}

			// === RETURN C ===
			if (cRetLConn) {
				float returnL = inputs[CRETURNLEFTIN_INPUT].getPolyVoltage(c) * gainRetC;
				float returnR = cRetRConn ? inputs[CRETURNRIGHTIN_INPUT].getPolyVoltage(c) * gainRetC : returnL;
				outL += returnL;
				outR += returnR;
				maxRetC = std::max(maxRetC, std::fabs(returnL + returnR));
			}

			outputs[AUDIOLEFTOUT_OUTPUT].setVoltage(clamp(outL, -10.f, 10.f), c);
			outputs[AUDIORIGHTOUT_OUTPUT].setVoltage(clamp(outR, -10.f, 10.f), c);
		}

		// --- LEDs (throttled, max across all poly channels) ---
		if (lightThrottle == 0) {
			lights[ASENDLED_LIGHT].setBrightnessSmooth(maxSendA * 0.1f, args.sampleTime * lightThrottleAmount);
			lights[ARETURNLED_LIGHT].setBrightnessSmooth(maxRetA * 0.1f, args.sampleTime * lightThrottleAmount);
			lights[BSENDLED_LIGHT].setBrightnessSmooth(maxSendB * 0.1f, args.sampleTime * lightThrottleAmount);
			lights[BRETURNLED_LIGHT].setBrightnessSmooth(maxRetB * 0.1f, args.sampleTime * lightThrottleAmount);
			lights[CSENDLED_LIGHT].setBrightnessSmooth(maxSendC * 0.1f, args.sampleTime * lightThrottleAmount);
			lights[CRETURNLED_LIGHT].setBrightnessSmooth(maxRetC * 0.1f, args.sampleTime * lightThrottleAmount);
		}

		lightThrottle = (lightThrottle + 1) & (lightThrottleAmount - 1);
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
