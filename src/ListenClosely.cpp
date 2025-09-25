#include "plugin.hpp"


struct ListenClosely : Module {
	enum ParamId {
		RATIO_PARAM,
		PEAKREDUCTION_PARAM,
		DRYWET_PARAM,
		GAIN_PARAM,
		LOWSHELF_PARAM,
		HIGHSHELF_PARAM,
		MID_PARAM,
		LOWFREQSELECT_PARAM,
		HIGHPASSFREQSELECT_PARAM,
		MIDFREQSELECT_PARAM,
		WIDTH_PARAM,
		OUTPUTVOL_PARAM,
		PREPOST_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		INL_INPUT,
		INR_INPUT,
		WIDTHCV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTL_OUTPUT,
		OUTR_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		GRAPH1_LIGHT,
		GRAPH2_LIGHT,
		GRAPH3_LIGHT,
		GRAPH4_LIGHT,
		GRAPH5_LIGHT,
		GRAPH6_LIGHT,
		GRAPH7_LIGHT,
		GRAPH8_LIGHT,
		GRAPH9_LIGHT,
		CLIPLED_LIGHT,
		_110LED_LIGHT,
		_80LED_LIGHT,
		_160LED_LIGHT,
		_220LED_LIGHT,
		_50LED_LIGHT,
		_300LED_LIGHT,
		_60LED_LIGHT,
		OFFLED_LIGHT,
		_35LED_LIGHT,
		_32KLED_LIGHT,
		_16KLED_LIGHT,
		_700LED_LIGHT,
		_48KLED_LIGHT,
		_360LED_LIGHT,
		_72KLED_LIGHT,
		LIGHTS_LEN
	};

	ListenClosely() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(RATIO_PARAM, 0.f, 2.f, 0.f, "Ratio", {"Compressor", "Bypass", "Limiter"});
		configSwitch(PREPOST_PARAM, 0.f, 2.f, 0.f, "EQ", {"Pre", "Bypass", "Post"});

		configSwitch(LOWFREQSELECT_PARAM, 0.f, 1.f, 0.f, "Low Frequency Select");
		configSwitch(MIDFREQSELECT_PARAM, 0.f, 1.f, 0.f, "Mid Frequency Select");
		configSwitch(HIGHPASSFREQSELECT_PARAM, 0.f, 1.f, 0.f, "Highpass Frequency Select");

        configParam(PEAKREDUCTION_PARAM, 0.f, 1.f, 0.5f, "Amount", "%", 0.f, 100.f);
		configParam(DRYWET_PARAM, 0.f, 1.f, 1.f, "Dry/Wet", "%", 0.f, 100.f);
        configParam(GAIN_PARAM, 0.f, 1.f, 0.25f, "Gain", "db", 0.f, 40.f);

        configParam(LOWSHELF_PARAM, -15.f, 15.f, 0.f, "Low Shelf Gain", "dB");
		configParam(MID_PARAM, -15.f, 15.f, 0.f, "Mid Gain", "dB");
        configParam(HIGHSHELF_PARAM, -15.f, 15.f, 0.f, "High Shelf Gain", "dB");

		configParam(WIDTH_PARAM, 0.f, 1.f, 0.5f, "Width", "%", 0.f, 200.f);

        configParam(OUTPUTVOL_PARAM, 0.f, 1.f, 0.25f, "Output Level", "x");

		configInput(INL_INPUT, "Audio Left");
		configInput(INR_INPUT, "Audio Right");
		configInput(WIDTHCV_INPUT, "Width CV");
		configOutput(OUTL_OUTPUT, "Audio Left");
		configOutput(OUTR_OUTPUT, "Audio Right");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct ListenCloselyWidget : ModuleWidget {
	ListenCloselyWidget(ListenClosely* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/ListenClosely_info.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(12.256, 26.45)), module, ListenClosely::PEAKREDUCTION_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(58.538, 26.45)), module, ListenClosely::DRYWET_PARAM));
		addParam(createParamCentered<_9mmKnob>(mm2px(Vec(35.487, 37.026)), module, ListenClosely::GAIN_PARAM));
		
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(12.164, 47.733)), module, ListenClosely::LOWSHELF_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(58.693, 47.733)), module, ListenClosely::HIGHSHELF_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(35.582, 64.355)), module, ListenClosely::MID_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(12.299, 93.217)), module, ListenClosely::WIDTH_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(58.414, 93.263)), module, ListenClosely::OUTPUTVOL_PARAM));

		addParam(createParamCentered<LEDBezel>(mm2px(Vec(12.221, 69.566)), module, ListenClosely::LOWFREQSELECT_PARAM));
		addParam(createParamCentered<LEDBezel>(mm2px(Vec(58.376, 69.573)), module, ListenClosely::HIGHPASSFREQSELECT_PARAM));
		addParam(createParamCentered<LEDBezel>(mm2px(Vec(35.572, 86.054)), module, ListenClosely::MIDFREQSELECT_PARAM));

		addParam(createParamCentered<_3PosHorizontal>(mm2px(Vec(35.402, 100.566)), module, ListenClosely::PREPOST_PARAM));
		addParam(createParamCentered<_3PosHorizontal>(mm2px(Vec(35.381, 22.267)), module, ListenClosely::RATIO_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.963, 113.811)), module, ListenClosely::INL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.969, 113.811)), module, ListenClosely::INR_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35.529, 113.811)), module, ListenClosely::WIDTHCV_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(51.107, 113.811)), module, ListenClosely::OUTL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(62.117, 113.811)), module, ListenClosely::OUTR_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(15.56, 14.72)), module, ListenClosely::GRAPH1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(20.562, 14.72)), module, ListenClosely::GRAPH2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(25.568, 14.72)), module, ListenClosely::GRAPH3_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(30.57, 14.72)), module, ListenClosely::GRAPH4_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(35.572, 14.72)), module, ListenClosely::GRAPH5_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(40.577, 14.72)), module, ListenClosely::GRAPH6_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(45.579, 14.72)), module, ListenClosely::GRAPH7_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(50.581, 14.72)), module, ListenClosely::GRAPH8_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(55.587, 14.72)), module, ListenClosely::GRAPH9_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(32.518, 49.664)), module, ListenClosely::CLIPLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(7.85, 61.391)), module, ListenClosely::_110LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(53.802, 61.548)), module, ListenClosely::_80LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(63.081, 61.51)), module, ListenClosely::_160LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(20.182, 65.837)), module, ListenClosely::_220LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(50.807, 69.049)), module, ListenClosely::_50LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(65.551, 69.013)), module, ListenClosely::_300LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(4.365, 72.754)), module, ListenClosely::_60LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(52.871, 76.901)), module, ListenClosely::OFFLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.457, 77.355)), module, ListenClosely::_35LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(41.518, 77.913)), module, ListenClosely::_32KLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(30.163, 78.104)), module, ListenClosely::_16KLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(28.171, 85.468)), module, ListenClosely::_700LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(42.723, 85.395)), module, ListenClosely::_48KLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(30.67, 93.354)), module, ListenClosely::_360LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(40.754, 93.671)), module, ListenClosely::_72KLED_LIGHT));
	}
};


Model* modelListenClosely = createModel<ListenClosely, ListenCloselyWidget>("ListenClosely");