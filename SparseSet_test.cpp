#include <gtest/gtest.h>

#include "SparseSet.hpp"

#include <cstdint>

namespace
{

TEST( SparseSet, ctor )
{
	SparseSet< std::uint8_t > vec;

	EXPECT_TRUE( vec.empty() );
}

TEST( SparseSet, intializer_list )
{
	SparseSet< std::uint16_t > vec{ 1, 6, 32, 5, 42, 16, 16, 16, 16, 16 };

	EXPECT_FALSE( vec.empty() );
	EXPECT_EQ( vec.size(), 6u );
	EXPECT_TRUE( vec.contains( 42 ) );
}

TEST( SparseSet, clear )
{
	SparseSet< std::uint32_t > vec{ 1 };

	EXPECT_FALSE( vec.empty() );
	vec.clear();
	EXPECT_TRUE( vec.empty() );
}

TEST( SparseSet, insert )
{
	SparseSet< std::uint8_t > vec;

	EXPECT_TRUE( vec.insert( 13 ) );
	EXPECT_FALSE( vec.insert( 13 ) );
	EXPECT_EQ( vec.size(), 1u );
	EXPECT_TRUE( vec.contains( 13 ) );

	EXPECT_ANY_THROW( vec.insert( 255 ) );
}

TEST( SparseSet, erase )
{
	SparseSet< std::uint16_t > vec{ 0, 1, 2 };

	EXPECT_TRUE( vec.erase( 1 ) );
	EXPECT_EQ( vec.size(), 2u );
	EXPECT_FALSE( vec.erase( 1 ) );
	EXPECT_EQ( vec.size(), 2u );

	EXPECT_TRUE( vec.contains( 0 ) );
	EXPECT_TRUE( vec.contains( 2 ) );
}

TEST( SparseSet, iterator )
{
	SparseSet< std::uint64_t > vec{ 0, 1, 2, 3 };

	int sum = 0;
	for( const auto& value : vec )
		sum += value;

	EXPECT_EQ( sum, 6 );
}

TEST( SparseSet, const_iterator )
{
	const SparseSet< std::uint8_t > vec{ 0, 1, 2, 3 };

	int sum = 0;
	for( const auto& value : vec )
		sum += value;

	EXPECT_EQ( sum, 6 );
}

TEST( SparseSet, sort )
{
	SparseSet< std::uint8_t > vec{ 3, 1, 0, 2, 5 };

	const std::vector< std::uint8_t > presort{ 3, 1, 0, 2, 5 };
	const std::vector< std::uint8_t > postsort{ 0, 1, 2, 3, 5 };

	EXPECT_FALSE( vec.contains( 4 ) );
	EXPECT_TRUE( std::equal( vec.begin(), vec.end(), presort.begin(), presort.end() ) );

	vec.sort();

	EXPECT_FALSE( vec.contains( 4 ) );
	EXPECT_TRUE( std::equal( vec.begin(), vec.end(), postsort.begin(), postsort.end() ) );

	// Checks if sparse vector is updated.
	vec.insert( 3 );
	EXPECT_TRUE( std::equal( vec.begin(), vec.end(), postsort.begin(), postsort.end() ) );
}

} // namespace
