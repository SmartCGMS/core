#pragma once

#include <fstream>

// discards Byte Order Mark from UTF-8, UTF-16 and UTF-32x encoded documents; leaves stream in state,
// where further text-only reading is possible
void Discard_BOM_If_Present(std::istream& is);

// determines, whether the element is present in supplied container; the element must have "==" operator defined,
// and the container should contain range-based for loops
template<class C, typename T>
inline bool Contains_Element(C const& container, T const& element)
{
	for (auto& el : container)
	{
		if (el == element)
			return true;
	}
	return false;
}
