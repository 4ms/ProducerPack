#pragma once

// Pade [7/6] rational approximation of tanh: one divide instead of a libm tanh call
// (which is slow on MetaModule's Cortex-A7). Error < 1e-6 for |x| < 3 and < 1e-4
// everywhere, and the curve meets +/-1 smoothly at the clamp point, so it's inaudible
// as a saturator / soft clipper.
static inline float fastTanh(float x) {
	constexpr float clampX = 4.97f;
	if (x > clampX)
		return 1.f;
	if (x < -clampX)
		return -1.f;
	const float x2 = x * x;
	const float num = x * (135135.f + x2 * (17325.f + x2 * (378.f + x2)));
	const float den = 135135.f + x2 * (62370.f + x2 * (3150.f + x2 * 28.f));
	return num / den;
}
