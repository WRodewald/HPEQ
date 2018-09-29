#pragma once

#include <complex>

/**
	Class provides a generic interface for fourier transforms. Depending on the implementation, this might add overhead compared to using FFT APIs directly. 

*/
class AFourierTransform
{
public:
	/**
		Creates an AFourierTransform instance. May allocate memory dynamically.
		@param order The order of the fft where size (numer of samples / bins) is 2^order.
	*/
	inline AFourierTransform(unsigned int order);
	
	virtual ~AFourierTransform() = default;
	
	/**
		Performs the fft out-of-place. The implementation is required to not block or dynamically allocate memory.
		@param in  input buffer of size 2^order 
		@param out output buffer of size 2^order 
	*/
	virtual void performFFT(std::complex<float> *in, std::complex<float>* out) = 0;

	/**
		Performs the inverse fft out-of-place. The implementation is required to not block or dynamically allocate memory.
		@param in  input buffer of size 2^order 
		@param out output buffer of size 2^order 
	*/
	virtual void performIFFT(std::complex<float> *in, std::complex<float>* out) = 0;

	/**
		Performs the fft in-place. The implementation is required to not block or dynamically allocate memory.
		@param buffer  input/output buffer of size 2^order
	*/
	virtual void performFFTInPlace(std::complex<float> *buffer) = 0;

	/**
		Performs the inverse fft in-place. The implementation is required to not block or dynamically allocate memory.
		@param buffer  input/output buffer of size 2^order
	*/
	virtual void performIFFTInPlace(std::complex<float> *buffer) = 0;


	/**
		Returns the number of bins / samples supported by the FFT engine
	*/
	inline unsigned int getSize()  const { return size; }

	/**
		Returns the order of the FFT (where size = 2^order)
	*/
	inline unsigned int getOrder() const { return order; }

private:
	unsigned int order;
	unsigned int size;
};


inline AFourierTransform::AFourierTransform(unsigned int order) : order(order), size(1 << order)
{}