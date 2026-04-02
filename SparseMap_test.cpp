#include <gtest/gtest.h>

#include "SparseMap.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <ranges>

namespace
{

TEST( SparseMap, ctor )
{
	sparse::Map< std::uint8_t, float > map;

	EXPECT_TRUE( map.empty() );
}

TEST( SparseMap, intializer_list )
{
	sparse::Map< std::uint16_t, float > map{ { 1, 1.f }, { 42, 42.f }, { 16, 16.f }, { 16, 1234.f } };

	EXPECT_FALSE( map.empty() );
	EXPECT_EQ( map.size(), 3u );
	EXPECT_EQ( map.at( 16 ), 16.f );
}

TEST( SparseMap, clear )
{
	sparse::Map< std::uint32_t, float > map{ { 1, {} } };

	EXPECT_FALSE( map.empty() );
	map.clear();
	EXPECT_TRUE( map.empty() );
}

TEST( SparseMap, insert )
{
	sparse::Map< std::uint8_t, float > map;

	EXPECT_TRUE( map.insert( 13, 7.f ) );
	EXPECT_FALSE( map.insert( 13, 13.f ) );
	EXPECT_EQ( map.size(), 1u );
	EXPECT_EQ( map.at( 13 ), 7.f );

	EXPECT_ANY_THROW( map.insert( 255, {} ) );
}

TEST( SparseMap, erase )
{
	sparse::Map< std::uint16_t, float > map{ { 0, 0.f }, { 1, 1.f }, { 2, 2.f } };

	EXPECT_TRUE( map.erase( 1 ) );
	EXPECT_EQ( map.size(), 2u );
	EXPECT_FALSE( map.erase( 1 ) );
	EXPECT_EQ( map.size(), 2u );

	EXPECT_TRUE( map.contains( 0 ) );
	EXPECT_EQ( map.at( 2 ), 2.f );
}

TEST( SparseMap, iterator )
{
	using namespace std::ranges;

	sparse::Map< std::uint16_t, float > map{ { 0, 0.f }, { 1, 1.f }, { 2, 2.f } };

	for( const auto& [ k, v ] : map )
		v = -v;

	const std::vector< float > expected{ -0.f, -1.f, -2.f };

	EXPECT_TRUE( equal( views::values( map ), expected ) );
}

TEST( SparseMap, const_iterator )
{
	const sparse::Map< std::uint16_t, float > map{ { 0, 0.f }, { 1, 1.f }, { 2, 2.f } };

	int sum = 0;
	for( const auto& [ k, v ] : map )
		sum += k;

	EXPECT_EQ( sum, 3 );
}

TEST( SparseMap, sort )
{
	using namespace std::ranges;

	sparse::Map< std::uint8_t, float > map{ { 3, {} }, { 1, {} }, { 0, {} }, { 2, {} }, { 5, {} } };

	const std::vector< std::uint8_t > presort{ 3, 1, 0, 2, 5 };
	const std::vector< std::uint8_t > postsort{ 0, 1, 2, 3, 5 };

	EXPECT_TRUE( equal( views::keys( map ), presort ) );
	map.sort();
	EXPECT_TRUE( equal( views::keys( map ), postsort ) );
}

TEST( SparseMap, at )
{
	sparse::Map< std::uint8_t, float > map;

	map.insert( 42, 13.f );
	map.at( 42 ) = 21.f;
	EXPECT_EQ( map.at( 42 ), 21.f );
}

TEST( SparseMap, find_slot )
{
	sparse::Map< std::uint8_t, float > map;

	for( int i = 0; i < std::numeric_limits< std::uint8_t >::max(); ++i )
		map.insert( i, i );

	// Erase second element
	EXPECT_EQ( map.at( 1 ), 1.f );
	EXPECT_TRUE( map.erase( 1 ) );

	// Should reuse index 1
	EXPECT_EQ( map.find_slot(), 1 );
	map.insert( map.find_slot(), 4.f );
	EXPECT_EQ( map.at( 1 ), 4.f );

	// Erase first element
	EXPECT_TRUE( map.erase( 0 ) );
	EXPECT_EQ( map.at( 1 ), 4.f );

	// Should reuse index 0
	EXPECT_EQ( map.find_slot(), 0 );
	map.insert( map.find_slot(), 2.f );
	EXPECT_EQ( map.at( 0 ), 2.f );
	EXPECT_EQ( map.at( 1 ), 4.f );
}

} // namespace
