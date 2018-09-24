#pragma once

#include <array>
#include <cassert>
#include <cmath>

constexpr bool isPowerOfTwo(int x)
{
	return x && ((x & (x - 1)) == 0);
}

/**
	A queue buffer of variable type with a static max size.
*/
template<typename T, unsigned int MaxSize>
class StaticQueue
{
public:
	StaticQueue(unsigned int length = 0);
	~StaticQueue() = default;

	StaticQueue(const  StaticQueue &) = default;
	StaticQueue(StaticQueue &&) = default;
	
	StaticQueue & operator=(const  StaticQueue &) = default;
	StaticQueue & operator=(StaticQueue &&) = default;
	

public:

	/**
		Returns the maximum queue length.
	*/
	unsigned int getSize() const;
	
	/**
		Writes a sample to the queue head
	*/
	void push(const T & input);

	/**
		Writes N samples to the queue head
		@param input the sample to be written
		@param N	 number of samples to be written
	*/
	void push(const T & input, unsigned int N);

	/**
		Reads a sample from the queue tail
	*/
	T pull();

	/**
		Clears the queue and sets the delay back to 0
	*/
	void clear();

	/**
		Returns the number of samples currently stored in the queue.
	*/
	unsigned int getLength() const;

private:
	static const unsigned int Mask{ MaxSize - 1 };

	std::array<T, MaxSize> buffer;
	unsigned int writePos{ 0 };
	unsigned int readPos{ 0 };
	unsigned int len{ 0 };

	
};

template<typename T, unsigned int MaxSize>
inline StaticQueue<T, MaxSize>::StaticQueue(unsigned int length)
{
	static_assert(isPowerOfTwo(MaxSize), "MaxSize should be a power of two");
	buffer.fill(T(0));
}

template<typename T, unsigned int MaxSize>
inline unsigned int StaticQueue<T, MaxSize>::getSize() const
{
	return MaxSize;
}

template<typename T, unsigned int MaxSize>
inline void StaticQueue<T, MaxSize>::push(const T & input)
{
	len++;
	buffer[writePos] = input;
	writePos = (writePos + 1) & Mask;
}

template<typename T, unsigned int MaxSize>
inline void StaticQueue<T, MaxSize>::push(const T & input, unsigned int N)
{
	for (int i = 0; i < N; i++)
	{
		push(input);
	}
}

template<typename T, unsigned int MaxSize>
inline T StaticQueue<T, MaxSize>::pull()
{
	len--;
	auto out = 	buffer[readPos];
	readPos = (readPos + 1) & Mask;
	return out;
}

template<typename T, unsigned int MaxSize>
inline void StaticQueue<T, MaxSize>::clear()
{
	len = 0;
	writePos = readPos = 0;
	buffer[writePos] = 0;
}

template<typename T, unsigned int MaxSize>
inline unsigned int StaticQueue<T, MaxSize>::getLength() const
{
	return len;
}

