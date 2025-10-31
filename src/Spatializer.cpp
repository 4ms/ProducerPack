#include "plugin.hpp"

struct Spatializer : Module {
	enum ParamId { RANGE_PARAM, TIME_PARAM, WIDTH_PARAM, MIDSIDE_PARAM, PARAMS_LEN };
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
	enum OutputId { SENDL_OUTPUT, SENDM_OUTPUT, SENDR_OUTPUT, OUTL_OUTPUT, OUTR_OUTPUT, OUTPUTS_LEN };
	enum LightId { LEDL_LIGHT, LEDR_LIGHT, LEDM_LIGHT, LIGHTS_LEN };

	struct TimeParamQuantity : rack::engine::ParamQuantity {
		std::string getDisplayValueString() override {
			Spatializer *m = dynamic_cast<Spatializer *>(module);
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
		configInput(INL_INPUT, "Audio Left");
		configInput(INR_INPUT, "Audio Right");
		configInput(RETURNL_INPUT, "Return Left");
		configInput(RETURNM_INPUT, "Return Mid");
		configInput(RETURNR_INPUT, "Return Right");
		configOutput(SENDL_OUTPUT, "Send Left");
		configOutput(SENDM_OUTPUT, "Send Mid");
		configOutput(SENDR_OUTPUT, "Send Right");
		configOutput(OUTL_OUTPUT, "Audio Left");
		configOutput(OUTR_OUTPUT, "Audio Right");
	}
	static const int maxDelaySamples = 2880;
	float delayBufferL[maxDelaySamples] = {};
	float delayBufferR[maxDelaySamples] = {};
	int delayIndex = 0;

	float slewedTime = 0.f;
	const float timeSlewRate = 0.0005f;

	// Cached values
	float cachedTime = -1.f;
	float cachedWidth = -1.f;
	float cachedMix = -1.f;
	float cachedRange = -1.f;

	int delaySamples = 1;
	float width = 0.f;
	float mix = 0.f;
	bool useMilliseconds = true;

	inline float crossfade(float a, float b, float x) {
		return a * (1.f - x) + b * x;
	}

