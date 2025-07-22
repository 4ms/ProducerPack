#include "plugin.hpp"
#include "KayOneSamples.h"

struct KayOne : Module {
	enum ParamId {
		SPEED_PARAM,
		LENGTH_PARAM,
		LOOP_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SPEEDCVIN_INPUT,
		LENGTHCVIN_INPUT,
		LOOPCVIN_INPUT,
		KICKTRIGIN_INPUT,
		SNARETRIGIN_INPUT,
		TOMLTRIG_INPUT,
		TOMHTRIG_INPUT,
		CLTRIG_INPUT,
		OHTRIG_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		KICKOUT_OUTPUT,
		SNAREOUT_OUTPUT,
		TOMLOUT_OUTPUT,
		TOMHOUT_OUTPUT,
		CLOUT_OUTPUT,
		OHOUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	struct Voice {
		float samplePos = 0.f;
		bool playing = false;
		bool lastTriggerState = false;
		const unsigned char* rawData = nullptr;
		int sampleLength = 0;
		int outputId = 0;

		int16_t readSample16(int index) {
			return (int16_t)(rawData[2 * index] | (rawData[2 * index + 1] << 8));
		}
	};

	Voice kickVoice;
	Voice snareVoice;
	Voice tomLoVoice;
	Voice tomHiVoice;
	Voice closedHatVoice;
	Voice openHatVoice;

	const float SPEED_LOW = 0.1f;
	const float SPEED_HIGH = 2.0f;
	const float LENGTH_MIN = 0.1f;
	const float LENGTH_MAX = 1.0f;

	KayOne() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(SPEED_PARAM, 0.f, 1.f, 0.5f, "Speed", "%", 0.f, 100.f);
		configParam(LENGTH_PARAM, 0.f, 1.f, 1.f, "Length", "%", 0.f, 100.f);
		configSwitch(LOOP_PARAM, 0.f, 1.f, 0.f, "Loop", {"Off", "On"});

		configInput(SPEEDCVIN_INPUT, "Speed CV");
		configInput(LENGTHCVIN_INPUT, "Length CV");
		configInput(LOOPCVIN_INPUT, "Loop Gate");

		configInput(KICKTRIGIN_INPUT, "Kick Trig");
		configInput(SNARETRIGIN_INPUT, "Snare Trig");
		configInput(TOMLTRIG_INPUT, "Tom Lo Trig");
		configInput(TOMHTRIG_INPUT, "Tom Hi Trig");
		configInput(CLTRIG_INPUT, "Closed Hat Trig");
		configInput(OHTRIG_INPUT, "Open Hat Trig");

		configOutput(KICKOUT_OUTPUT, "Kick");
		configOutput(SNAREOUT_OUTPUT, "Snare");
		configOutput(TOMLOUT_OUTPUT, "Tom Lo");
		configOutput(TOMHOUT_OUTPUT, "Tom Hi");
		configOutput(CLOUT_OUTPUT, "Closed Hat");
		configOutput(OHOUT_OUTPUT, "Open Hat");

		kickVoice.rawData = Kick_raw;
		kickVoice.sampleLength = sizeof(Kick_raw) / 2;
		kickVoice.outputId = KICKOUT_OUTPUT;

		snareVoice.rawData = Snare_raw;
		snareVoice.sampleLength = sizeof(Snare_raw) / 2;
		snareVoice.outputId = SNAREOUT_OUTPUT;

		tomLoVoice.rawData = TomLo_raw;
		tomLoVoice.sampleLength = sizeof(TomLo_raw) / 2;
		tomLoVoice.outputId = TOMLOUT_OUTPUT;

		tomHiVoice.rawData = TomHi_raw;
		tomHiVoice.sampleLength = sizeof(TomHi_raw) / 2;
		tomHiVoice.outputId = TOMHOUT_OUTPUT;

		closedHatVoice.rawData = ClosedHat_raw;
		closedHatVoice.sampleLength = sizeof(ClosedHat_raw) / 2;
		closedHatVoice.outputId = CLOUT_OUTPUT;

		openHatVoice.rawData = OpenHat_raw;
		openHatVoice.sampleLength = sizeof(OpenHat_raw) / 2;
		openHatVoice.outputId = OHOUT_OUTPUT;
	}

