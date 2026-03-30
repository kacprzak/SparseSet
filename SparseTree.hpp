#pragma once

#include <concepts>

template< typename Key >
    requires std::unsigned_integral< Key >
class SparseTree final
{
public:
	bool empty() const { return true; }
};
