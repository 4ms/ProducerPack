#include "plugin.hpp"
#include "helpers/fast_math.hpp"


// Free-running LFO frequency range (used when no external clock is patched)
static constexpr float TIME_FREQ_MIN = 0.1f;
static constexpr float TIME_FREQ_MAX = 60.f;

static float timeKnobToFreq(float knobVal) {
	float octaves = std::log2(TIME_FREQ_MAX / TIME_FREQ_MIN);
	return TIME_FREQ_MIN * std::pow(2.f, knobVal * octaves);
}
static float timeFreqToKnob(float hz) {
	float octaves = std::log2(TIME_FREQ_MAX / TIME_FREQ_MIN);
	return std::log2(hz / TIME_FREQ_MIN) / octaves;
}

// Ext-clock div/mult scheme (same /32..x16 range and tooltip labels as the 4ms MPEG
// Div/Mult knob). Note MPEG's own signal is unipolar (an envelope); here it's only
// borrowed for the divider ratios/labels — our LFO stays bipolar throughout.
struct ClkFactor { bool mult; int n; };
static const ClkFactor clkFactors[] = {
	{false, 32}, {false, 16}, {false, 8}, {false, 7}, {false, 6}, {false, 5}, {false, 4}, {false, 3}, {false, 2},
	{true,  1},
	{true,  2}, {true,  3}, {true,  4}, {true,  5}, {true,  6}, {true,  7}, {true,  8}, {true, 12}, {true, 16},
};
static const char* clkFactorLabels[] = {
	"/32", "/16", "/8", "/7", "/6", "/5", "/4", "/3", "/2", "=", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x12", "x16"
};
static const int numClkFactors = 19;

static int divMultFactorIdx(float knobVal) {
	return clamp((int)std::round(knobVal * (numClkFactors - 1)), 0, numClkFactors - 1);
}


// Forward-declared so button/mode-aware quantities can read back module state
struct AutoPan;

struct TimeQuantity : ParamQuantity {
	// Free-running: param 0..1 → 0.1..60 Hz. Ext clock patched: knob becomes the
	// same /8..x8 div/mult selector as MPEG's Div/Mult knob (checked via module).
	float getDisplayValue() override { return timeKnobToFreq(getValue()); }
	void setDisplayValue(float hz) override { setValue(timeFreqToKnob(hz)); }
	std::string getDisplayValueString() override;
	std::string getUnit() override;
};

struct ShapeQuantity : ParamQuantity {
	std::string getDisplayValueString() override;
	std::string getUnit() override { return ""; }
};

struct TremoloQuantity : ParamQuantity {
	std::string getDisplayValueString() override;
	std::string getUnit() override { return ""; }
};


struct AutoPan : Module {
	enum ParamId {
		AMOUNT_PARAM,
		TIME_PARAM,
		SHAPE_PARAM,
		CURVE_PARAM,
		TREMOLO_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		AMOUNTCV_INPUT,
		EXTCLOCK_INPUT,
		INL_INPUT,
		TIMECV_INPUT,
		INR_INPUT,
		SHAPECV_INPUT,
		TREMOLOCV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTL_OUTPUT,
		OUTR_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		// Each entry is a RedGreenBlueLight: +0=R, +1=G, +2=B
		TIMELEDLEFT_LIGHT, TIMELEDLEFT_LIGHT_1, TIMELEDLEFT_LIGHT_2,
		TIMELEDRIGHT_LIGHT, TIMELEDRIGHT_LIGHT_1, TIMELEDRIGHT_LIGHT_2,
		SHAPELED_SINE_LIGHT,
		SHAPELED_TRIANGLE_LIGHT,
		SHAPELED_SAW_LIGHT,
		SHAPELED_SQUARE_LIGHT,
		SHAPELED_RANDOM_LIGHT,
		LIGHTS_LEN
	};

