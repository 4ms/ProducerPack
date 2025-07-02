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
	
		void process(const ProcessArgs& args) override {
			float inL = inputs[INL_INPUT].getVoltage();
			float inR = inputs[INR_INPUT].isConnected() ? inputs[INR_INPUT].getVoltage() : inL;
		
			// Get the current time parameter value and adjust based on CV input if connected
			float targetTime = clamp(params[TIME_PARAM].getValue() + (inputs[TIMECV_INPUT].isConnected() ? inputs[TIMECV_INPUT].getVoltage() / 10.f : 0.f), 0.f, 1.f);
			slewedTime += (targetTime - slewedTime) * clamp(timeSlewRate, 0.f, 1.f);
		
			// Range Switch: check if we're in milliseconds or samples mode
			bool isMillisecondsMode = params[RANGE_PARAM].getValue() < 0.5f; // < 0.5 means milliseconds
			float time = slewedTime;
		
			int delaySamples = 0;
			if (isMillisecondsMode) {
				// Milliseconds Mode: map time to 1ms to 30ms
				float timeMs = rescale(time, 0.f, 1.f, 1.f, 30.f);
				delaySamples = clamp((int)(timeMs * args.sampleRate * 0.001f), 1, maxDelaySamples - 1);
			} else {
				// Samples Mode: map time to 1 to 50 samples
				delaySamples = clamp((int)(rescale(time, 0.f, 1.f, 1.f, 50.f)), 1, maxDelaySamples - 1);
			}
		
			// Width and Pan Calculation
			float width = clamp(params[WIDTH_PARAM].getValue() + (inputs[WIDTHCV_INPUT].isConnected() ? inputs[WIDTHCV_INPUT].getVoltage() / 10.f : 0.f), 0.f, 1.f);
			bool isStereo = inputs[INR_INPUT].isConnected();
			
			// Mid/Side Mix Calculation
			float mix = clamp(params[MIDSIDE_PARAM].getValue() + (inputs[MIDSIDECV_INPUT].isConnected() ? inputs[MIDSIDECV_INPUT].getVoltage() / 10.f : 0.f), 0.f, 1.f);
			
			// Delay Line Write
			delayBufferL[delayIndex] = inL;
			delayBufferR[delayIndex] = isStereo ? inR : inL;
			
			// Read Delayed Samples
			int readIndex = delayIndex - delaySamples;
			if (readIndex < 0) readIndex += maxDelaySamples;
			float delayedL = delayBufferL[readIndex];
			float delayedR = -delayBufferR[readIndex];
			delayIndex = (delayIndex + 1) % maxDelaySamples;
			
			// Wet Signal Panning
			float wetL = 0.f;
			float wetR = 0.f;
		
			if (isStereo) {
				// Stereo mode: same as before
				float panWidth = (width <= 0.5f) ? (width / 0.5f) : 1.f;
				wetL = delayedL * (0.5f * (1.f - panWidth) + panWidth) + delayedR * 0.5f * (1.f - panWidth);
				wetR = delayedR * (0.5f * (1.f - panWidth) + panWidth) + delayedL * 0.5f * (1.f - panWidth);
			} else {
				// Mono: Apply width to spatialize L and R delay lines
				float centerAmt = 1.f - width;
				float sideAmt = width;
		
				// DelayL → more L as width increases
				// DelayR → more R as width increases
				wetL = delayedL * (0.5f * centerAmt + sideAmt);
				wetR = delayedR * (0.5f * centerAmt + sideAmt);
			}
		
			// --- Send Outputs ---
			if (isStereo) {
				// Stereo Mode
				outputs[SENDL_OUTPUT].setVoltage(wetL);
				outputs[SENDR_OUTPUT].setVoltage(wetR);
				outputs[SENDM_OUTPUT].setVoltage((inL + inR) * 0.5f);  // Dry sum, halved
			}
			else {
				// Mono Mode
				outputs[SENDL_OUTPUT].setVoltage(wetL);
				outputs[SENDR_OUTPUT].setVoltage(wetR);
				outputs[SENDM_OUTPUT].setVoltage(inL);
			}
		
			// Mid Signal Calculation
			float dryM = inL + inR;
			dryM *= 0.33f;
			float wetM = wetL + wetR;
			float midSignal = crossfade(dryM, wetM, mix);
		
			// Return Signals
			float returnL = inputs[RETURNL_INPUT].isConnected() ? inputs[RETURNL_INPUT].getVoltage() : wetL;
			float returnR = inputs[RETURNR_INPUT].isConnected() ? inputs[RETURNR_INPUT].getVoltage() : wetR;
			float returnM = inputs[RETURNM_INPUT].isConnected() ? inputs[RETURNM_INPUT].getVoltage() : (isStereo ? (returnL + returnR) * 0.5f : inL);
			midSignal = inputs[RETURNM_INPUT].isConnected() ? returnM : midSignal;
		
			// Final Output Mixing
			float wetLFinal = inputs[RETURNL_INPUT].isConnected() ? returnL : wetL;
			float wetRFinal = inputs[RETURNR_INPUT].isConnected() ? returnR : wetR;
			float outL = crossfade(midSignal, wetLFinal, mix);
			float outR = crossfade(midSignal, wetRFinal, mix);
			outputs[OUTL_OUTPUT].setVoltage(outL);
			outputs[OUTR_OUTPUT].setVoltage(outR);
		
			// LED Signal Display
			float attenuator = 1.f / 5.f;
			float leftSignal = fabs(returnL) * attenuator;
			float rightSignal = fabs(returnR) * attenuator;
			float midSignalLevel = fabs(midSignal) * attenuator;
			float midLedBrightness = (1.f - mix) * 1.0f;
			lights[LEDM_LIGHT].setBrightnessSmooth((midSignalLevel * midLedBrightness) * 2.f, args.sampleTime);
		
			float sideLedBrightness = mix;
			lights[LEDL_LIGHT].setBrightnessSmooth((leftSignal * sideLedBrightness) * 2.f, args.sampleTime);
			lights[LEDR_LIGHT].setBrightnessSmooth((rightSignal * sideLedBrightness) * 2.f, args.sampleTime);
		}		
};


