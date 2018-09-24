#include "AFourierTransformFactory.h"

std::unique_ptr<const AFourierTransformFactory> AFourierTransformFactory::factoryInstance;

AFourierTransformFactory * AFourierTransformFactory::installStaticFactory(AFourierTransformFactory * factory)
{
	factoryInstance = std::unique_ptr<AFourierTransformFactory>(factory);
	return factory;
}

AFourierTransform * AFourierTransformFactory::FourierTransform(unsigned int order)
{
	if (factoryInstance) return factoryInstance->createFourierTransform(order);
	return nullptr;
}