	// Tremolo button is a momentary rising-edge trigger; toggling state is tracked
	// virtually here. TREMOLOCV_INPUT (jack, Schmitt-conditioned, fires above
	// 0.5V) toggles the same state — either source can flip it.
	bool tremoloState = false;
	bool tremoloBtnPrev = false;
	dsp::SchmittTrigger tremoloTrigInput;

	// Shape button is a momentary rising-edge trigger, same as Tremolo — each
	// press advances to the next waveform, wrapping Random back to Sine.
	int  shapeIndex = 0;
	bool shapeBtnPrev = false;

	// LFO state
	float phase = 0.f;
	float randomPhase = 0.f;  // runs at 2x the LFO frequency, so Random steps twice per cycle
	float randomValue = 0.f;

	// De-click slew state for the waveforms with hard discontinuities (Saw's
	// wrap, Square's edges, Random's steps) — Sine and Triangle are already
	// continuous and don't need it.
	float sawSlewed = 0.f;
	float squareSlewed = 0.f;
	float randomSlewed = 0.f;

	// De-click slew for the final per-channel gain — smooths the abrupt jump
	// when Tremolo toggles (mono envelope <-> stereo pan is a different gain
	// function per channel, so switching mid-signal can pop otherwise).
	float gainLSlewed = 1.f;
	float gainRSlewed = 1.f;

	// Ext clock div/mult mode: measures period between ext clock edges, then
	// applies the Time knob's selected /8..x8 factor to derive the LFO frequency.
	// Cached each block for the tooltip (TimeQuantity) to read without touching inputs.
	bool  extClockMode = false;
	int   timeDivMultIdx = 7;
	dsp::SchmittTrigger extClockTrig;
	float extClockTimer = 0.f;
	float extClockPeriod = 0.f;
	bool  extClockPeriodValid = false;
	// Counts incoming clock edges toward a division's next hard-sync point.
	int   extClockPulseCount = 0;

	AutoPan() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(AMOUNT_PARAM, -1.f, 1.f, 0.f, "Amount", "%", 0.f, 100.f);
#ifdef METAMODULE
		// MetaModule's UI reads the raw name/unit/display* fields directly and does not
		// invoke a ParamQuantity's virtual overrides (same fix as DJFilter/TapeDelay), so
		// give these native settings instead of relying on getDisplayValueString().
		// Time: same exponential mapping as timeKnobToFreq(), MIN * (MAX/MIN)^knob Hz.
		// (With Ext. Clock patched it picks a div/mult instead; the native display
		// can't follow that, so it still shows the free-running Hz value.)
		configParam(TIME_PARAM, 0.f, 1.f, 0.5f, "Time", "Hz", TIME_FREQ_MAX / TIME_FREQ_MIN, TIME_FREQ_MIN);
		configButton(SHAPE_PARAM, "Shape");
#else
		configParam<TimeQuantity>(TIME_PARAM, 0.f, 1.f, 0.5f, "Time");
		configParam<ShapeQuantity>(SHAPE_PARAM, 0.f, 1.f, 0.f, "Shape");
#endif
		configParam(CURVE_PARAM, 0.f, 1.f, 0.f, "Curve", "%", 0.f, 100.f);
#ifdef METAMODULE
		configButton(TREMOLO_PARAM, "Tremolo");
#else
		configParam<TremoloQuantity>(TREMOLO_PARAM, 0.f, 1.f, 0.f, "Tremolo");
#endif
		configInput(AMOUNTCV_INPUT, "Amount CV");
		configInput(EXTCLOCK_INPUT, "Ext. Clock");
		configInput(INL_INPUT, "In L");
		configInput(TIMECV_INPUT, "Time CV");
		configInput(INR_INPUT, "In R");
		configInput(SHAPECV_INPUT, "Shape CV");
		configInput(TREMOLOCV_INPUT, "Tremolo CV");
		configOutput(OUTL_OUTPUT, "Out L");
		configOutput(OUTR_OUTPUT, "Out R");

		configBypass(INL_INPUT, OUTL_OUTPUT);
		configBypass(INR_INPUT, OUTR_OUTPUT);

