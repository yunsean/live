#pragma once

template <typename T, typename M>
const T& ReturnAs(const T& retValue, const M&) {
	return retValue;
}

template <typename M>
void ReturnAs(const M&) {
}

#define rvaa(ret, any)	(any, ret)
#define raav(void, any)	ReturnAs(any)
#define raaa(ret, any)	ReturnAs(ret, any)

