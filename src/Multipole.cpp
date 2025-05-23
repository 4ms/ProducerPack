#include "plugin.hpp"


struct Multipole : Module {
	enum ParamId {
		CENTERFREQUENCY_PARAM,
		RESONANCE_PARAM,
		BANDWIDTH_PARAM,
		NUMPOLES_PARAM,
		NOTCHBANDPASS_PARAM,
		POLEGAP_PARAM,
		STEREOWIDTH_PARAM,
		ALLODDEVEN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		INLEFT_INPUT,
		CENTERFREQUENCYCV_INPUT,
		RESONANCECV_INPUT,
		NUMPOLESCV_INPUT,
		BANDWIDTHCV_INPUT,
		INRIGHT_INPUT,
		POLEGAPCV_INPUT,
		AOECV_INPUT,
		WIDTHCV_INPUT,
		NOTCHBANDPASSCV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTLEFT_OUTPUT,
		OUTRIGHT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Multipole() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(CENTERFREQUENCY_PARAM, 0.f, 1.f, 0.f, "Center Frequency", "hz", 0.0f, 100.f); // Change to real value later
		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.0f, 100.0f);
		configParam(BANDWIDTH_PARAM, 0.f, 1.f, 0.f, "Bandwidth", "%", 0.0f, 100.0f);

		configParam(NUMPOLES_PARAM, 1.f, 8.f, 1.f, "# of Poles");
		paramQuantities[NUMPOLES_PARAM]->snapEnabled = true; 

		configParam(NOTCHBANDPASS_PARAM, 0.f, 1.f, 0.f, "Notch/Bandpass", "%", 0.0f, 100.0f);
		configParam(POLEGAP_PARAM, 0.f, 1.f, 0.f, "Pole Gap", "%", 0.0f, 100.0f);
		configParam(STEREOWIDTH_PARAM, 0.f, 1.f, 0.f, "Stereo Width", "%", 0.0f, 100.f);
		configParam(ALLODDEVEN_PARAM, 0.f, 1.f, 0.f, "All/Odd/Even", "%", 0.0f, 100.f);

		configInput(CENTERFREQUENCYCV_INPUT, "Center Frequency CV");
		configInput(RESONANCECV_INPUT, "Resonance CV");
		configInput(NUMPOLESCV_INPUT, "Num Poles CV");
		configInput(BANDWIDTHCV_INPUT, "Bandwidth CV");
		configInput(POLEGAPCV_INPUT, "Pole Gap CV");
		configInput(AOECV_INPUT, "All/Odd/Even CV");
		configInput(WIDTHCV_INPUT, "Stereo Width CV");
		configInput(NOTCHBANDPASSCV_INPUT, "Notch/Bandpass CV");

		configInput(INLEFT_INPUT, "Audio L");
		configInput(INRIGHT_INPUT, "Audio R");
		configOutput(OUTLEFT_OUTPUT, "Audio L");
		configOutput(OUTRIGHT_OUTPUT, "Audio R");
	}

	// Interface
	float centerFrequencyControl = 0;
	float resonanceControl = 0;
	float bandwidthControl = 0;
	float numPolesControl = 0; 
	float notchBandpassControl = 0;
	float poleGapControl = 0;
	float stereoWidthControl = 0;
	float allOddEvenControl = 0;

    // Filter state variables for each filter instance (8 total)
    float lastInput1[8] = {0.0f};  // Last inputs for each filter
    float lastInput2[8] = {0.0f};  // Last inputs for each filter
    float lastOutput1[8] = {0.0f};  // Last outputs for each filter
    float lastOutput2[8] = {0.0f};  // Last outputs for each filter

   // Function to process higher-order (multistage) filters
   void processMultistageFilter(int filterIndex, float frequency, float bandwidth, float resonance, float inputSignal, bool isNotch, float* outputSignal) {
	// Calculate Q factor (resonance / bandwidth)
	float Q = resonance / bandwidth;

	// Omega for the filter
	float omega = 2.0 * M_PI * frequency / APP->engine->getSampleRate();
	float alpha = sin(omega) / (2.0 * Q);

	// Initialize the intermediate signal
	float intermediateSignal = inputSignal;

	// Loop over each biquad stage (for higher-order filter)
	for (int i = 0; i < numPolesControl; ++i) {
		// Calculate the coefficients for each biquad stage
		float a0 = 1.0 + alpha;
		float a1 = -2.0 * cos(omega);
		float a2 = 1.0 - alpha;
		float b0 = alpha;
		float b1 = 0.0;
		float b2 = -alpha;

		// For notch filter, invert the coefficients
		if (isNotch) {
			b0 = 1.0 - alpha;
			b1 = -2.0 * cos(omega);
			b2 = 1.0 + alpha;
		}

		// Process signal (biquad filter formula)
		float outputSignalTemp = (b0 / a0) * intermediateSignal + (b1 / a0) * lastInput1[filterIndex] + (b2 / a0) * lastInput2[filterIndex] -
								 (a1 / a0) * lastOutput1[filterIndex] - (a2 / a0) * lastOutput2[filterIndex];

		// Update state for the next cycle
		lastInput2[filterIndex] = lastInput1[filterIndex];
		lastInput1[filterIndex] = intermediateSignal;
		lastOutput2[filterIndex] = lastOutput1[filterIndex];
		lastOutput1[filterIndex] = outputSignalTemp;

		// Pass the output of this stage to the next
		intermediateSignal = outputSignalTemp;
	}

	// Return the final output after all stages have been processed
	*outputSignal = intermediateSignal;
}

