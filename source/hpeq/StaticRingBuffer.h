#pragma once

#include <array>
#include <cassert>
#include <cmath>

namespace
{
	constexpr bool isPowerOfTwo(int x)
	{
		return x && ((x & (x - 1)) == 0);
	}
}

/**
	A ring buffer of variable type with a static max size.
*/
template<typename T, unsigned int MaxSize>
class StaticRingBuffer
{
public:
	StaticRingBuffer(unsigned int length = 0);
	~StaticRingBuffer() = default;

	StaticRingBuffer(const  StaticRingBuffer &) = default;
	StaticRingBuffer(StaticRingBuffer &&) = default;
	
	StaticRingBuffer & operator=(const  StaticRingBuffer &) = default;
	StaticRingBuffer & operator=(StaticRingBuffer &&) = default;
	

public:
	/**
		Returns the current ringbuffer length (number of elements in the delay line)
	*/
	unsigned int getLength() const;

	/**
		Returns the current buffer size, that is, the length of the buffer used to create the ring buffer.
		getLength() <= getSize()
	*/
	unsigned int getSize() const;

	/**
		Function changes the length
	*/
	void setLength(unsigned int length);

	/**
		Function changes the length and updates the size to the closest sufficiently large power of two
	*/
	void setLengthAndResize(unsigned int length);

	/**
		Functions updates the internally used memory range for perfomrance. 
		Fills the std::array with a specific value
	*/
	void setSize(unsigned int size, const T & fillVal);

	/**
		Functions updates the internally used memory range for perfomrance
	*/
	void setSize(unsigned int size);

	/**
		Functions fills the internal std::array with a given value
	*/
	void fill(const T & val = T(0));

	/**
		Function writes, reads and increments.
	*/
	T tick(const T & input);


	/**
		Function increments the write Pos without altering the buffers content.
	*/
	void increment();

	/**
		Function returns a reference to an element, with readPos == writePos at idx = 0. 
	*/
	T & operator[](unsigned int readPos);

	/**
		Function performs linear interpolation, with readPos == writePos at idx = 0. 
	*/
	T readF(float readPos);


private:

	unsigned int length{ 0 };
	unsigned int size{ MaxSize };
	unsigned int mask{ MaxSize - 1 };

	std::array<T, MaxSize> buffer;
	unsigned int writePos{ 0 };

	
};

template<typename T, unsigned int MaxSize>
inline StaticRingBuffer<T, MaxSize>::StaticRingBuffer(unsigned int length)
{
	static_assert(isPowerOfTwo(MaxSize), "MaxSize should be a power of two");
	buffer.fill(T(0));
	setLength(length);
}

template<typename T, unsigned int MaxSize>
inline unsigned int StaticRingBuffer<T, MaxSize>::getLength() const
{
	return length;
}

template<typename T, unsigned int MaxSize>
inline unsigned int StaticRingBuffer<T, MaxSize>::getSize() const
{
	return size;
}

template<typename T, unsigned int MaxSize>
inline void StaticRingBuffer<T, MaxSize>::setLength(unsigned int length)
{
	assert(length < size);
	this->length = length;
}

template<typename T, unsigned int MaxSize>
inline void StaticRingBuffer<T, MaxSize>::setLengthAndResize(unsigned int length)
{
	this->length = length;
	unsigned int closestPow2Fit = std::pow(2, std::ceil(std::log2(length+1)));
	
	if (closestPow2Fit != size)
	{
		setSize(closestPow2Fit);
	}
}

template<typename T, unsigned int MaxSize>
inline void StaticRingBuffer<T, MaxSize>::fill(const T & val)
{
	buffer.fill(val);
}

template<typename T, unsigned int MaxSize>
inline T StaticRingBuffer<T, MaxSize>::tick(const T & input)
{
	unsigned int readPos  = (writePos-length + MaxSize) & mask;

	buffer[writePos] = input;
	
	increment();
	
	return buffer[readPos];
}

template<typename T, unsigned int MaxSize>
inline void StaticRingBuffer<T, MaxSize>::setSize(unsigned int size, const T & fillVal)
{
	setSize(size);
	fill(fillVal);
}

template<typename T, unsigned int MaxSize>
inline void StaticRingBuffer<T, MaxSize>::setSize(unsigned int size)
{
	assert(size <= MaxSize);
	assert(isPowerOfTwo(size));
	this->size = size;
	this->mask = size - 1;

	writePos = (writePos) & mask;
}

template<typename T, unsigned int MaxSize>
inline void StaticRingBuffer<T, MaxSize>::increment()
{
	writePos = (writePos + 1) & mask;
}

template<typename T, unsigned int MaxSize>
inline T & StaticRingBuffer<T, MaxSize>::operator[](unsigned int idx)
{
	unsigned int pos = (writePos - idx + MaxSize) & mask;

	return buffer[pos];
}


template<typename T, unsigned int MaxSize>
inline T StaticRingBuffer<T, MaxSize>::readF(float pos)
{
	unsigned int posInt = pos;
	float frac = pos - static_cast<float>(posInt);
	unsigned int pos1 = (writePos - posInt     + MaxSize) & mask;
	unsigned int pos2 = (writePos - (posInt+1) + MaxSize) & mask;

	auto y1 = buffer[pos1];
	auto y2 = buffer[pos2];

	return y1 + frac * (y2 - y1);
}