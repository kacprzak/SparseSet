#include <gtest/gtest.h>

#include "SparseTree.hpp"

namespace
{

TEST( SparseTree, ctor )
{
	SparseTree< std::uint8_t > tree;

	EXPECT_TRUE( tree.empty() );
}

} // namespace
