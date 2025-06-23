#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelDJFilter;
extern Model* modelMonobass;
extern Model* modelSpatializer;
extern Model* modelStereoWidth;
extern Model* modelStereoCrossfader;
extern Model* model_70sEQ;
extern Model* model_70sComp;
extern Model* modelBoost;