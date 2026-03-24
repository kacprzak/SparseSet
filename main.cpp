#include <gtest/gtest.h>

#include "SparseVector.hpp"

namespace
{

TEST( SparseVector, ctor )
{
	SparseVector< float > vec;

	EXPECT_TRUE( vec.empty() );
}

TEST( SparseVector, intializer_list )
{
	SparseVector< short > vec{ { 1, 2, 3, 4, 5, 6 } };

	EXPECT_FALSE( vec.empty() );
	EXPECT_EQ( vec.at( 4 ), 5 );
}

TEST( SparseVector, emplace )
{
	SparseVector< float > vec;
	EXPECT_EQ( vec.emplace( 0.f ), 0.f );

	EXPECT_FALSE( vec.empty() );
	EXPECT_EQ( vec.size(), 1 );

	EXPECT_EQ( vec.emplace( 0.f ), 0.f );
	EXPECT_EQ( vec.size(), 2 );
	EXPECT_EQ( vec.emplace( 1.f ), 1.f );
	EXPECT_EQ( vec.size(), 3 );
}

TEST( SparseVector, erase )
{
	SparseVector< float > vec;
	vec.emplace( 0.f );
	vec.emplace( 1.f );
	vec.emplace( 2.f );

	EXPECT_TRUE( vec.erase( 1 ) );
	EXPECT_EQ( vec.size(), 2 );
	EXPECT_FALSE( vec.erase( 1 ) );
	EXPECT_EQ( vec.size(), 2 );

	EXPECT_EQ( vec.at( 0 ), 0.f );
	EXPECT_EQ( vec.at( 2 ), 2.f );
}

TEST( SparseVector, reuse )
{
	SparseVector< float > vec;
	vec.emplace( 0.f );
	vec.emplace( 1.f );
	vec.emplace( 2.f );

	EXPECT_EQ( vec.at( 1 ), 1.f );
	EXPECT_TRUE( vec.erase( 1 ) );

	// Should reuse free index
	vec.emplace( 4.f );
	EXPECT_EQ( vec.at( 1 ), 4.f );
}

TEST( SparseVector, iterator )
{
	SparseVector< int > vec;
	vec.emplace( 0 );
	vec.emplace( 1 );
	vec.emplace( 2 );
	vec.emplace( 3 );

	int sum = 0;
	for( const auto& value : vec )
	{
		sum += value;
	}
	EXPECT_EQ( sum, 6 );
}

} // namespace
