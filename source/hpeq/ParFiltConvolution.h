#pragma once


#include "AConvolutionEngine.h"
/**
	A convolution engine that implements the parallel filterbank IIR approximation method discussed in Bank 2007. 
	(Direct design of parallel second-order filters for instrument bodymodeling) 
	Work In Progress!
*/
class ParFiltConvolution : public AConvolutionEngine
{
public:

	/**
		A simple second order section filter topology with low level coefficient access
	*/
	class SOS
	{
		/**
			Processes a single filter sample
			@param input input sample
			@return output sample
		*/
		inline float tick(float input);

		/**
			Directly sets coefficients b0, b1, b2 (feed forward) and a1, a2 (feed backward)
		*/
		inline void setCoeffs(float b0, float b1, float b2, float a1, float a2);

	private: 
		float z1{ 0 };
		float z2{ 0 };
		float b0{ 1 };
		float b1{ 0 };
		float b2{ 0 };
		float a1{ 0 };
		float a2{ 0 };
	};


public:

	ParFiltConvolution();

	// Inherited via AConvolutionEngine
	virtual void process(const float * readL, const float * readR, float * writeL, float * writeR, unsigned int numSamples) override;

	/**
		Updates the internel parallel filter bank
		@param order order of the filter bank were number of parallel second order elements N = 2^order
		@param lambda warping coefficient
	*/
	void updateFilterBank(unsigned int order, float lambda);

protected:
	virtual void onImpulseResponseUpdated() override;

private:

	/**
		Finds poles of the prony approximated IIR transfer function
	*/
	static std::vector<std::complex<double>> findPoles(const ImpulseResponse& ir, unsigned int order);

	/**
		Implements the prony algorithm for FIR to IIR approximation, for fixed order = orderFB = orderFF
		@param h the FIR coefficients
		@parma order the order of the returned IIR coefficients were number of coefficeints is order +1
		@return the b (feed forward) and a (feed backward) coefficients with pair.first is b.
	*/
	static std::pair<std::vector<double>,std::vector<double>> prony(std::vector<double> h, unsigned int order);


	/**
		Finds roots of polynom @p polynom
		@param polynom the coefficeints of a polynom where polynom.back() is the x^0 coefficient
	*/
	static std::vector<std::complex<double>> findRoots(std::vector<double> polynom);

private:

	std::vector<SOS> FilterBank;

	unsigned int order{ 5 };
	float lambda{ 0 };

};

inline float ParFiltConvolution::SOS::tick(float input)
{
	auto out = b0 * input + z1;
	z1		 = b1 * input + z2 - a1 * out;
	z2		 = b2 * input      - a2 * out;

	return out;
}

inline void ParFiltConvolution::SOS::setCoeffs(float b0, float b1, float b2, float a1, float a2)
{
	this->b0 = b0; 
	this->b1 = b1;
	this->b2 = b2;
	this->a1 = a1;
	this->a2 = a2;
}