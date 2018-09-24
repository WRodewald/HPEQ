#pragma once

#include "AFourierTransform.h"
#include <memory>

/**
Simple factory that can create a fourier transform
*/
class AFourierTransformFactory
{

public:
	static void installStaticFactory(AFourierTransformFactory * factory);

	static AFourierTransform * FourierTransform(unsigned int order);

protected:
	virtual AFourierTransform * createFourierTransform(unsigned int order) const = 0;

private:

	static std::unique_ptr<const AFourierTransformFactory> factoryInstance;
};
