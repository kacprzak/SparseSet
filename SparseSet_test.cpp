#include <gtest/gtest.h>

#include "SparseSet.hpp"

#include <cstdint>

namespace
{

TEST( SparseSet, ctor )
{
	SparseSet< std::uint8_t > set;

	EXPECT_TRUE( set.empty() );
}

TEST( SparseSet, intializer_list )
{
	SparseSet< std::uint16_t > set{ 1, 6, 32, 5, 42, 16, 16, 16, 16, 16 };

	EXPECT_FALSE( set.empty() );
	EXPECT_EQ( set.size(), 6u );
	EXPECT_TRUE( set.contains( 42 ) );
}

TEST( SparseSet, clear )
{
	SparseSet< std::uint32_t > set{ 1 };

	EXPECT_FALSE( set.empty() );
	set.clear();
	EXPECT_TRUE( set.empty() );
}

TEST( SparseSet, insert )
{
	SparseSet< std::uint8_t > set;

	EXPECT_TRUE( set.insert( 13 ) );
	EXPECT_FALSE( set.insert( 13 ) );
	EXPECT_EQ( set.size(), 1u );
	EXPECT_TRUE( set.contains( 13 ) );

	EXPECT_ANY_THROW( set.insert( 255 ) );
}

TEST( SparseSet, erase )
{
	SparseSet< std::uint16_t > set{ 0, 1, 2 };

	EXPECT_TRUE( set.erase( 1 ) );
	EXPECT_EQ( set.size(), 2u );
	EXPECT_FALSE( set.erase( 1 ) );
	EXPECT_EQ( set.size(), 2u );

	EXPECT_TRUE( set.contains( 0 ) );
	EXPECT_TRUE( set.contains( 2 ) );
}

TEST( SparseSet, iterator )
{
	SparseSet< std::uint64_t > set{ 0, 1, 2, 3 };

	int sum = 0;
	for( const auto& value : set )
		sum += value;

	EXPECT_EQ( sum, 6 );
}

TEST( SparseSet, const_iterator )
{
	const SparseSet< std::uint8_t > set{ 0, 1, 2, 3 };

	int sum = 0;
	for( const auto& value : set )
		sum += value;

	EXPECT_EQ( sum, 6 );
}

TEST( SparseSet, sort )
{
	SparseSet< std::uint8_t > set{ 3, 1, 0, 2, 5 };

	const std::vector< std::uint8_t > presort{ 3, 1, 0, 2, 5 };
	const std::vector< std::uint8_t > postsort{ 0, 1, 2, 3, 5 };

	EXPECT_FALSE( set.contains( 4 ) );
	EXPECT_TRUE( std::ranges::equal( set, presort ) );

	set.sort();

	EXPECT_FALSE( set.contains( 4 ) );
	EXPECT_TRUE( std::ranges::equal( set, postsort ) );

	// Checks if sparse vector is updated.
	set.insert( 3 );
	EXPECT_TRUE( std::ranges::equal( set, postsort ) );
}

} // namespace
