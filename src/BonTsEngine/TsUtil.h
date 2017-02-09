#pragma once


#include <functional>
#include <vector>


namespace TsEngine
{

	inline DWORD Load32(const void *p)
	{
		return _byteswap_ulong(*static_cast<const DWORD*>(p));
	}


	// 挿入ソート
	template<typename TValue, typename TSize, typename TCmp>
		void InsertionSort(TValue *array, TSize size, TCmp cmp = std::less<TValue>())
	{
		for (TSize i = 1; i < size; i++) {
			TValue tmp = array[i];
			if (cmp(tmp, array[i - 1])) {
				TSize j = i;
				do {
					array[j] = array[j - 1];
					j--;
				} while (j > 0 && cmp(tmp, array[j - 1]));
				array[j] = tmp;
			}
		}
	}

	template<typename TArray, typename TCmp>
		void InsertionSort(TArray &array, TCmp cmp)
	{
		return InsertionSort(array.data(), array.size(), cmp);
	}

	template<typename TArray>
		void InsertionSort(TArray &array)
	{
		return InsertionSort(array, std::less<TArray::value_type>());
	}

}
