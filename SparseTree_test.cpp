#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>

#include "SparseTree.hpp"

namespace
{

TEST( SparseTree, ctor )
{
	SparseTree< std::uint8_t, float > tree;

	EXPECT_TRUE( tree.empty() );
}

TEST( SparseTree, iterator )
{
	using namespace std::ranges;

	SparseTree< std::uint16_t, float > tree{ { 0, 0.f }, { 1, 1.f }, { 2, 2.f } };

	for( const auto& [ k, v ] : tree )
		v = -v;

	const std::vector< float > expected{ -0.f, -1.f, -2.f };

	EXPECT_TRUE( equal( views::values( tree ), expected ) );
}

TEST( SparseTree, insert )
{
	SparseTree< std::uint8_t, float > tree;

	EXPECT_TRUE( tree.insert( 0, 2.f ) );
	EXPECT_FALSE( tree.empty() );
	EXPECT_EQ( tree.at( 0 ), 2.f );
}

TEST( SparseTree, parent )
{
	SparseTree< std::uint8_t, float > tree;

	EXPECT_TRUE( tree.insert( 0, 0.f ) );
	EXPECT_TRUE( tree.insert( 1, 1.f, 0 ) );
	EXPECT_EQ( ( *tree.parent( 1 ) ).first, 0u );
	EXPECT_EQ( ( *tree.parent( 1 ) ).second, 0.f );
}

} // namespace
