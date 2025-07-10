#include "plugin.hpp"

struct InvertedRangeParamQuantity : rack::engine::ParamQuantity {
	float displayMin, displayMax;

	InvertedRangeParamQuantity(float min, float max, std::string paramName) : displayMin(min), displayMax(max) {
		name = paramName;
	}

	float getDisplayValue() override {
		return displayMax - getValue() * (displayMax - displayMin);
	}

	std::string getDisplayValueString() override {
		return rack::string::f("%.1f Hz", getDisplayValue());
	}
};


struct Bitcrusher : Module {
	enum ParamId {
		SAMPLERATE_PARAM,
		BITDEPTH_PARAM,
		DRY_WET_PARAM,
		CUTOFF_PARAM,
		RESONANCE_PARAM,
		FILTERTYPE_PARAM,
		VOLUME_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SAMPLERATECVIN_INPUT,
		BITDEPTHCVIN_INPUT,
		DRY_WETCVIN_INPUT,
		CUTOFFCVIN_INPUT,
		RESONANCECVIN_INPUT,
		AUDIOLEFTIN_INPUT,
		AUDIORIGHTIN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIOLEFTOUT_OUTPUT,
		AUDIORIGHTOUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	
	Bitcrusher() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(SAMPLERATE_PARAM, 0.f, 1.f, 0.f, "Clock Frequency"); 
		configSwitch(BITDEPTH_PARAM, 0.f, 15.f, 0.f, "Bit Depth", {"16 bits", "15 bits", "14 bits", "13 bits", "12 bits", "11 bits", "10 bits", "9 bits", "8 bits", "7 bits", "6 bits", "5 bits", "4 bits", "3 bits", "2 bits", "1 bit"});
		configParam(DRY_WET_PARAM, 0.f, 1.f, 1.f, "Dry/Wet", "%", 0.f, 100.f);
		configParam(CUTOFF_PARAM, 0.f, 1.f, 1.f, "Cutoff", "hz", 80.f, 100.f);
		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);
		configSwitch(FILTERTYPE_PARAM, 0.f, 1.f, 0.f, "Filter Type", {"Lowpass", "Highpass"});
		configParam(VOLUME_PARAM, 0.f, 1.f, 1.f, "Volume", "%", 0.f, 100.f);
		configInput(SAMPLERATECVIN_INPUT, "Sample Rate CV");
		configInput(BITDEPTHCVIN_INPUT, "Bit Depth CV");
		configInput(DRY_WETCVIN_INPUT, "Dry/Wet CV");
		configInput(CUTOFFCVIN_INPUT, "Cutoff CV");
		configInput(RESONANCECVIN_INPUT, "Resonance CV");
		configInput(AUDIOLEFTIN_INPUT, "Audio Left");
		configInput(AUDIORIGHTIN_INPUT, "Audio Right");
		configOutput(AUDIOLEFTOUT_OUTPUT, "Audio Left");
		configOutput(AUDIORIGHTOUT_OUTPUT, "Audio Right");

		paramQuantities[SAMPLERATE_PARAM] = new InvertedRangeParamQuantity(20.f, 8000.f, "Clock Frequency");
		paramQuantities[SAMPLERATE_PARAM]->module = this;
		paramQuantities[SAMPLERATE_PARAM]->paramId = SAMPLERATE_PARAM;
	}
	void process(const ProcessArgs& args) override {
	}
};


struct BitcrusherWidget : ModuleWidget {
	BitcrusherWidget(Bitcrusher* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Bitcrusher.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(8.924, 19.095)), module, Bitcrusher::SAMPLERATE_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(31.302, 19.095)), module, Bitcrusher::BITDEPTH_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(20.32, 44.599)), module, Bitcrusher::DRY_WET_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(8.924, 67.068)), module, Bitcrusher::CUTOFF_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(31.302, 67.068)), module, Bitcrusher::RESONANCE_PARAM));
		addParam(createParamCentered<_2PosHorizontal>(mm2px(Vec(20.32, 85.948)), module, Bitcrusher::FILTERTYPE_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(20.32, 95.631)), module, Bitcrusher::VOLUME_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.924, 29.117)), module, Bitcrusher::SAMPLERATECVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31.302, 29.117)), module, Bitcrusher::BITDEPTHCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.32, 53.904)), module, Bitcrusher::DRY_WETCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.924, 77.047)), module, Bitcrusher::CUTOFFCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31.302, 77.047)), module, Bitcrusher::RESONANCECVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.585, 117.107)), module, Bitcrusher::AUDIOLEFTIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.493, 117.107)), module, Bitcrusher::AUDIORIGHTIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(27.136, 117.107)), module, Bitcrusher::AUDIOLEFTOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(36.083, 117.107)), module, Bitcrusher::AUDIORIGHTOUT_OUTPUT));
	}
};


Model* modelBitcrusher = createModel<Bitcrusher, BitcrusherWidget>("Bitcrusher");