#pragma once

#include <thread>
#include <mutex>
#include <memory>

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
	template<typename T>
	class SOS
	{
	public:
		struct Coeffs
		{
			T b0{ 1 };
			T b1{ 0 };
			T b2{ 0 };
			T a1{ 0 };
			T a2{ 0 };

		};

		/**
			Processes a single filter sample
			@param input input sample
			@return output sample
		*/
		inline T tick(const T & input);

		/**
			Directly sets coefficients b0, b1, b2 (feed forward) and a1, a2 (feed backward)
		*/
		inline void setCoeffs(const T& b0, const T& b1, const T& b2, const T& a1, const T& a2);

		/**
			Directly sets coefficients via coeffs struct
		*/
		inline void setCoeffs(Coeffs coeffs);

		/**
		Returns the current coefficients
		*/
		inline Coeffs getCoeffs() { return coeffs; };
		
	private:
		T z1{ 0 };
		T z2{ 0 };

		Coeffs coeffs;

	};

	struct FilterBank
	{
		std::vector<SOS<float>> parallelFilters;

		float b0;
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
	void createNewFilterBank(unsigned int order, float lambda);

protected:
	virtual void onImpulseResponseUpdated() override;

private:

	
	/**
		Implements the prony algorithm for FIR to IIR approximation, for fixed order = orderFB = orderFF
		@param h the FIR coefficients
		@parma order the order of the returned IIR coefficients
		@return the b (feed forward) and a (feed backward) coefficients with pair.first is b.
	*/
	static std::pair<std::vector<double>,std::vector<double>> prony(std::vector<double> h, unsigned int iirOrder);


	/**
		Finds roots of polynom @p polynom
		@param polynom the coefficeints of a polynom where polynom.back() is the x^0 coefficient
	*/
	static std::vector<std::complex<double>> roots(std::vector<double> polynom);

	/**
		Does stuff.
	*/
	static FilterBank approximateIR(std::vector<double> ir, std::vector<std::complex<double>> poles, unsigned int firOrder);

	/**
		Returns the coefficients for a complex polynomial. Assumes that the complex roots are conjugates and thus only returns the real part
	*/
	static std::vector<double> poly(std::vector<std::complex<double>> roots);

	/**
		Sorts complex poles. 
		- removes poles that are very close to zero
		- moves poles close to the real axis on the real axis
		- returns a list of complex conjugate poles followed by real poles. (If odd, the last pole is real)
	*/
	static std::vector<std::complex<double>> sortPoles(std::vector<std::complex<double>> input);

private:

	void checkUpdatedFilterbank();

private:

	// sync stuff

	std::mutex filterBankSyncMutex;

	bool newFilterBankReady{ false };
	std::unique_ptr<FilterBank> offlineFilterBank;
	std::unique_ptr<FilterBank> onlineFilterBank;

	unsigned int order{ 5 };
	float lambda{ 0 };

};

template<typename T>
inline T ParFiltConvolution::SOS<T>::tick(const T& input)
{
	auto out = coeffs.b0 * input + z1;
	z1		 = coeffs.b1 * input + z2 - coeffs.a1 * out;
	z2		 = coeffs.b2 * input      - coeffs.a2 * out;

	return out;
}

template<typename T>
inline void ParFiltConvolution::SOS<T>::setCoeffs(const T& b0, const T& b1, const T& b2, const T& a1, const T& a2)
{
	this->coeffs.b0 = b0; 
	this->coeffs.b1 = b1;
	this->coeffs.b2 = b2;
	this->coeffs.a1 = a1;
	this->coeffs.a2 = a2;
}

template<typename T>
inline void ParFiltConvolution::SOS<T>::setCoeffs(Coeffs coeffs)
{
	this->coeffs = coeffs;
}