	void process(const ProcessArgs &args) override {
		const float sampleRate = args.sampleRate;
	
		// --- Inputs ---
		const float inL = inputs[INL_INPUT].getVoltage();
		const bool stereoIn = inputs[INR_INPUT].isConnected();
		const float inR = stereoIn ? inputs[INR_INPUT].getVoltage() : inL;
	
		// --- Update params/CVs ---
		float targetTime = params[TIME_PARAM].getValue() + inputs[TIMECV_INPUT].getVoltage() * 0.1f;
		targetTime = std::clamp(targetTime, 0.f, 1.f);
	
		if (targetTime != cachedTime || params[RANGE_PARAM].getValue() != cachedRange) {
			cachedTime = targetTime;
			cachedRange = params[RANGE_PARAM].getValue();
			useMilliseconds = cachedRange < 0.5f;
	
			if (useMilliseconds)
				delaySamples = std::clamp(
					(int)(rescale(slewedTime, 0.f, 1.f, 1.f, 30.f) * sampleRate * 0.001f), 1, maxDelaySamples - 1);
			else
				delaySamples = std::clamp((int)(rescale(slewedTime, 0.f, 1.f, 1.f, 50.f)), 1, maxDelaySamples - 1);
		}
	
		float newWidth =
			std::clamp(params[WIDTH_PARAM].getValue() + inputs[WIDTHCV_INPUT].getVoltage() * 0.1f, 0.f, 1.f);
		if (newWidth != cachedWidth) {
			cachedWidth = newWidth;
			width = cachedWidth;
		}
	
		// --- M/S knob ---
		mix = std::clamp(params[MIDSIDE_PARAM].getValue() + inputs[MIDSIDECV_INPUT].getVoltage() * 0.1f, 0.f, 1.f);
	
		// --- Slew time ---
		slewedTime += (targetTime - slewedTime) * timeSlewRate;
	
		// --- Delay ---
		delayBufferL[delayIndex] = inL;
		delayBufferR[delayIndex] = stereoIn ? inR : inL;
	
		int readIndex = delayIndex - delaySamples;
		if (readIndex < 0)
			readIndex += maxDelaySamples;
		delayIndex = (delayIndex + 1) % maxDelaySamples;
	
		const float delayedL = delayBufferL[readIndex];
		const float delayedR = -delayBufferR[readIndex];
	
		// --- Spatial panning ---
		float wetL, wetR;
		if (stereoIn) {
			const float panWidth = (width <= 0.5f) ? (width * 2.f) : 1.f;
			const float center = 0.5f * (1.f - panWidth);
			wetL = delayedL * (center + panWidth) + delayedR * center;
			wetR = delayedR * (center + panWidth) + delayedL * center;
		} else {
			const float centerAmt = 1.f - width;
			const float sideAmt = width;
			const float blend = 0.5f * centerAmt + sideAmt;
			wetL = delayedL * blend;
			wetR = delayedR * blend;
		}
	
		// --- Sends: pure patch points ---
		outputs[SENDL_OUTPUT].setVoltage(wetL);
		outputs[SENDR_OUTPUT].setVoltage(wetR);
		outputs[SENDM_OUTPUT].setVoltage((inL + inR) * 0.5f);
	
// --- Returns: patch points override defaults ---
const float returnL = inputs[RETURNL_INPUT].isConnected() ? inputs[RETURNL_INPUT].getVoltage() : wetL;
const float returnR = inputs[RETURNR_INPUT].isConnected() ? inputs[RETURNR_INPUT].getVoltage() : wetR;
const float defaultMid = (inL + inR) * 0.5f;  // dry sum
const float returnM = inputs[RETURNM_INPUT].isConnected() ? inputs[RETURNM_INPUT].getVoltage() : defaultMid;

// --- Main outputs: M/S crossfader applied to returns ---
float outL, outR;
if (!stereoIn) {
    // MONO MODE: mid is sum of inputs or patched mid, sides are wet or patched returns
    const float midMono = returnM;     // sum of inputs overridden by RETURNM
    const float sideL = returnL;       // wet or RETURNL
    const float sideR = returnR;       // wet or RETURNR
    outL = crossfade(midMono, sideL, mix);
    outR = crossfade(midMono, sideR, mix);
} else {
    // STEREO MODE: mix mid/side of returns
    const float mid = returnM;  
    const float side = (returnL - returnR) * 0.5f;
    const float newMid = crossfade(mid, 0.f, mix);
    const float newSide = crossfade(0.f, side, mix);
    outL = newMid + newSide;
    outR = newMid - newSide;
}
	
		outputs[OUTL_OUTPUT].setVoltage(outL);
		outputs[OUTR_OUTPUT].setVoltage(outR);
	
		// --- LED metering ---
		const float ledScale = 0.2f;
		const float leftSignal = fabsf(outL) * ledScale;
		const float rightSignal = fabsf(outR) * ledScale;
		const float midSignalLevel = fabsf((outL + outR) * 0.5f) * ledScale;
	
		const float midLedBrightness = (1.f - mix) * 2.f;
		const float sideLedBrightness = mix * 2.f;
	
		lights[LEDM_LIGHT].setBrightnessSmooth(midSignalLevel * midLedBrightness, args.sampleTime);
		lights[LEDL_LIGHT].setBrightnessSmooth(leftSignal * sideLedBrightness, args.sampleTime);
		lights[LEDR_LIGHT].setBrightnessSmooth(rightSignal * sideLedBrightness, args.sampleTime);
	}	
};

struct SpatializerWidget : ModuleWidget {
	SpatializerWidget(Spatializer *module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Spatializer_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Switch2Pos>(mm2px(Vec(9.206, 19.076)), module, Spatializer::RANGE_PARAM));
		addParam(createParamCentered<Davies_large>(mm2px(Vec(30.514, 19.757)), module, Spatializer::MIDSIDE_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(13.507, 41.317)), module, Spatializer::TIME_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(47.522, 41.317)), module, Spatializer::WIDTH_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(13.525, 60.556)), module, Spatializer::TIMECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.556, 60.556)), module, Spatializer::MIDSIDECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.586, 60.556)), module, Spatializer::WIDTHCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.114, 97.187)), module, Spatializer::RETURNL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.556, 97.187)), module, Spatializer::RETURNM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(53.096, 97.187)), module, Spatializer::RETURNR_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.114, 111.2)), module, Spatializer::INL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.021, 111.2)), module, Spatializer::INR_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.114, 81.107)), module, Spatializer::SENDL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.556, 81.107)), module, Spatializer::SENDM_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(53.096, 81.107)), module, Spatializer::SENDR_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.228, 111.2)), module, Spatializer::OUTL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(53.096, 111.2)), module, Spatializer::OUTR_OUTPUT));

		addChild(
			createLightCentered<MediumLight<RedLight>>(mm2px(Vec(8.114, 69.975)), module, Spatializer::LEDL_LIGHT));
		addChild(
			createLightCentered<MediumLight<RedLight>>(mm2px(Vec(30.556, 69.975)), module, Spatializer::LEDM_LIGHT));
		addChild(
			createLightCentered<MediumLight<RedLight>>(mm2px(Vec(53.096, 69.975)), module, Spatializer::LEDR_LIGHT));
	}
};

Model *modelSpatializer = createModel<Spatializer, SpatializerWidget>("Spatializer");
