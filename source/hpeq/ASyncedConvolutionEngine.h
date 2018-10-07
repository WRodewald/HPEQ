#pragma once

#include "AConvolutionEngine.h"
#include "ThreadSyncable.h"

/**
	#ASyncedConvolutionEngine implements a #AConvolutionEngine that manages the syncing
	of a datatype @p T between audio thread and main or 3rd thread.
*/
template<typename T>
class ASyncedConvolutionEngine : public AConvolutionEngine
{

protected:
	/**
		Pre Processing method called befor swaping live (audio thread) data and offline data.
		@param ir the input impulse response
		@return T the data used during audio processing
	*/
	virtual T preProcess(const ImpulseResponse &ir) = 0;

	virtual void onImpulseResponseUpdate() override final;

	/**
		Called after the data was updated. Called in audio thread.
	*/
	virtual void onDataUpdate() { }

	/**
		Has to be called during processing. Will check for a data update thread savely.
	*/
	void updateData();

	/**
		Will return the data pointer, might be null.
	*/
	T* getData();
	
private:

	ThreadSyncable<T> data;

};

template<typename T>
inline void ASyncedConvolutionEngine<T>::onImpulseResponseUpdate()
{
	data.set(preProcess(*getImpulseResponse()));
}

template<typename T>
inline void ASyncedConvolutionEngine<T>::updateData()
{
	bool updated = data.update();

	if (updated)
	{
		onDataUpdate();
	}
}

template<typename T>
inline T * ASyncedConvolutionEngine<T>::getData()
{
	return data.get();
}
