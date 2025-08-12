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
	p->addModel(modelSeventiesEQ);
	p->addModel(modelSeventiesComp);
	p->addModel(modelBoost);
	p->addModel(modelTwoOp);
	p->addModel(modelDecay);
	p->addModel(modelAuxSends);
	p->addModel(modelBitcrusher);
	p->addModel(modelDrumBus);
	p->addModel(modelOctopush);
	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
