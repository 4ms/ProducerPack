#include "plugin.hpp"


struct PrePostBypass : Module {
	enum ParamId {
		SWITCH_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		FX1INL_INPUT,
		FX1INR_INPUT,
		FX2INL_INPUT,
		FX2INR_INPUT,
		PGMINL_INPUT,
		PGMINR_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		FX1OUTL_OUTPUT,
		FX1OUTR_OUTPUT,
		FX2OUTL_OUTPUT,
		FX2OUTR_OUTPUT,
		PGMOUTL_OUTPUT,
		PGMOUTR_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	PrePostBypass() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(SWITCH_PARAM, 0.f, 2.f, 0.f, "Routing", {"Pre", "Bypass", "Post"});
		configInput(FX1INL_INPUT, "FX1 L");
		configInput(FX1INR_INPUT, "FX1 R");
		configInput(FX2INL_INPUT, "FX2 L");
		configInput(FX2INR_INPUT, "FX2 R");
		configInput(PGMINL_INPUT, "PGM L");
		configInput(PGMINR_INPUT, "PGM R");
		configOutput(FX1OUTL_OUTPUT, "FX1 L");
		configOutput(FX1OUTR_OUTPUT, "FX1 R");
		configOutput(FX2OUTL_OUTPUT, "FX2 L");
		configOutput(FX2OUTR_OUTPUT, "FX2 R");
		configOutput(PGMOUTL_OUTPUT, "PGM L");
		configOutput(PGMOUTR_OUTPUT, "PGM R");
	}

	void process(const ProcessArgs& args) override {
		int mode = (int)params[SWITCH_PARAM].getValue();
	
		// Read all inputs
		float fx1InL = inputs[FX1INL_INPUT].getVoltage();
		float fx1InR = inputs[FX1INR_INPUT].getVoltage();
		float fx2InL = inputs[FX2INL_INPUT].getVoltage();
		float fx2InR = inputs[FX2INR_INPUT].getVoltage();
		float pgmL = inputs[PGMINL_INPUT].getVoltage();
		float pgmR = inputs[PGMINR_INPUT].getVoltage();
	
		switch (mode) {
			case 0: // Pre: FX1 -> FX2
				outputs[FX1OUTL_OUTPUT].setVoltage(pgmL);
				outputs[FX1OUTR_OUTPUT].setVoltage(pgmR);
				outputs[FX2OUTL_OUTPUT].setVoltage(fx1InL);
				outputs[FX2OUTR_OUTPUT].setVoltage(fx1InR);
				outputs[PGMOUTL_OUTPUT].setVoltage(fx2InL);
				outputs[PGMOUTR_OUTPUT].setVoltage(fx2InR);
				break;
	
			case 1: // Bypass: FX2 only
				outputs[FX1OUTL_OUTPUT].setVoltage(0.f);
				outputs[FX1OUTR_OUTPUT].setVoltage(0.f);
				outputs[FX2OUTL_OUTPUT].setVoltage(pgmL);
				outputs[FX2OUTR_OUTPUT].setVoltage(pgmR);
				outputs[PGMOUTL_OUTPUT].setVoltage(fx2InL);
				outputs[PGMOUTR_OUTPUT].setVoltage(fx2InR);
				break;
	
			case 2: // Post: FX2 -> FX1
				outputs[FX1OUTL_OUTPUT].setVoltage(fx2InL);
				outputs[FX1OUTR_OUTPUT].setVoltage(fx2InR);
				outputs[FX2OUTL_OUTPUT].setVoltage(pgmL);
				outputs[FX2OUTR_OUTPUT].setVoltage(pgmR);
				outputs[PGMOUTL_OUTPUT].setVoltage(fx1InL);
				outputs[PGMOUTR_OUTPUT].setVoltage(fx1InR);
				break;
		}
	}	
};


struct PrePostBypassWidget : ModuleWidget {
	PrePostBypassWidget(PrePostBypass* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/PrePostBypass_info.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<_3PosHorizontal>(mm2px(Vec(10.557, 119.79)), module, PrePostBypass::SWITCH_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.662, 12.307)), module, PrePostBypass::FX1INL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.393, 12.392)), module, PrePostBypass::FX1INR_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.059, 51.036)), module, PrePostBypass::FX2INL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.79, 51.121)), module, PrePostBypass::FX2INR_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.059, 85.929)), module, PrePostBypass::PGMINL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.79, 86.014)), module, PrePostBypass::PGMINR_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.662, 27.703)), module, PrePostBypass::FX1OUTL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(14.393, 27.788)), module, PrePostBypass::FX1OUTR_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(6.059, 62.86)), module, PrePostBypass::FX2OUTL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(14.79, 62.945)), module, PrePostBypass::FX2OUTR_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.927, 98.943)), module, PrePostBypass::PGMOUTL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(14.658, 99.029)), module, PrePostBypass::PGMOUTR_OUTPUT));
	}
};


Model* modelPrePostBypass = createModel<PrePostBypass, PrePostBypassWidget>("PrePostBypass");