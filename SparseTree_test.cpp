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

TEST( SparseTree, initializer_list )
{
	sparse::Tree< std::uint16_t, float > tree{ { 0, 0.f }, { 1, 1.f }, { 2, 2.f } };

	EXPECT_FALSE( tree.empty() );
	EXPECT_EQ( tree.size(), 3u );

	int counter = 0;
	tree.for_each_bfs( [ & ]( const auto& /*kv*/ ) { ++counter; } );
	EXPECT_EQ( counter, 3 );
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

	// 0
	// ├── 3
	// ├── 2
	// │   ├── 5
	// │   └── 4
	// └── 1
	EXPECT_TRUE( tree.insert( 0, 0.f ) );
	EXPECT_TRUE( tree.insert( 3, 3.f, 0 ) );
	EXPECT_TRUE( tree.insert_after( 2, 2.f, 3 ) );
	EXPECT_TRUE( tree.insert_after( 1, 1.f, 2 ) );
	EXPECT_TRUE( tree.insert( 4, 4.f, 2 ) );
	EXPECT_TRUE( tree.insert( 5, 5.f, 2 ) );
	EXPECT_FALSE( tree.insert( 5, -5.f, 2 ) ); // duplicate key

	{
		const std::vector< float > expected{ 3.f, 2.f, 1.f };

		std::vector< float > result;
		for( auto it = tree.children_begin( 0 ); it != tree.end(); it = tree.children_next( it ) )
			result.push_back( it->second );

		EXPECT_EQ( result, expected );
	}

	EXPECT_EQ( tree.size(), 6u );
	tree.erase( 2 ); // erases node 2 and its subtree (4, 5)
	EXPECT_EQ( tree.size(), 3u );

	{
		const std::vector< float > expected{ 3.f, 1.f };

		std::vector< float > result;
		for( auto it = tree.children_begin( 0 ); it != tree.end(); it = tree.children_next( it ) )
			result.push_back( it->second );

		EXPECT_EQ( result, expected );
	}
}

namespace
{
void init( sparse::Tree< std::uint8_t, float >& tree )
{
	// Forest (root-list order: 7, then 0):
	// 7  (leaf)
	// 0
	// ├── 1
	// │   ├── 4
	// │   └── 5
	// ├── 2
	// │   └── 6
	// └── 3
	EXPECT_TRUE( tree.insert( 0, 0.f ) );
	EXPECT_TRUE( tree.insert( 1, 1.f, 0 ) );
	EXPECT_TRUE( tree.insert_after( 2, 2.f, 1 ) );
	EXPECT_TRUE( tree.insert_after( 3, 3.f, 2 ) );
	EXPECT_TRUE( tree.insert( 6, 6.f, 2 ) );
	EXPECT_TRUE( tree.insert( 4, 4.f, 1 ) );
	EXPECT_TRUE( tree.insert_after( 5, 5.f, 4 ) );
	EXPECT_TRUE( tree.insert( 7, 7.f ) ); // prepended → becomes first root
}
} // namespace

TEST( SparseTree, for_each_bfs )
{
	sparse::Tree< std::uint8_t, float > tree;
	init( tree );

	// Roots first (7, 0), then each level left-to-right.
	const std::vector< float > expected{ 7.f, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f };

	std::vector< float > result;
	tree.for_each_bfs( [ & ]( const auto& kv ) { result.push_back( kv.second ); } );

	EXPECT_EQ( result, expected );
}

TEST( SparseTree, for_each_dfs )
{
	sparse::Tree< std::uint8_t, float > tree;
	init( tree );

	// Pre-order: each node visited before its children, roots in root-list order.
	const std::vector< float > expected{ 7.f, 0.f, 1.f, 4.f, 5.f, 2.f, 6.f, 3.f };

	std::vector< float > result;
	tree.for_each_dfs(
	    [ & ]( const auto& kv )
	    {
		    result.push_back( kv.second );
		    return true;
	    },
	    []( const auto& ) {} );

	EXPECT_EQ( result, expected );
}

TEST( SparseTree, for_each_bfs_from )
{
	sparse::Tree< std::uint8_t, float > tree;
	init( tree );

	// Subtree rooted at node 0
	{
		const std::vector< float > expected{ 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f };

		std::vector< float > result;
		tree.for_each_bfs( 0, [ & ]( const auto& kv ) { result.push_back( kv.second ); } );

		EXPECT_EQ( result, expected );
	}

	// Subtree rooted at node 1
	{
		const std::vector< float > expected{ 1.f, 4.f, 5.f };

		std::vector< float > result;
		tree.for_each_bfs( 1, [ & ]( const auto& kv ) { result.push_back( kv.second ); } );

		EXPECT_EQ( result, expected );
	}

	// Node 7 (leaf)
	{
		const std::vector< float > expected{ 7.f };

		std::vector< float > result;
		tree.for_each_bfs( 7, [ & ]( const auto& kv ) { result.push_back( kv.second ); } );

		EXPECT_EQ( result, expected );
	}
}

TEST( SparseTree, for_each_dfs_from )
{
	sparse::Tree< std::uint8_t, float > tree;
	init( tree );

	// Subtree rooted at node 0
	{
		const std::vector< float > expected{ 0.f, 1.f, 4.f, 5.f, 2.f, 6.f, 3.f };

		std::vector< float > result;
		tree.for_each_dfs(
		    0,
		    [ & ]( const auto& kv )
		    {
			    result.push_back( kv.second );
			    return true;
		    },
		    []( const auto& ) {} );

		EXPECT_EQ( result, expected );
	}

	// Subtree rooted at node 2
	{
		const std::vector< float > expected{ 2.f, 6.f };

		std::vector< float > result;
		tree.for_each_dfs(
		    2,
		    [ & ]( const auto& kv )
		    {
			    result.push_back( kv.second );
			    return true;
		    },
		    []( const auto& ) {} );

		EXPECT_EQ( result, expected );
	}

	// on_enter returning false skips the subtree; on_leave still fires for that node.
	{
		std::vector< float > entered;
		std::vector< float > left;

		tree.for_each_dfs(
		    0,
		    [ & ]( const auto& kv )
		    {
			    entered.push_back( kv.second );
			    return kv.second == 0.f; // descend only into node 0
		    },
		    [ & ]( const auto& kv ) { left.push_back( kv.second ); } );

		const std::vector< float > expected_entered{ 0.f, 1.f, 2.f, 3.f };
		const std::vector< float > expected_left{ 1.f, 2.f, 3.f, 0.f }; // children leave before parent
		EXPECT_EQ( entered, expected_entered );
		EXPECT_EQ( left, expected_left );
	}
}

TEST( SparseTree, sort_bfs )
{
	using namespace std::ranges;

	sparse::Tree< std::uint8_t, float > tree;
	init( tree );

	{
		const std::vector< float > expected{ 7.f, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f };

		tree.sort_bfs();

		EXPECT_TRUE( equal( views::values( tree ), expected ) );
	}
}

} // namespace