		lightDivider.setDivision(LIGHT_DIVISION);
	}

	// Per-sample-rate constants (std::exp is expensive on MetaModule, so these are
	// only recomputed when the sample rate changes instead of every sample).
	float lastSampleTime = -1.f;
	float waveSlewCoeff = 0.f;
	float gainSlewCoeff = 0.f;

	// Exponential VCA response constant for the Curve trimpot (see gainFromLfo).
	static constexpr float VCA_EXPO_K = 7.f;
	const float invExpoRange = 1.f / (std::exp(VCA_EXPO_K) - 1.f);

	// Cached results of expensive math, recomputed only when their input changes
	// (std::pow / std::log2 are slow on MetaModule's Cortex-A7).
	float lastTimeKnob = -1.f;
	float cachedBaseFreq = 0.f;
	float lastTimeCv = 0.f;
	float cachedTimeCvMult = 1.f; // 2^timeCv; exactly 1 while timeCv is 0
	float lastDepthMag = -1.f;
	float cachedDepth = 0.f;

	// Panel LEDs don't need audio-rate updates: refresh them every LIGHT_DIVISION
	// samples (still ~3kHz at 48kHz), passing the elapsed time so the smoothing
	// timing is unchanged.
	static constexpr int LIGHT_DIVISION = 16;
	dsp::ClockDivider lightDivider;

	// Stepped selector: Sine, Triangle, Saw, Square, Random — no crossfade
	// between shapes, shapeVal picks one waveform outright.
	float computeWave(float p, float shapeVal) {
		p -= std::floor(p);
		int idx = clamp((int)std::round(shapeVal), 0, 4);

		// Fixed 1ms slew time constant (waveSlewCoeff, cached per sample rate) — just
		// enough to round off the hard edges (de-click) without audibly softening the
		// waveform shapes.
		const float slewCoeff = waveSlewCoeff;

		switch (idx) {
			case 0:  return 5.f * std::sin(2.f * float(M_PI) * p);
			case 1:  return 5.f * (1.f - 4.f * std::abs(p - 0.5f));
			case 2:  sawSlewed += (5.f * (1.f - 2.f * p) - sawSlewed) * slewCoeff; return sawSlewed;
			case 3:  squareSlewed += ((p < 0.5f ? 5.f : -5.f) - squareSlewed) * slewCoeff; return squareSlewed;
			default: randomSlewed += (randomValue - randomSlewed) * slewCoeff; return randomSlewed;
		}
	}

	void process(const ProcessArgs& args) override {
		if (args.sampleTime != lastSampleTime) {
			lastSampleTime = args.sampleTime;
			constexpr float DECLICK_TIME = 0.001f;
			constexpr float GAIN_DECLICK_TIME = 0.005f;
			waveSlewCoeff = 1.f - std::exp(-args.sampleTime / DECLICK_TIME);
			gainSlewCoeff = 1.f - std::exp(-args.sampleTime / GAIN_DECLICK_TIME);
		}

		// ── Tremolo: rising edge toggles on/off, from either the panel button or
		//     a trigger on TREMOLOCV_INPUT (Schmitt-conditioned, fires above 0.5V) ──
		bool tremoloBtn = params[TREMOLO_PARAM].getValue() > 0.5f;
		bool tremoloBtnRising = tremoloBtn && !tremoloBtnPrev;
		tremoloBtnPrev = tremoloBtn;
		bool tremoloTrigRising = tremoloTrigInput.process(inputs[TREMOLOCV_INPUT].getVoltage(), 0.1f, 0.5f);
		if (tremoloBtnRising || tremoloTrigRising)
			tremoloState = !tremoloState;

		// ── Shape button: rising edge advances to the next waveform, wrapping
		//     Random back to Sine ──
		bool shapeBtn = params[SHAPE_PARAM].getValue() > 0.5f;
		if (shapeBtn && !shapeBtnPrev)
			shapeIndex = (shapeIndex + 1) % 5;
		shapeBtnPrev = shapeBtn;

		// ── Shape selector: button-selected shapeIndex, offset by CV (10V = full
		//     sweep across all 5 shapes) ──
		float shapeCv = inputs[SHAPECV_INPUT].getVoltage();
		float shapeVal = clamp(float(shapeIndex) + shapeCv / 10.f * 4.f, 0.f, 4.f);
		const bool updateLights = lightDivider.process();
		const float lightTime = args.sampleTime * LIGHT_DIVISION;
		if (updateLights) {
			int shapeDisplayIdx = clamp((int)std::round(shapeVal), 0, 4);
			for (int i = 0; i < 5; i++)
				lights[SHAPELED_SINE_LIGHT + i].setBrightnessSmooth(i == shapeDisplayIdx ? 1.f : 0.f, lightTime);
		}

		// ── LFO frequency: free-running Hz knob, or ext-clock div/mult when patched ──
		extClockMode = inputs[EXTCLOCK_INPUT].isConnected();
		float timeCv = inputs[TIMECV_INPUT].getVoltage();
		float freq;
		if (extClockMode) {
			extClockTimer += args.sampleTime;

			// CV still steers the division/multiplication selector (10V = full sweep)
			float divKnobVal = clamp(params[TIME_PARAM].getValue() + timeCv / 10.f, 0.f, 1.f);
			timeDivMultIdx = divMultFactorIdx(divKnobVal);
			ClkFactor factor = clkFactors[timeDivMultIdx];

			if (extClockTrig.process(inputs[EXTCLOCK_INPUT].getVoltage(), 0.1f, 1.f)) {
				if (extClockTimer > 1e-4f) {
					extClockPeriod = extClockTimer;
					extClockPeriodValid = true;
				}
				extClockTimer = 0.f;

				// Hard-sync phase on qualifying incoming edges — a pure frequency
				// estimate free-runs the phase accumulator with no correction, so
				// any measurement jitter (sample-rate quantization of the period,
				// tempo wobble, etc.) accumulates into audible long-term drift
				// against the actual clock. Resetting phase periodically bounds
				// the drift instead of letting it grow unchecked.
				if (factor.mult) {
					// Multiply: every incoming edge is a fresh cycle boundary.
					phase = 0.f;
				} else {
					// Divide: only every Nth incoming edge lands on a boundary.
					extClockPulseCount++;
					if (extClockPulseCount >= factor.n) {
						extClockPulseCount = 0;
						phase = 0.f;
					}
				}
			}

			float extFreq = extClockPeriodValid ? (1.f / extClockPeriod) : 0.f;
			freq = factor.mult ? extFreq * factor.n : extFreq / factor.n;
			freq = clamp(freq, 0.f, 2000.f);
		} else {
			extClockTimer = 0.f;
			extClockPeriodValid = false;
			extClockPulseCount = 0;
			extClockTrig.reset();
			const float timeKnob = params[TIME_PARAM].getValue();
			if (timeKnob != lastTimeKnob) {
				lastTimeKnob = timeKnob;
				cachedBaseFreq = timeKnobToFreq(timeKnob);
			}
			if (timeCv != lastTimeCv) {
				lastTimeCv = timeCv;
				cachedTimeCvMult = std::pow(2.f, timeCv);
			}
			freq = clamp(cachedBaseFreq * cachedTimeCvMult, TIME_FREQ_MIN, TIME_FREQ_MAX);
		}

		// Bias the draw toward the extremes (power curve on a bipolar uniform draw)
		// so Random punches harder than flat uniform without landing at the edges
		// as often as a full sqrt curve — still fully spans ±5V.
		auto stepRandom = [&]() {
			constexpr float RANDOM_BIAS_EXP = 0.7f;
			float u = random::uniform() * 2.f - 1.f; // -1..1 uniform
			float biased = std::copysign(std::pow(std::abs(u), RANDOM_BIAS_EXP), u);
			randomValue = 5.f * biased;
		};

		phase += freq * args.sampleTime;
		if (phase >= 1.f)
			phase -= 1.f;
		randomPhase += 2.f * freq * args.sampleTime;
		if (randomPhase >= 1.f) {
			randomPhase -= 1.f;
			stepRandom();
		}

		// ── Master LFO value (single waveform; no phase-shift trick — that broke
		//     Random, which doesn't depend on phase, and Saw, which isn't
		//     anti-symmetric under a half-period shift) ──
		float lfo = computeWave(phase, shapeVal);

		// ── Amount/depth: attenuverter, CV integrated (10V = full sweep). Negative
		//     Amount flips the pan/tremolo direction (like the old Invert control),
		//     magnitude sets depth exactly as before. ──
		float amountCv = inputs[AMOUNTCV_INPUT].getVoltage();
		float depthRaw = clamp(params[AMOUNT_PARAM].getValue() + amountCv / 10.f, -1.f, 1.f);
		bool  negAmount = depthRaw < 0.f;
		float depthMag = std::abs(depthRaw);
		// Reverse-expo taper: fast rise near 0, flattens out approaching full depth
		if (depthMag != lastDepthMag) {
			lastDepthMag = depthMag;
			cachedDepth = 1.f - std::pow(1.f - depthMag, 1.4f);
		}
		const float depth = cachedDepth;

		float effectiveLfo = negAmount ? -lfo : lfo;

		// ── Time LEDs: brightness scales with Amount/depth as before; color now
		//     shows Amount's sign — red when negative, blue when positive in pan
		//     mode; yellow when negative, green when positive in tremolo mode —
		//     while amplitude still drives brightness in both directions ──
		auto setTimeLed = [&](int lightId, float value) {
			float scaled = value * depth;
			if (tremoloState) {
				lights[lightId + 0].setBrightnessSmooth(negAmount ? scaled : 0.f, lightTime); // R (yellow = R+G)
				lights[lightId + 1].setBrightnessSmooth(scaled, lightTime);                    // G
				lights[lightId + 2].setBrightnessSmooth(0.f, lightTime);                       // B
			} else {
				lights[lightId + 0].setBrightnessSmooth(negAmount ? scaled : 0.f, lightTime);  // R
				lights[lightId + 1].setBrightnessSmooth(0.f, lightTime);                       // G
				lights[lightId + 2].setBrightnessSmooth(negAmount ? 0.f : scaled, lightTime);  // B
			}
		};
		float effectiveLfoNorm = effectiveLfo / 5.f;
		if (!updateLights) {
			// LEDs are refreshed at a divided rate (see LIGHT_DIVISION)
		} else if (tremoloState) {
			// Half-wave: only the positive portion of the LFO drives brightness,
			// negative portion clips to 0. Sign-preserving, so Amount inversion
			// (which flips effectiveLfo's sign) still changes which half is "on".
			float pos = std::max(0.f, effectiveLfoNorm);
			setTimeLed(TIMELEDLEFT_LIGHT, pos);
			setTimeLed(TIMELEDRIGHT_LIGHT, pos);
		} else {
			setTimeLed(TIMELEDLEFT_LIGHT, std::max(0.f, -effectiveLfoNorm));
			setTimeLed(TIMELEDRIGHT_LIGHT, std::max(0.f, effectiveLfoNorm));
		}

		// Analog-style VCA: CV = unity bias + depth-attenuated bipolar LFO, same as
		// patching the LFO straight into a VCA's CV input normalled to unity gain.
		// v is the raw ±5V (10Vpp) LFO; at full Amount (depth=1) the entire swing
		// hits the VCA's CV input unattenuated — 5V of CV = 1.0x gain change
		// (textbook unity sensitivity).
		// Curve trimpot blends the VCA's CV response from linear (0) to exponential
		// (1) — one continuous curve across the whole ±5V sweep, same as a real
		// lin/exp VCA response switch. NOT a symmetric fold around center (that
		// pins the center at unity and just "punches" both peaks); the exponential
		// curve here stays closed near the bottom of the sweep and only opens up
		// near the top, so raising Curve makes the audio chop/shut-off harder and
		// stay closed longer, exactly like an analog exponential CV response.
		float curveAmt = params[CURVE_PARAM].getValue();
		auto gainFromLfo = [&](float v) -> float {
			float normV = (v + 5.f) * 0.1f; // 0..1, monotonic across the full ±5V sweep
			// At Curve = 0 the blend is exactly linear, so skip the two std::exp calls.
			float normVBlend = normV;
			if (curveAmt != 0.f) {
				float normVExpo = (std::exp(VCA_EXPO_K * normV) - 1.f) * invExpoRange;
				normVBlend = normV + curveAmt * (normVExpo - normV);
			}
			float shaped = normVBlend * 2.f - 1.f; // back to -1..1
			return std::max(0.f, 1.f + depth * shaped);
		};

		float gainL, gainR;
		if (tremoloState) {
			// Half-wave rectify for tremolo: only the positive portion of the LFO
			// drives the envelope, negative portion clips to 0 (silent). Remap the
			// clipped 0..5V portion back to a full -5..5 sweep so the Curve response
			// still spans its whole intended range. Sign-preserving (unlike full
			// rectification), so Amount inversion still changes which half is "on".
			float halfRect = std::max(0.f, effectiveLfo) * 2.f - 5.f;
			gainL = gainR = gainFromLfo(halfRect);
		} else {
			// Negative lfo → left louder (pan left); positive lfo → right louder (pan right)
			gainL = gainFromLfo(-effectiveLfo);
			gainR = gainFromLfo(effectiveLfo);
		}

		// De-click: slew the final gain rather than applying it raw, so toggling
		// Tremolo (or any other instantaneous gain-function change) doesn't pop.
		gainLSlewed += (gainL - gainLSlewed) * gainSlewCoeff;
		gainRSlewed += (gainR - gainRSlewed) * gainSlewCoeff;

		// ── Overall output gain rises with Amount/depth, independent of the LFO
		//     modulation shape — simple makeup boost as the knob turns up ──
		constexpr float AMOUNT_OUTPUT_GAIN = 0.15f;
		float outputGain = 1.f + depth * AMOUNT_OUTPUT_GAIN;
		const float outGainL = outputGain * gainLSlewed;
		const float outGainR = outputGain * gainRSlewed;

		// ── Polyphonic audio (like the rest of Producer Pack): one shared LFO, the
		//     same pan/tremolo gain applied to every voice. Right is normalized from
		//     left per voice when unpatched. ──
		const bool stereoIn = inputs[INR_INPUT].isConnected();
		const int nL = inputs[INL_INPUT].getChannels();
		const int nR = stereoIn ? inputs[INR_INPUT].getChannels() : nL;
		const int n = std::max(nL, nR);
		outputs[OUTL_OUTPUT].setChannels(n);
		outputs[OUTR_OUTPUT].setChannels(n);

		for (int c = 0; c < n; c++) {
			float inL = inputs[INL_INPUT].getPolyVoltage(c);
			float inR = stereoIn ? inputs[INR_INPUT].getPolyVoltage(c) : inL;

			// ── Output, soft tanh clamp at ±10V (fast rational tanh, see fast_math.hpp) ──
			outputs[OUTL_OUTPUT].setVoltage(10.f * fastTanh(inL * outGainL * 0.1f), c);
			outputs[OUTR_OUTPUT].setVoltage(10.f * fastTanh(inR * outGainR * 0.1f), c);
		}
	}

	// shapeIndex and tremoloState are plain module state driven by momentary
	// buttons (not Params, which Rack auto-saves) — without this they'd silently
	// reset to Sine/Off on every patch load.
	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "shapeIndex", json_integer(shapeIndex));
		json_object_set_new(rootJ, "tremoloState", json_boolean(tremoloState));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* shapeIndexJ = json_object_get(rootJ, "shapeIndex");
		if (shapeIndexJ)
			shapeIndex = clamp((int)json_integer_value(shapeIndexJ), 0, 4);

		json_t* tremoloStateJ = json_object_get(rootJ, "tremoloState");
		if (tremoloStateJ)
			tremoloState = json_boolean_value(tremoloStateJ);
	}
};

