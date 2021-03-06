#pragma once

#include <thread>
#include <mutex>
#include <memory>
#include <array>

#include "ASyncedConvolutionEngine.h"
#include "ThreadSyncable.h"

namespace
{
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


	/**
		A simple low order FIR filter topology
	*/
	template<typename T, unsigned int MaxOrder>
	class FIRFilter
	{
	public:
		struct Coeffs
		{
			std::array<T, MaxOrder + 1> b;
		};

	public:
		FIRFilter()
		{
			z.fill(0);
			coeffs.b.fill(0);
			coeffs.b[0] = 1;
		}

	

		/**
			Processes a single filter sample
			@param input input sample
			@return output sample
		*/
		inline T tick(const T & input);

		/**
			Directly sets coefficients via coeffs struct
		*/
		inline void setCoeffs(Coeffs coeffs);

		/**
			Returns the current coefficients
		*/
		inline Coeffs getCoeffs() { return coeffs; };

	private:
		std::array<T, MaxOrder> z;

		Coeffs coeffs;
		unsigned int curOrder{ 1 };

	};

	/**
		A small unility class that contains filters to be used during processing.
	*/
	struct FilterBank
	{
		std::vector<SOS<float>> parallelFilters;

		FIRFilter<float, 16> firFilter;
	};

}

/**
	A convolution engine that implements the parallel filterbank IIR approximation method discussed in Bank 2007. 
	(Direct design of parallel second-order filters for instrument bodymodeling) 
	Work In Progress!
*/
class ParFiltConvolution : public ASyncedConvolutionEngine<FilterBank>
{
public:
	/**
		Maximum size of target FIR filter. If higher, FIR will be truncated after min phase conversion.
	*/
	static const int MaxInputFIRSize{ 4096 }; 

public:

	ParFiltConvolution();

	// Inherited via AConvolutionEngine
	virtual void process(const float * readL, const float * readR, float * writeL, float * writeR, unsigned int numSamples) override;

	
	/**
		Sets the warp coefficient lambda for the IIR approximation. Does not force an update of the filter bank.
		@parm lambda the warp coefficeints used in the IIR approximation algorithm. Should be between 0..1
	*/
	void setWarpCoefficient(float lambda);

	/**
		Sets the size of the parallel filter bank. Note that the number of #SOS filters actually used might be less then the number 
		set with this function.
		@param numSOSFilters Number of #SOS filters in the parallel filter bank. 
		@param firOrder Order of parallel FIR section. 
	*/
	void setFilterBankSize(unsigned int numSOSFilters, unsigned int firOrder);

	/**
		Creates a new filter bank with given @p lambda and @p numSOSFilters. The function is static to help with
		multi threading robustness. 
		@param ir	the impulse response that will be approximated
		@parm lambda the warp coefficeints used in the IIR approximation algorithm. Should be between 0..1
		@param numSOSFilters Number of #SOS filters in the parallel filter bank. 
		@param firOrder Order of parallel FIR section. 
	*/
	static FilterBank createNewFilterBank(ImpulseResponse ir, float lambda, unsigned int numSOSFilters, unsigned int firOrder);
	
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
		Performs a least mean square approximation to find the feed forward coefficeints of a IIR filter with a given bigger FIR response
		and a set of poles (feed back coefficients). Work in Progress
		@param ir the target FIR filter
		@param poles a set of (complex) poles used as a strarting point
		@param firOrder the order of the parallel fir element
		@return returns the ready to go filter bank 
	*/
	static FilterBank approximateIR(std::vector<double> ir, std::vector<std::complex<double>> poles, unsigned int firOrder);

	/**
		Returns the coefficients for a complex polynomial. Assumes that the complex roots are conjugates and thus only returns the real part
		@param roots the roots of a polynomial
		@return the associated coefficients were coefficients.back() is the constant element (x^0)
	*/
	static std::vector<double> poly(std::vector<std::complex<double>> roots);

	/**
		Sorts  and cleans a set of complex poles. 
		- removes poles that are very close to zero
		- moves poles close to the real axis on the real axis
		- returns a list of complex conjugate poles followed by real poles. (If odd, the last pole is real)
	*/
	static std::vector<std::complex<double>> sortPoles(std::vector<std::complex<double>> input);



private:
	
	unsigned int numSOSSections{ 32 };
	unsigned int firOrder{ 0 };
	float lambda{ 0 };


	// Inherited via ASyncedConvolutionEngine
	virtual FilterBank preProcess(const ImpulseResponse & ir) override;

};

template<typename T>
inline T SOS<T>::tick(const T& input)
{
	auto out = coeffs.b0 * input + z1;
	z1		 = coeffs.b1 * input + z2 - coeffs.a1 * out;
	z2		 = coeffs.b2 * input      - coeffs.a2 * out;

	return out;
}

template<typename T>
inline void SOS<T>::setCoeffs(const T& b0, const T& b1, const T& b2, const T& a1, const T& a2)
{
	this->coeffs.b0 = b0; 
	this->coeffs.b1 = b1;
	this->coeffs.b2 = b2;
	this->coeffs.a1 = a1;
	this->coeffs.a2 = a2;
}

template<typename T>
inline void SOS<T>::setCoeffs(Coeffs coeffs)
{
	this->coeffs = coeffs;
}

template<typename T, unsigned int MaxOrder>
inline T FIRFilter<T, MaxOrder>::tick(const T & input)
{
	auto out = this->coeffs.b[0] * input + this->z[0];
	for (int i = 0; i < (static_cast<int>(curOrder)- 1); i++)
	{
		this->z[i] = this->coeffs.b[i + 1] * input + z[i + 1];
	}
	z[curOrder - 1] = this->coeffs.b[curOrder] * input;
	return out;
}

template<typename T, unsigned int MaxOrder>
inline void FIRFilter<T, MaxOrder>::setCoeffs(Coeffs coeffs)
{
	this->coeffs = coeffs;
	
	curOrder = MaxOrder;
	while (curOrder > 0 && (coeffs.b[curOrder] == 0)) curOrder--;
	curOrder = std::max(curOrder, 1U);
}