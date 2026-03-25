#include <cstdint>
#include <gtest/gtest.h>
#include <utility>

#include "SparseVector.hpp"

namespace
{

TEST( SparseVector, ctor )
{
	SparseVector< std::uint8_t > vec;

	EXPECT_TRUE( vec.empty() );
}

TEST( SparseVector, intializer_list )
{
	SparseVector< std::uint16_t > vec{ 1, 6, 32, 5, 42, 16 };

	EXPECT_FALSE( vec.empty() );
	EXPECT_EQ( vec.size(), 6 );
	EXPECT_EQ( vec.at( 42 ), 42 );
}

TEST( SparseVector, clear )
{
	SparseVector< std::uint32_t > vec{ 1 };

	EXPECT_FALSE( vec.empty() );
	vec.clear();
	EXPECT_TRUE( vec.empty() );
}

/*
TEST( SparseVector, emplace )
{
    SparseVector< std::pair< std::uint8_t, float > > vec;
    EXPECT_EQ( vec.emplace( 0.f ), 0 );

    EXPECT_FALSE( vec.empty() );
    EXPECT_EQ( vec.size(), 1 );

    EXPECT_EQ( vec.emplace( 0.f ), 1 );
    EXPECT_EQ( vec.size(), 2 );
    EXPECT_EQ( vec.emplace( 1.f ), 2 );
    EXPECT_EQ( vec.size(), 3 );
}
*/

TEST( SparseVector, insert )
{
	SparseVector< std::pair< std::uint8_t, float > > vec;

	EXPECT_TRUE( vec.insert( { 13, 13.f } ) );
	EXPECT_FALSE( vec.insert( { 13, -1.f } ) );
	EXPECT_EQ( vec.size(), 1 );
	EXPECT_EQ( vec.at( 13 ).second, 13.f );
}

TEST( SparseVector, insert_or_assign )
{
	SparseVector< std::pair< std::uint64_t, float > > vec;

	EXPECT_TRUE( vec.insert_or_assign( { 13, 13.f } ) );
	EXPECT_FALSE( vec.insert_or_assign( { 13, -1.f } ) );
	EXPECT_EQ( vec.size(), 1 );
	EXPECT_EQ( vec.at( 13 ).second, -1.f );
}

TEST( SparseVector, erase )
{
	SparseVector< std::pair< std::uint16_t, float > > vec{ { 0, 0.f }, { 1, 1.f }, { 2, 2.f } };

	EXPECT_TRUE( vec.erase( 1 ) );
	EXPECT_EQ( vec.size(), 2 );
	EXPECT_FALSE( vec.erase( 1 ) );
	EXPECT_EQ( vec.size(), 2 );

	EXPECT_EQ( vec.at( 0 ).second, 0.f );
	EXPECT_EQ( vec.at( 2 ).second, 2.f );
}

/*
TEST( SparseVector, reuse )
{
    SparseVector< std::pair< std::uint16_t, float > > vec{ 0.f, 1.f, 2.f };

    // Erase second element
    EXPECT_EQ( vec.at( 1 ).second, 1.f );
    EXPECT_TRUE( vec.erase( 1 ) );

    // Should reuse index 1
    vec.emplace( 4.f );
    EXPECT_EQ( vec.at( 1 ).second, 4.f );

    // Erase first element
    EXPECT_TRUE( vec.erase( 0 ) );
    EXPECT_EQ( vec.at( 1 ).second, 4.f );

    // Should reuse index 0
    EXPECT_EQ( vec.emplace( 2.f ), 0 );
    EXPECT_EQ( vec.at( 0 ).second, 2.f );
    EXPECT_EQ( vec.at( 1 ).second, 4.f );
}
*/

TEST( SparseVector, iterator )
{
	SparseVector< std::uint64_t > vec{ 0, 1, 2, 3 };

	int sum = 0;
	for( const auto& value : vec )
		sum += value;

	EXPECT_EQ( sum, 6 );
}

TEST( SparseVector, const_iterator )
{
	const SparseVector< std::uint8_t > vec{ 0, 1, 2, 3 };

	int sum = 0;
	for( const auto& value : vec )
		sum += value;

	EXPECT_EQ( sum, 6 );
}

} // namespace
