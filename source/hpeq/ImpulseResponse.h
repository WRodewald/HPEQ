
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
		@param left		left buffer
		@param right	right buffer
		@param fs		sample rate
	*/
	inline ImpulseResponse(std::vector<float> left, std::vector<float> right, float fs);


	/**
		Creates an dirac impulse response
	*/
	inline ImpulseResponse();

	/**
		Creates an ImpulseResponse by copying the argument buffers.
		@param left			pointer to left buffer
		@param right		pointer to right buffer
		@param numSamples	number of samples in left and right buffers
		@param fs			sample rate
	*/
	inline ImpulseResponse(const float*  left, const float * right, unsigned int numSamples, float fs);


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
		Returns left or right read pointer
		@param idx the channel index, where idx == 0 will return left and idx == 1 will return the right channel
	*/
	inline const float * getChannel(unsigned int idx)  const;

	/**
		Returns left read/write pointer.
	*/
	inline float * getLeft();

	/**
		Returns right read/write pointer.
	*/
	inline float * getRight();

	/**
		Returns left or right read pointer
		@param idx the channel index, where idx == 0 will return left and idx == 1 will return the right channel
	*/
	inline float * getChannel(unsigned int idx);


	/**
		Returns left vector
	*/
	inline const std::vector<float>& getLeftVector()  const;

	/**
		Returns right vector
	*/
	inline const std::vector<float> &getRightVector() const;

	/**
		Returns left or right vector
		@param idx the channel index, where idx == 0 will return left and idx == 1 will return the right channel
	*/
	inline const std::vector<float> & getVector(unsigned int idx) const;

	/**
		Returns number of samples in the Impulse Response.
	*/
	inline unsigned int getSize() const;

	/**
		Returns the impulse respons native sample rate.	
	*/
	inline float getSampleRate() const;

private:
	std::vector<float> left;
	std::vector<float> right;

	float sampleRate{ 44100 };
};


ImpulseResponse::ImpulseResponse(std::vector<float> left, std::vector<float> right, float fs) :
	left(left), right(right), sampleRate(fs)
{
	assert(left.size() == right.size());
	assert(left.size() > 0);
}

inline ImpulseResponse::ImpulseResponse() : ImpulseResponse({ 1 }, { 1 }, 44100)
{}

inline ImpulseResponse::ImpulseResponse(const float * left, const float * right, unsigned int numSamples, float fs) : sampleRate(fs)
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


inline const float * ImpulseResponse::getLeft() const
{
	return left.data();
}

inline const float * ImpulseResponse::getRight() const
{
	return right.data();
}

inline const std::vector<float> & ImpulseResponse::getLeftVector() const
{
	return left;
}

inline const std::vector<float> & ImpulseResponse::getRightVector() const
{
	return right;
}

inline const std::vector<float> & ImpulseResponse::getVector(unsigned int idx) const
{
	assert(idx < 2);
	return (idx == 0) ? left:  right;
}

inline const float * ImpulseResponse::getChannel(unsigned int idx) const
{
	assert(idx < 2);
	return (idx == 0) ? left.data() : right.data();
}

inline float * ImpulseResponse::getLeft()
{
	return left.data();
}

inline float * ImpulseResponse::getRight()
{
	return right.data();
}

inline float * ImpulseResponse::getChannel(unsigned int idx)
{
	assert(idx < 2);
	return (idx == 0) ? left.data() : right.data();
}

inline unsigned int ImpulseResponse::getSize() const
{
	return left.size();
}

inline float ImpulseResponse::getSampleRate() const
{
	return sampleRate;
}
