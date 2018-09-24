#include "AFourierTransformFactory.h"

std::unique_ptr<const AFourierTransformFactory> AFourierTransformFactory::factoryInstance = nullptr;

void AFourierTransformFactory::installStaticFactory(AFourierTransformFactory * factory)
{
	factoryInstance = std::unique_ptr<AFourierTransformFactory>(factory);
}

AFourierTransform * AFourierTransformFactory::FourierTransform(unsigned int order)
{
	if (factoryInstance) return factoryInstance->createFourierTransform(order);
}
