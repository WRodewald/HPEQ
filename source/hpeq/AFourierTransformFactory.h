#pragma once

#include "AFourierTransform.h"
#include <memory>

/**
	Class implements a simple factory that can create a fourier transform engines
*/
class AFourierTransformFactory
{

public:
	/**
		Installs the factory as a singleton to be. The singleton will be used to create fourier transform enignes
		via #FourierTransform
	*/
	static AFourierTransformFactory * installStaticFactory(AFourierTransformFactory * factory);

	/**
		Creates a fourier transform engine with given order @p order. The function forwards the call to #createFourierTransform
		of the currently installed #AFourierTransformFactory singleton.
		@param order the order
		@return the created fourier transform engine. The caller takes ownership.
	*/
	static AFourierTransform * FourierTransform(unsigned int order);

protected:
	/**
		The virtual factory function creates a fourier transform engine.
		@param order the order
		@return the created fourier transform engine. The caller takes ownership.
	*/
	virtual AFourierTransform * createFourierTransform(unsigned int order) const = 0;

private:

	static std::unique_ptr<const AFourierTransformFactory> factoryInstance;
};