struct SpatializerWidget : ModuleWidget {
	SpatializerWidget(Spatializer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Spatializer_info.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_2Pos>(mm2px(Vec(9.206, 19.076)), module, Spatializer::RANGE_PARAM));
		addParam(createParamCentered<Davies1900hLargeBlackKnob>(mm2px(Vec(30.514, 19.757)), module, Spatializer::MIDSIDE_PARAM));
		addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(13.507, 41.317)), module, Spatializer::TIME_PARAM));
		addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(47.522, 41.317)), module, Spatializer::WIDTH_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(13.507, 60.475)), module, Spatializer::TIMECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.514, 60.475)), module, Spatializer::MIDSIDECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.522, 60.475)), module, Spatializer::WIDTHCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.103, 97.056)), module, Spatializer::RETURNL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.514, 97.056)), module, Spatializer::RETURNM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(53.024, 97.056)), module, Spatializer::RETURNR_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.103, 111.049)), module, Spatializer::INL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.99, 111.049)), module, Spatializer::INR_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.103, 82.035)), module, Spatializer::SENDL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.514, 82.035)), module, Spatializer::SENDM_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(53.024, 82.035)), module, Spatializer::SENDR_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.176, 111.049)), module, Spatializer::OUTL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(53.024, 111.049)), module, Spatializer::OUTR_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(8.103, 70.529)), module, Spatializer::LEDL_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(30.514, 70.529)), module, Spatializer::LEDM_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(53.024, 70.529)), module, Spatializer::LEDR_LIGHT));
	}
};

Model* modelSpatializer = createModel<Spatializer, SpatializerWidget>("Spatializer");