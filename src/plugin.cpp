#include "plugin.hpp"


Plugin* pluginInstance;


void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
	p->addModel(modelDJFilter);
	p->addModel(modelMonobass);
	p->addModel(modelSpatializer);
	p->addModel(modelStereoWidth);
	p->addModel(modelStereoCrossfader);
	p->addModel(model_70sEQ);
	p->addModel(model_70sComp);
	p->addModel(modelBoost);
	p->addModel(model_2op);
	p->addModel(modelDecay);
	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
