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
	SparseVector< short > vec{ 1, 2, 3, 4, 5, 6 };

	EXPECT_FALSE( vec.empty() );
	EXPECT_EQ( vec.at( 4 ), 5 );
}

TEST( SparseVector, clear )
{
	SparseVector< int > vec{ 1 };

	EXPECT_FALSE( vec.empty() );
	vec.clear();
	EXPECT_TRUE( vec.empty() );
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

TEST( SparseVector, insert )
{
	SparseVector< float > vec;

	EXPECT_TRUE( vec.insert( 13, 13.f ) );
	EXPECT_FALSE( vec.insert( 13, -1.f ) );
	EXPECT_EQ( vec.size(), 1 );
	EXPECT_EQ( vec.at( 13 ), 13.f );
}

TEST( SparseVector, insert_or_assign )
{
	SparseVector< float > vec;

	EXPECT_TRUE( vec.insert_or_assign( 13, 13.f ) );
	EXPECT_FALSE( vec.insert_or_assign( 13, -1.f ) );
	EXPECT_EQ( vec.size(), 1 );
	EXPECT_EQ( vec.at( 13 ), -1.f );
}

TEST( SparseVector, erase )
{
	SparseVector< float > vec{ 0.f, 1.f, 2.f };

	EXPECT_TRUE( vec.erase( 1 ) );
	EXPECT_EQ( vec.size(), 2 );
	EXPECT_FALSE( vec.erase( 1 ) );
	EXPECT_EQ( vec.size(), 2 );

	EXPECT_EQ( vec.at( 0 ), 0.f );
	EXPECT_EQ( vec.at( 2 ), 2.f );
}

TEST( SparseVector, reuse )
{
	SparseVector< float > vec{ 0.f, 1.f, 2.f };

	// Erase second element
	EXPECT_EQ( vec.at( 1 ), 1.f );
	EXPECT_TRUE( vec.erase( 1 ) );

	// Should reuse index 1
	vec.emplace( 4.f );
	EXPECT_EQ( vec.at( 1 ), 4.f );

	// Erase first element
	EXPECT_TRUE( vec.erase( 0 ) );
	EXPECT_EQ( vec.at( 1 ), 4.f );

	// Should reuse index 0
	EXPECT_EQ( vec.emplace( 2.f ), 2.f );
	EXPECT_EQ( vec.at( 0 ), 2.f );
	EXPECT_EQ( vec.at( 1 ), 4.f );
}

TEST( SparseVector, iterator )
{
	SparseVector< int > vec{ 0, 1, 2, 3 };

	int sum = 0;
	for( const auto& value : vec )
	{
		sum += value;
	}
	EXPECT_EQ( sum, 6 );
}

} // namespace
