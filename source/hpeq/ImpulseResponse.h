
#pragma once

#include <vector>
#include <assert.h>  
#include "AFourierTransform.h"

/**
	Small utility class that stores an uneditable stereo impulse response 
*/
class ImpulseResponse
{

public:
	/**
		Creates the internal buffers by copying the vectors
	*/
	inline ImpulseResponse(std::vector<float> left, std::vector<float> right);


	/**
		Creates an dirac impulse response
	*/
	inline ImpulseResponse();

	/**
		Creates an ImpulseResponse by copying the argument buffers.
	*/
	inline ImpulseResponse(const float*  left, const float * right, unsigned int numSamples);


public:
	/**
		Converts the impulse response to a monophonic IR
	*/
	inline void makeMono();


public:
	/**
		Returns left read pointer.
	*/
	inline const float * getLeft()  const;

	/**
		Returns right read pointer.
	*/
	inline const float * getRight() const;

	/**
		Returns number of samples in the Impulse Response.
	*/
	inline unsigned int getSize() const;


private:
	std::vector<float> left;
	std::vector<float> right;
};


ImpulseResponse::ImpulseResponse(std::vector<float> left, std::vector<float> right) :
	left(left), right(right)
{
	assert(left.size() == right.size());
	assert(left.size() > 0);
}

inline ImpulseResponse::ImpulseResponse() : ImpulseResponse({ 1 }, { 1 })
{}

inline ImpulseResponse::ImpulseResponse(const float * left, const float * right, unsigned int numSamples)
{
	this->left.reserve(numSamples);
	this->right.reserve(numSamples);
	assert(numSamples > 0);

	for (int i = 0; i < numSamples; i++)
	{
		this->left.push_back(left[i]);
		this->right.push_back(right[i]);
	}
}

inline void ImpulseResponse::makeMono()
{
	for (int i = 0; i < getSize(); i++)
	{
		auto sum = 0.5f * (left[i] + right[i]);
		left[i] = right[i] = sum;
	}
}


inline const float * ImpulseResponse::getLeft() const
{
	return left.data();
}

inline const float * ImpulseResponse::getRight() const
{
	return right.data();
}

inline unsigned int ImpulseResponse::getSize() const
{
	return left.size();
}