	void process(const ProcessArgs& args) override {
		// Speed calculation
		float knobSpeed = 0.01f + params[SPEED_PARAM].getValue() * (1.0f - 0.01f);
		float speedCV = clamp(inputs[SPEEDCVIN_INPUT].getVoltage(), -5.f, 5.f);
		float speedOffset = (speedCV / 5.f) * 0.5f;
		float normSpeed = clamp(knobSpeed + speedOffset, 0.01f, 1.0f);
		float speed = SPEED_LOW + (normSpeed - 0.01f) * ((SPEED_HIGH - SPEED_LOW) / (1.0f - 0.01f));

		// Length calculation
		float knobLength = params[LENGTH_PARAM].getValue();
		float lengthCV = clamp(inputs[LENGTHCVIN_INPUT].getVoltage(), -5.f, 5.f);
		float lengthOffset = (lengthCV / 5.f) * 0.5f;
		float normLength = clamp(knobLength + lengthOffset, 0.1f, 1.0f);
		float lengthRatio = LENGTH_MIN + (normLength - 0.1f) * ((LENGTH_MAX - LENGTH_MIN) / (1.0f - 0.1f));

		bool baseLoop = params[LOOP_PARAM].getValue() > 0.5f;
		float loopCV = inputs[LOOPCVIN_INPUT].getVoltage();
		bool loopEnabled = baseLoop;
		
		// Apply CV override logic
		if (!baseLoop && loopCV > 1.f) {
			loopEnabled = true;
		}
		else if (baseLoop && loopCV < -1.f) {
			loopEnabled = false;
		}
		
		// Process each voice
		processVoice(args, kickVoice, KICKTRIGIN_INPUT, KICKOUT_OUTPUT, speed, lengthRatio, loopEnabled);
		processVoice(args, snareVoice, SNARETRIGIN_INPUT, SNAREOUT_OUTPUT, speed, lengthRatio, loopEnabled);
		processVoice(args, tomLoVoice, TOMLTRIG_INPUT, TOMLOUT_OUTPUT, speed, lengthRatio, loopEnabled);
		processVoice(args, tomHiVoice, TOMHTRIG_INPUT, TOMHOUT_OUTPUT, speed, lengthRatio, loopEnabled);
		processVoice(args, closedHatVoice, CLTRIG_INPUT, CLOUT_OUTPUT, speed, lengthRatio, loopEnabled);
		processVoice(args, openHatVoice, OHTRIG_INPUT, OHOUT_OUTPUT, speed, lengthRatio, loopEnabled);		
	}

	void processVoice(const ProcessArgs& args, Voice& voice, int trigInputId, int outputId, float speed, float lengthRatio, bool loopEnabled) {
		bool trigger = inputs[trigInputId].getVoltage() > 1.0f;
	
		// Rising edge detection: start or restart voice
		if (trigger && !voice.lastTriggerState) {
			voice.playing = true;
			voice.samplePos = 0.f;
		}
		voice.lastTriggerState = trigger;
	
		// If loop is enabled and we're not already playing, auto-start
		if (loopEnabled && !voice.playing) {
			voice.playing = true;
			voice.samplePos = 0.f;
		}
	
		int maxSamplesToPlay = (int)(voice.sampleLength * lengthRatio);
	
		if (voice.playing) {
			int idx = (int)voice.samplePos;
			if (idx < maxSamplesToPlay) {
				int16_t sampleInt = voice.readSample16(idx);
				float sample = (float)sampleInt / 32768.f;
				outputs[outputId].setVoltage(sample * 5.f);
				voice.samplePos += speed;
			} else {
				if (loopEnabled) {
					voice.samplePos = 0.f; // Restart loop
				} else {
					voice.playing = false;
					outputs[outputId].setVoltage(0.f);
				}
			}
		} else {
			outputs[outputId].setVoltage(0.f);
		}
	}	
};

struct KayOneWidget : ModuleWidget {
	KayOneWidget(KayOne* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/KayOne_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(15.24, 15.958)), module, KayOne::SPEED_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(6.221, 35.67)), module, KayOne::LENGTH_PARAM));
		addParam(createParamCentered<_2Pos>(mm2px(Vec(23.632, 35.67)), module, KayOne::LOOP_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 29.153)), module, KayOne::SPEEDCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.221, 49.49)), module, KayOne::LENGTHCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.632, 49.49)), module, KayOne::LOOPCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.221, 63.771)), module, KayOne::KICKTRIGIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.221, 73.417)), module, KayOne::SNARETRIGIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.221, 83.302)), module, KayOne::TOMLTRIG_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.221, 93.19)), module, KayOne::TOMHTRIG_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.221, 103.551)), module, KayOne::CLTRIG_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.221, 114.064)), module, KayOne::OHTRIG_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.632, 63.771)), module, KayOne::KICKOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.632, 73.417)), module, KayOne::SNAREOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.632, 83.302)), module, KayOne::TOMLOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.632, 93.19)), module, KayOne::TOMHOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.632, 103.551)), module, KayOne::CLOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.632, 114.064)), module, KayOne::OHOUT_OUTPUT));
	}
};


Model* modelKayOne = createModel<KayOne, KayOneWidget>("KayOne");