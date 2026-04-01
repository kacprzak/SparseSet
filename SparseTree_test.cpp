#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>

#include "SparseTree.hpp"

namespace
{

TEST( SparseTree, ctor )
{
	sparse::Tree< std::uint8_t, float > tree;

	EXPECT_TRUE( tree.empty() );
}

TEST( SparseTree, iterator )
{
	using namespace std::ranges;

	sparse::Tree< std::uint16_t, float > tree{ { 0, 0.f }, { 1, 1.f }, { 2, 2.f } };

	for( const auto& [ k, v ] : tree )
		v = -v;

	const std::vector< float > expected{ -0.f, -1.f, -2.f };

	EXPECT_TRUE( equal( views::values( tree ), expected ) );
}

TEST( SparseTree, insert )
{
	sparse::Tree< std::uint8_t, float > tree;

	EXPECT_TRUE( tree.insert( 0, 2.f ) );
	EXPECT_FALSE( tree.empty() );
	EXPECT_EQ( tree.at( 0 ), 2.f );
}

TEST( SparseTree, erase )
{
	sparse::Tree< std::uint8_t, float > tree;

	EXPECT_TRUE( tree.insert( 0, 2.f ) );
	EXPECT_TRUE( tree.contains( 0 ) );
	EXPECT_TRUE( tree.erase( 0 ) );
	EXPECT_FALSE( tree.contains( 0 ) );
}

TEST( SparseTree, parent )
{
	sparse::Tree< std::uint8_t, float > tree;

	EXPECT_TRUE( tree.insert( 0, 0.f ) );
	EXPECT_EQ( tree.parent( 0 ), tree.end() );

	EXPECT_TRUE( tree.insert( 1, 1.f, 0 ) );
	EXPECT_EQ( ( *tree.parent( 1 ) ).first, 0u );
	EXPECT_EQ( ( *tree.parent( 1 ) ).second, 0.f );
}

TEST( SparseTree, children )
{
	sparse::Tree< std::uint8_t, float > tree;

	// Root
	EXPECT_TRUE( tree.insert( 0, 0.f ) );
	// Children
	EXPECT_TRUE( tree.insert( 3, 3.f, 0 ) );
	EXPECT_TRUE( tree.insert( 2, 2.f, 0 ) );
	EXPECT_TRUE( tree.insert( 1, 1.f, 0 ) );

	EXPECT_TRUE( tree.insert( 4, 4.f, 2 ) );
	EXPECT_TRUE( tree.insert( 5, 5.f, 2 ) );

	EXPECT_FALSE( tree.insert( 5, -5.f, 2 ) );

	{
		const std::vector< float > expected{ 3.f, 2.f, 1.f };

		std::vector< float > result;
		for( auto it = tree.children_begin( 0 ); it != tree.end(); it = tree.children_next( it ) )
			result.push_back( it->second );

		EXPECT_EQ( result, expected );
	}

	EXPECT_EQ( tree.size(), 6u );
	tree.erase( 2 );
	EXPECT_EQ( tree.size(), 3u );

	{
		const std::vector< float > expected{ 3.f, 1.f };

		std::vector< float > result;
		for( auto it = tree.children_begin( 0 ); it != tree.end(); it = tree.children_next( it ) )
			result.push_back( it->second );

		EXPECT_EQ( result, expected );
	}
}

TEST( SparseTree, for_each_bfs )
{
	sparse::Tree< std::uint8_t, float > tree;

	// Root
	EXPECT_TRUE( tree.insert( 0, 0.f ) );
	// Children
	EXPECT_TRUE( tree.insert( 1, 1.f, 0 ) );
	EXPECT_TRUE( tree.insert( 2, 2.f, 0 ) );
	EXPECT_TRUE( tree.insert( 3, 3.f, 0 ) );

	EXPECT_TRUE( tree.insert( 6, 6.f, 2 ) );

	EXPECT_TRUE( tree.insert( 4, 4.f, 1 ) );
	EXPECT_TRUE( tree.insert( 5, 5.f, 1 ) );
	// Extra root
	EXPECT_TRUE( tree.insert( 7, 7.f ) );

	{
		const std::vector< float > expected{ 0.f, 7.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f };

		std::vector< float > result;
		tree.for_each_bfs( [ & ]( const auto& kv ) { result.push_back( kv.second ); } );

		EXPECT_EQ( result, expected );
	}
}

} // namespace
