#pragma once

#include <mutex>


/**
	A small utility class that helps to sync bigger data via main and audio thread with a combination of
	locks and pointer swap.
*/
template <typename T>
class ThreadSyncable
{
public:
	/**
		Sets the content from the source thread (main / 3rd thread). Should not be called from a time sensitive thread.
		@param data the data to be set.
	*/
	void set(const T& data);

	/**
		Polls for an update and if necessary swaps the live thread data with the offline data. 
		Should be called from the time sensitive thread. Function locks a mutex shared with #set but generally has a low CPU footprint. (No allocation)
	*/
	void update();

	/**
		Returns a raw access to the live data. Might return nullptr. 
	*/
	T * get();

private:
	std::mutex syncMutex;
	bool updated{ false };

	std::unique_ptr<T> offlineData;
	std::unique_ptr<T> liveData;
};

template<typename T>
inline void ThreadSyncable<T>::set(const T & data)
{
	std::lock_guard<std::mutex> lock(syncMutex);

	offlineData = std::unique_ptr<T>(new T(data));
	updated = true;
}

template<typename T>
inline void ThreadSyncable<T>::update()
{
	std::lock_guard<std::mutex> lock(syncMutex);

	if (updated)
	{
		std::swap(offlineData, liveData);
		updated = false;
	}
}

template<typename T>
inline T * ThreadSyncable<T>::get()
{
	return liveData.get();
}
