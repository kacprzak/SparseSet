#include <gtest/gtest.h>

#include "SparseMap.hpp"

#include <cstdint>
#include <limits>

namespace
{

TEST( SparseMap, ctor )
{
	SparseMap< std::uint8_t, float > vec;

	EXPECT_TRUE( vec.empty() );
}

TEST( SparseMap, intializer_list )
{
	SparseMap< std::uint16_t, float > vec{ { 1, 1.f }, { 42, 42.f }, { 16, 16.f }, { 16, 1234.f } };

	EXPECT_FALSE( vec.empty() );
	EXPECT_EQ( vec.size(), 3u );
	EXPECT_EQ( vec.at( 16 ), 16.f );
}

TEST( SparseMap, clear )
{
	SparseMap< std::uint32_t, float > vec{ { 1, {} } };

	EXPECT_FALSE( vec.empty() );
	vec.clear();
	EXPECT_TRUE( vec.empty() );
}

TEST( SparseMap, insert )
{
	SparseMap< std::uint8_t, float > vec;

	EXPECT_TRUE( vec.insert( 13, 7.f ) );
	EXPECT_FALSE( vec.insert( 13, 13.f ) );
	EXPECT_EQ( vec.size(), 1u );
	EXPECT_EQ( vec.at( 13 ), 7.f );

	EXPECT_ANY_THROW( vec.insert( 255, {} ) );
}

TEST( SparseMap, erase )
{
	SparseMap< std::uint16_t, float > vec{ { 0, 0.f }, { 1, 1.f }, { 2, 2.f } };

	EXPECT_TRUE( vec.erase( 1 ) );
	EXPECT_EQ( vec.size(), 2u );
	EXPECT_FALSE( vec.erase( 1 ) );
	EXPECT_EQ( vec.size(), 2u );

	EXPECT_TRUE( vec.contains( 0 ) );
	EXPECT_EQ( vec.at( 2 ), 2.f );
}

TEST( SparseMap, iterator )
{
	SparseMap< std::uint16_t, float > vec{ { 0, 0.f }, { 1, 1.f }, { 2, 2.f } };

	int sum = 0;
	for( const auto& [ k, v ] : vec )
		sum += k;

	EXPECT_EQ( sum, 3 );
}

TEST( SparseMap, const_iterator )
{
	const SparseMap< std::uint16_t, float > vec{ { 0, 0.f }, { 1, 1.f }, { 2, 2.f } };

	int sum = 0;
	for( const auto& [ k, v ] : vec )
		sum += k;

	EXPECT_EQ( sum, 3 );
}

/*
TEST( SparseMap, sort )
{
    SparseMap< std::uint8_t, float > vec{ { 3, {} }, { 1, {} }, { 0, {} }, { 2, {} }, { 5, {} } };

    const std::vector< std::uint8_t > presort{ 3, 1, 0, 2, 5 };
    const std::vector< std::uint8_t > postsort{ 0, 1, 2, 3, 5 };

    EXPECT_FALSE( vec.contains( 4 ) );
    EXPECT_TRUE( std::equal( vec.begin(), vec.end(), presort.begin(), presort.end() ) );

    auto actualIt   = vec.begin();
    auto expectedIt = presort.begin();
    for( ; actualIt != vec.end() and expectedIt != presort.end(); ++actualIt, ++expectedIt )
    {
        EXPECT_EQ( actualIt->first, *expectedIt );
    }

    vec.sort();

    EXPECT_FALSE( vec.contains( 4 ) );
    EXPECT_TRUE( std::equal( vec.begin(), vec.end(), postsort.begin(), postsort.end() ) );

    // Checks if sparse vector is updated.
    vec.insert( 3, {} );
    EXPECT_TRUE( std::equal( vec.begin(), vec.end(), postsort.begin(), postsort.end() ) );
}
*/

TEST( SparseMap, at )
{
	SparseMap< std::uint8_t, float > vec;

	vec.insert( 42, 13.f );
	vec.at( 42 ) = 21.f;
	EXPECT_EQ( vec.at( 42 ), 21.f );
}

TEST( SparseMap, find_slot )
{
	SparseMap< std::uint8_t, float > vec;

	for( int i = 0; i < std::numeric_limits< std::uint8_t >::max(); ++i )
		vec.insert( i, i );

	// Erase second element
	EXPECT_EQ( vec.at( 1 ), 1.f );
	EXPECT_TRUE( vec.erase( 1 ) );

	// Should reuse index 1
	EXPECT_EQ( vec.find_slot(), 1 );
	vec.insert( vec.find_slot(), 4.f );
	EXPECT_EQ( vec.at( 1 ), 4.f );

	// Erase first element
	EXPECT_TRUE( vec.erase( 0 ) );
	EXPECT_EQ( vec.at( 1 ), 4.f );

	// Should reuse index 0
	EXPECT_EQ( vec.find_slot(), 0 );
	vec.insert( vec.find_slot(), 2.f );
	EXPECT_EQ( vec.at( 0 ), 2.f );
	EXPECT_EQ( vec.at( 1 ), 4.f );
}

} // namespace
