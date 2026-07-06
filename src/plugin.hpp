#pragma once
#include <rack.hpp>
//

using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin *pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelDJFilter;
extern Model* modelSpatializer;
extern Model* modelStereoWidth;
extern Model* modelStereoCrossfader;
extern Model* modelSeventiesEQ;
extern Model* modelSeventiesComp;
extern Model* modelBoost;
extern Model* modelDecay;
extern Model* modelAuxSends;
extern Model* modelBitcrusher;
extern Model* modelDrumBus;
extern Model* modelTapeDelay;

struct Knob9mm : RoundKnob {
	Knob9mm() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/9mm_knob.svg")));
	}
};

struct Davies1900hBlack : RoundKnob {
	Davies1900hBlack() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/Davies1900hBlack.svg")));
	}
};

struct Davies_large : RoundKnob {
	Davies_large() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/Davies_large.svg")));
	}
};

struct Switch2Pos : SvgSwitch {
	Switch2Pos() {
		momentary = false; // true for momentary behavior
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SubMiniToggle_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SubMiniToggle_2.svg")));
	}
};

struct Switch2PosHorizontal : SvgSwitch {
	Switch2PosHorizontal() {
		momentary = false; // true for momentary behavior
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SubMiniToggleH_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SubMiniToggleH_2.svg")));
	}
};

struct Switch3Pos : SvgSwitch {
	Switch3Pos() {
		momentary = false; // true for momentary behavior
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SubMiniToggle_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SubMiniToggle_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SubMiniToggle_2.svg")));
	}
};

struct Switch3PosHorizontal : SvgSwitch {
	Switch3PosHorizontal() {
		momentary = false; // true for momentary behavior
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SubMiniToggleH_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SubMiniToggleH_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/SubMiniToggleH_2.svg")));
	}
};
