#include "plugin.hpp"

#ifdef METAMODULE_BUILTIN
extern Plugin *pluginInstance;
void init_ProducerPack(Plugin *p) {
#else
Plugin *pluginInstance;
void init(Plugin *p) {
#endif
	pluginInstance = p;

	// Add modules here
	p->addModel(modelDJFilter);
	p->addModel(modelSpatializer);
	p->addModel(modelStereoWidth);
	p->addModel(modelStereoCrossfader);
	p->addModel(modelSeventiesEQ);
	p->addModel(modelSeventiesComp);
	p->addModel(modelBoost);
	p->addModel(modelDecay);
	p->addModel(modelAuxSends);
	p->addModel(modelBitcrusher);
	p->addModel(modelDrumBus);

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
