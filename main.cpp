#include <cstdint>
#include <gtest/gtest.h>
#include <limits>

#include "SparseSet.hpp"

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
	EXPECT_EQ( vec.size(), 6 );
	EXPECT_EQ( vec.at( 42 ), 42 );
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
	SparseMap< std::uint8_t, float > vec;

	EXPECT_TRUE( vec.insert( { 13, 13.f } ) );
	EXPECT_FALSE( vec.insert( { 13, -1.f } ) );
	EXPECT_EQ( vec.size(), 1 );
	EXPECT_EQ( vec.at( 13 ).second, 13.f );

	EXPECT_ANY_THROW( vec.insert( { 255, 1.f } ) );
}

TEST( SparseSet, insert_or_assign )
{
	SparseMap< std::uint8_t, float > vec;

	EXPECT_TRUE( vec.insert_or_assign( { 13, 13.f } ) );
	EXPECT_FALSE( vec.insert_or_assign( { 13, -1.f } ) );
	EXPECT_EQ( vec.size(), 1 );
	EXPECT_EQ( vec.at( 13 ).second, -1.f );

	EXPECT_ANY_THROW( vec.insert_or_assign( { 255, 1.f } ) );
}

TEST( SparseSet, erase )
{
	SparseMap< std::uint16_t, float > vec{ { 0, 0.f }, { 1, 1.f }, { 2, 2.f } };

	EXPECT_TRUE( vec.erase( 1 ) );
	EXPECT_EQ( vec.size(), 2 );
	EXPECT_FALSE( vec.erase( 1 ) );
	EXPECT_EQ( vec.size(), 2 );

	EXPECT_EQ( vec.at( 0 ).second, 0.f );
	EXPECT_EQ( vec.at( 2 ).second, 2.f );
}

TEST( SparseSet, find_slot )
{
	SparseMap< std::uint8_t, float > vec;

	for( int i = 0; i < std::numeric_limits< std::uint8_t >::max(); ++i )
		vec.insert( { i, i } );

	// Erase second element
	EXPECT_EQ( vec.at( 1 ).second, 1.f );
	EXPECT_TRUE( vec.erase( 1 ) );

	// Should reuse index 1
	EXPECT_EQ( vec.find_slot(), 1 );
	vec.insert( { vec.find_slot(), 4.f } );
	EXPECT_EQ( vec.at( 1 ).second, 4.f );

	// Erase first element
	EXPECT_TRUE( vec.erase( 0 ) );
	EXPECT_EQ( vec.at( 1 ).second, 4.f );

	// Should reuse index 0
	EXPECT_EQ( vec.find_slot(), 0 );
	vec.insert( { vec.find_slot(), 2.f } );
	EXPECT_EQ( vec.at( 0 ).second, 2.f );
	EXPECT_EQ( vec.at( 1 ).second, 4.f );
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
	vec.insert_or_assign( 3 );
	EXPECT_TRUE( std::equal( vec.begin(), vec.end(), postsort.begin(), postsort.end() ) );
}

} // namespace
