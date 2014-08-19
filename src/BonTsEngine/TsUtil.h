#pragma once


#include <functional>
#include <vector>


namespace TsEngine
{

	// ë}ì¸É\Å[Ég
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

	template<typename TValue, typename TCmp>
		void InsertionSort(std::vector<TValue> &array, TCmp cmp = std::less<TValue>())
	{
		return InsertionSort(array.data(), array.size(), cmp);
	}

}
