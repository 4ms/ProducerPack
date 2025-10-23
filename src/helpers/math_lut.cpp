#include "math_lut.hpp"
#include <cmath>

struct SinTableRange {
	static constexpr float min = -2.f * M_PI;
	static constexpr float max = 2.f * M_PI;
};

const Mapping::LookupTable_t<64, float> Sin =
	Mapping::LookupTable_t<64, float>::generate<SinTableRange>([](float x) { return sinf(x); });

struct CosTableRange {
	static constexpr float min = -2.f * M_PI;
	static constexpr float max = 2.f * M_PI;
};

const Mapping::LookupTable_t<64, float> Cos =
	Mapping::LookupTable_t<64, float>::generate<CosTableRange>([](float x) { return cosf(x); });

struct ExpTableRange {
	static constexpr float min = -2.f;
	static constexpr float max = 2.f;
};

const Mapping::LookupTable_t<64, float> Exp =
	Mapping::LookupTable_t<64, float>::generate<ExpTableRange>([](float x) { return std::exp(x); });

struct LogTableRange {
	static constexpr float min = 2.f;
	static constexpr float max = -2.f;
};

const Mapping::LookupTable_t<64, float> Log =
	Mapping::LookupTable_t<64, float>::generate<LogTableRange>([](float x) { return std::log(x); });

	struct Log2TableRange {
		static constexpr float min = 20.f;
		static constexpr float max = 7000.f;
	};
	
	const Mapping::LookupTable_t<64, float> Log2 =
		Mapping::LookupTable_t<64, float>::generate<Log2TableRange>([](float x) { return std::log2(x); });
	
