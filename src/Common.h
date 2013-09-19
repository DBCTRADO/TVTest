#ifndef TVTEST_COMMON_H
#define TVTEST_COMMON_H


template <typename T> inline T pointer_cast(void *p)
{
	return static_cast<T>(p);
}

template <typename T> inline T pointer_cast(const void *p)
{
	return static_cast<T>(p);
}


#endif
