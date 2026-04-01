#include <gtest/gtest.h>

#include "SparseSet.hpp"

#include <cstdint>

namespace
{

TEST( SparseSet, ctor )
{
	sparse::Set< std::uint8_t > set;

	EXPECT_TRUE( set.empty() );
}

TEST( SparseSet, intializer_list )
{
	sparse::Set< std::uint16_t > set{ 1, 6, 32, 5, 42, 16, 16, 16, 16, 16 };

	EXPECT_FALSE( set.empty() );
	EXPECT_EQ( set.size(), 6u );
	EXPECT_TRUE( set.contains( 42 ) );
}

TEST( SparseSet, clear )
{
	sparse::Set< std::uint32_t > set{ 1 };

	EXPECT_FALSE( set.empty() );
	set.clear();
	EXPECT_TRUE( set.empty() );
}

TEST( SparseSet, insert )
{
	sparse::Set< std::uint8_t > set;

	EXPECT_TRUE( set.insert( 13 ) );
	EXPECT_FALSE( set.insert( 13 ) );
	EXPECT_EQ( set.size(), 1u );
	EXPECT_TRUE( set.contains( 13 ) );

	EXPECT_ANY_THROW( set.insert( 255 ) );
}

TEST( SparseSet, erase )
{
	sparse::Set< std::uint16_t > set{ 0, 1, 2 };

	EXPECT_TRUE( set.erase( 1 ) );
	EXPECT_EQ( set.size(), 2u );
	EXPECT_FALSE( set.erase( 1 ) );
	EXPECT_EQ( set.size(), 2u );

	EXPECT_TRUE( set.contains( 0 ) );
	EXPECT_TRUE( set.contains( 2 ) );
}

TEST( SparseSet, iterator )
{
	sparse::Set< std::uint64_t > set{ 0, 1, 2, 3 };

	int sum = 0;
	for( auto& value : set )
		sum += value;

	EXPECT_EQ( sum, 6 );
}

TEST( SparseSet, const_iterator )
{
	const sparse::Set< std::uint8_t > set{ 0, 1, 2, 3 };

	int sum = 0;
	for( const auto& value : set )
		sum += value;

	EXPECT_EQ( sum, 6 );
}

TEST( SparseSet, sort )
{
	using namespace std::ranges;

	sparse::Set< std::uint8_t > set{ 3, 1, 0, 2, 5 };

	const std::vector< std::uint8_t > presort{ 3, 1, 0, 2, 5 };
	const std::vector< std::uint8_t > postsort{ 0, 1, 2, 3, 5 };

	EXPECT_FALSE( set.contains( 4 ) );
	EXPECT_TRUE( equal( set, presort ) );

	set.sort();

	EXPECT_FALSE( set.contains( 4 ) );
	EXPECT_TRUE( equal( set, postsort ) );

	// Checks if sparse vector is updated.
	set.insert( 3 );
	EXPECT_TRUE( equal( set, postsort ) );
}

TEST( SparseSet, reorder )
{
	using namespace std::ranges;

	sparse::Set< std::uint16_t > set{ 3, 1, 10, 0, 2, 5 };

	const std::vector< std::uint16_t > presort{ 3, 1, 10, 0, 2, 5 };
	const std::vector< std::uint16_t > postsort{ 10, 0, 1, 2, 3, 5 };

	EXPECT_FALSE( set.contains( 4 ) );
	EXPECT_TRUE( equal( set, presort ) );

	set.reorder( postsort );

	EXPECT_FALSE( set.contains( 4 ) );
	EXPECT_TRUE( equal( set, postsort ) );

	// Checks if sparse vector is updated.
	set.insert( 3 );
	EXPECT_TRUE( equal( set, postsort ) );
}

} // namespace