// Crossfader function for smoothly transitioning between bandpass and notch filters
float crossfade(float bandpassSignal, float notchSignal, float crossfadeAmount) {
	// Crossfade between bandpass and notch filter signals
	return bandpassSignal * (1.0f - crossfadeAmount) + notchSignal * crossfadeAmount;
}

void process(const ProcessArgs& args) override {
	const int paramIds[] = {CENTERFREQUENCY_PARAM, RESONANCE_PARAM, BANDWIDTH_PARAM, NUMPOLES_PARAM, NOTCHBANDPASS_PARAM, POLEGAP_PARAM, STEREOWIDTH_PARAM, ALLODDEVEN_PARAM};
	const int cvInputs[] = {CENTERFREQUENCYCV_INPUT, RESONANCECV_INPUT, BANDWIDTHCV_INPUT, NUMPOLESCV_INPUT, NOTCHBANDPASSCV_INPUT, POLEGAPCV_INPUT, WIDTHCV_INPUT, AOECV_INPUT};
	float* controlValues[] = {&centerFrequencyControl, &resonanceControl, &bandwidthControl, &numPolesControl, &notchBandpassControl, &poleGapControl, &stereoWidthControl, &allOddEvenControl};

	// Get the control values from parameters and CV inputs
	for (int i = 0; i < 8; ++i) {
		float cvScale = (inputs[cvInputs[i]].getVoltage() + 5.0f) / 10.0f;
		float cvSum = params[paramIds[i]].getValue() + (cvScale - 0.5f);
		*(controlValues[i]) = clamp(cvSum, 0.0f, 1.0f);
	}

	centerFrequencyControl = (centerFrequencyControl * 2000.0f) + 100.0f; 
	
	float frequencies[8] = {centerFrequencyControl, (centerFrequencyControl * 2.0f), (centerFrequencyControl * 3.0f), (centerFrequencyControl * 4.0f), 
							(centerFrequencyControl * 5.0f), (centerFrequencyControl * 6.0f), (centerFrequencyControl * 7.0f), (centerFrequencyControl * 8.0f)};

	float bandwidths[8] = {bandwidthControl, bandwidthControl, bandwidthControl, bandwidthControl, bandwidthControl, bandwidthControl, bandwidthControl, bandwidthControl};
	float resonances[8] = {resonanceControl, resonanceControl, resonanceControl, resonanceControl, resonanceControl, resonanceControl, resonanceControl, resonanceControl};

	// Interpolate the crossfade value
	float crossfadeAmount = notchBandpassControl; // Smooth transition between 0 (bandpass) and 1 (notch)

	// Process the signal through all the filters and apply crossfading
	float inputSignal = inputs[INLEFT_INPUT].getVoltage();
	float outputSignal = 0.0f;
	for (int i = 0; i < 8; ++i) {
		float bandpassSignal = 0.0f;
		float notchSignal = 0.0f;

		// Process the bandpass filter with multiple poles
		processMultistageFilter(i, frequencies[i], bandwidths[i], resonances[i], inputSignal, false, &bandpassSignal);

		// Process the notch filter with multiple poles
		processMultistageFilter(i, frequencies[i], bandwidths[i], resonances[i], inputSignal, true, &notchSignal);

		// Apply the crossfade to combine bandpass and notch signals
		float finalSignal = crossfade(bandpassSignal, notchSignal, crossfadeAmount);

		// Accumulate the processed signal
		outputSignal += finalSignal;
	}

	// Set the output voltage
	outputSignal = clamp((outputSignal * 0.3f), -5.0f, 5.0f);
	outputs[OUTLEFT_OUTPUT].setVoltage(outputSignal);
}
};

struct MultipoleWidget : ModuleWidget {
	MultipoleWidget(Multipole* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Multipole.svg")));

		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(14.157, 21.516)), module, Multipole::CENTERFREQUENCY_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(78.52, 21.526)), module, Multipole::RESONANCE_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(49.089, 21.85)), module, Multipole::BANDWIDTH_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(13.082, 47.657)), module, Multipole::NUMPOLES_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(81.404, 48.86)), module, Multipole::NOTCHBANDPASS_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(48.578, 49.372)), module, Multipole::POLEGAP_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(69.159, 76.55)), module, Multipole::STEREOWIDTH_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(23.535, 78.515)), module, Multipole::ALLODDEVEN_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.314, 109.09)), module, Multipole::INLEFT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(18.585, 109.796)), module, Multipole::CENTERFREQUENCYCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(52.339, 110.081)), module, Multipole::RESONANCECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(71.505, 109.986)), module, Multipole::NUMPOLESCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34.185, 110.558)), module, Multipole::BANDWIDTHCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.43, 122.926)), module, Multipole::INRIGHT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.326, 123.007)), module, Multipole::POLEGAPCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(53.08, 123.293)), module, Multipole::AOECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(72.246, 123.198)), module, Multipole::WIDTHCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34.926, 123.769)), module, Multipole::NOTCHBANDPASSCV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(90.947, 109.602)), module, Multipole::OUTLEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(91.688, 122.813)), module, Multipole::OUTRIGHT_OUTPUT));
	}
};


Model* modelMultipole = createModel<Multipole, MultipoleWidget>("Multipole");