std::string TimeQuantity::getDisplayValueString() {
	auto* m = static_cast<AutoPan*>(module);
	if (m->extClockMode)
		return clkFactorLabels[m->timeDivMultIdx];
	float f = getDisplayValue();
	return string::f(f < 10.f ? "%.2f" : "%.1f", f);
}
std::string TimeQuantity::getUnit() {
	return static_cast<AutoPan*>(module)->extClockMode ? "" : " Hz";
}

std::string TremoloQuantity::getDisplayValueString() {
	return static_cast<AutoPan*>(module)->tremoloState ? "On" : "Off";
}

std::string ShapeQuantity::getDisplayValueString() {
	static const char* names[5] = {"Sine", "Triangle", "Saw", "Square", "Random"};
	int idx = clamp(static_cast<AutoPan*>(module)->shapeIndex, 0, 4);
	return names[idx];
}


struct AutoPanWidget : ModuleWidget {
	AutoPanWidget(AutoPan* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/AutoPan.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Positions synced to the "components" layer placeholders in AutoPan.svg.
		// NOTE: this SVG's viewBox uses px user-units (96dpi) rather than 1:1 mm
		// like the previous version, so circle cx/cy were converted via /(96/25.4)
		// to get millimeters for mm2px() here.
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(11.2882, 20.3713)), module, AutoPan::AMOUNT_PARAM));
		addParam(createParamCentered<Davies1900hBlack>(mm2px(Vec(11.2882, 44.3709)), module, AutoPan::TIME_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(11.2900, 65.1161)), module, AutoPan::CURVE_PARAM));
		{
			auto* tb = createParamCentered<TL1105>(mm2px(Vec(29.6868, 65.1161)), module, AutoPan::SHAPE_PARAM);
			tb->momentary = true;
			addParam(tb);
		}
		{
			auto* tb = createParamCentered<TL1105>(mm2px(Vec(29.5742, 20.3660)), module, AutoPan::TREMOLO_PARAM);
			tb->momentary = true;
			addParam(tb);
		}
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.3047, 83.5312)), module, AutoPan::AMOUNTCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.2668, 111.5295)), module, AutoPan::EXTCLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.3047, 97.5295)), module, AutoPan::INL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.2668, 83.5312)), module, AutoPan::TIMECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.3047, 111.5295)), module, AutoPan::INR_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(34.2288, 83.5312)), module, AutoPan::SHAPECV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.2668, 97.5295)), module, AutoPan::TREMOLOCV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(34.2288, 97.5295)), module, AutoPan::OUTL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(34.2288, 111.5295)), module, AutoPan::OUTR_OUTPUT));

		// Shape LEDs are a vertical column (all at x≈27.45mm) in the new layout,
		// converted from <ellipse> placeholders (SineLED/RampLED/TriangleLED/
		// SquareLED/RandomLED) on the components layer, not <circle>.
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(27.4527, 53.8165)), module, AutoPan::SHAPELED_SINE_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(27.4527, 44.1786)), module, AutoPan::SHAPELED_SAW_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(27.4527, 48.9976)), module, AutoPan::SHAPELED_TRIANGLE_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(27.4527, 39.3596)), module, AutoPan::SHAPELED_SQUARE_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(27.4527, 34.5407)), module, AutoPan::SHAPELED_RANDOM_LIGHT));

		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(27.0816, 14.2982)), module, AutoPan::TIMELEDLEFT_LIGHT));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(32.0687, 14.2982)), module, AutoPan::TIMELEDRIGHT_LIGHT));
	}
};


Model* modelAutoPan = createModel<AutoPan, AutoPanWidget>("AutoPan");
