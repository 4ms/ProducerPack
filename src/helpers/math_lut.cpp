#include "math_lut.hpp"
#include <cmath>

struct SinTableRange {
	static constexpr float min = -2.f * M_PI;
	static constexpr float max = 2.f * M_PI;
};

const Mapping::LookupTable_t<64, float> Sin =
	Mapping::LookupTable_t<64, float>::generate<SinTableRange>([](float x) { return sinf(x); });

struct ExpTableRange {
	static constexpr float min = -2.f;
	static constexpr float max = 2.f;
};

const Mapping::LookupTable_t<64, float> Exp =
	Mapping::LookupTable_t<64, float>::generate<ExpTableRange>([](float x) { return sinf(x); });
