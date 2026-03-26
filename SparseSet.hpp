#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

/**
 * Returns true if T is pair with Key as first element.
 */
template< typename Key, typename T >
concept key_value_pair
    = requires( T pair ) { requires std::same_as< Key, std::remove_cvref_t< decltype( pair.first ) > >; };

/**
 * Returns true if T is same as Key or has Key as first element of pair.
 */
template< typename Key, typename T >
concept key_convertible = requires {
	requires std::unsigned_integral< Key >;
	requires std::same_as< Key, T > || key_value_pair< Key, T >;
};

/**
 * Container with elements stored contiguously in memory, but indicies remain the same after insertion and erasing.
 */
template< typename Key, typename T = Key >
    requires key_convertible< Key, T >
class SparseSet final
{
public:
	using key_type        = Key;
	using value_type      = T;
	using size_type       = std::size_t;
	using reference       = value_type&;
	using const_reference = const value_type&;

private:
	// Marks unused keys.
	static constexpr auto s_tombstone = std::numeric_limits< key_type >::max();

	[[nodiscard]]
	static auto to_key( const T& value ) -> key_type
	{
		if constexpr( key_value_pair< Key, T > )
			return std::get< 0 >( value );
		else
			return value;
	}

	void swap( const key_type first, const key_type second )
	{
		if( first == second )
			return;

		std::swap( m_dense[ m_sparse[ first ] ], m_dense[ m_sparse[ second ] ] );
		std::swap( m_sparse[ first ], m_sparse[ second ] );
	}

	void do_insert( const value_type& value )
	{
		const auto key = to_key( value );

		if( key == s_tombstone )
			throw std::logic_error{ "Invalid key value." };

		if( key >= m_sparse.size() )
			m_sparse.resize( key + 1u, s_tombstone );

		m_sparse[ key ] = m_dense.size();
		m_dense.emplace_back( value );
	}

public:
	SparseSet() = default;

	SparseSet( std::initializer_list< T > init )
	{
		m_dense.reserve( init.size() );
		m_sparse.reserve( init.size() );

		for( const auto& item : init )
			insert( item );
	}

	[[nodiscard]]
	constexpr bool empty() const noexcept
	{
		return m_dense.empty();
	}

	[[nodiscard]]
	constexpr size_type size() const noexcept
	{
		return m_dense.size();
	}

	[[nodiscard]]
	constexpr bool contains( const key_type& key ) const noexcept
	{
		return key < m_sparse.size() and m_sparse[ key ] != s_tombstone;
	}

	constexpr void clear() noexcept
	{
		m_dense.clear();
		m_sparse.clear();
	}

	/**
	 * Returns smallest key without value.
	 */
	[[nodiscard]]
	constexpr key_type find_slot() const
	{
		const auto it = std::find( m_sparse.begin(), m_sparse.end(), s_tombstone );

		return std::distance( m_sparse.begin(), it );
	}

	/**
	 * Returns true if insertion took place and false if there is already value under index.
	 */
	bool insert( const value_type& value )
	{
		const auto key = to_key( value );

		if( contains( key ) )
			return false;

		do_insert( value );
		return true;
	}

	/**
	 * Returns true if insertion took place and false if the assignment took place.
	 */
	bool insert_or_assign( const value_type& value )
	{
		const auto key = to_key( value );

		if( contains( key ) )
		{
			m_dense[ m_sparse[ key ] ] = value;
			return false;
		}

		do_insert( value );
		return true;
	}

	auto erase( const key_type& key ) -> bool
	{
		if( not contains( key ) )
			return false;

		// Swap and pop
		swap( key, to_key( m_dense.back() ) );

		m_dense.pop_back();
		m_sparse[ key ] = s_tombstone;

		return true;
	}

	void sort()
	{
		std::sort(
		    m_dense.begin(), m_dense.end(), []( const auto& a, const auto& b ) { return to_key( a ) < to_key( b ); } );

		std::fill( m_sparse.begin(), m_sparse.end(), s_tombstone );

		for( auto it = m_dense.begin(); it != m_dense.end(); ++it )
			m_sparse[ to_key( *it ) ] = std::distance( m_dense.begin(), it );
	}

	[[nodiscard]]
	auto at( const key_type& key )
	    requires key_value_pair< Key, T >
	{
		return std::get< 1 >( m_dense.at( m_sparse.at( key ) ) );
	}

	[[nodiscard]]
	auto at( const key_type& key ) const
	    requires key_value_pair< Key, T >
	{
		return std::get< 1 >( m_dense.at( m_sparse.at( key ) ) );
	}

	[[nodiscard]]
	auto begin()
	{
		return m_dense.begin();
	}

	[[nodiscard]]
	auto end()
	{
		return m_dense.end();
	}

	[[nodiscard]]
	auto begin() const
	{
		return m_dense.begin();
	}

	[[nodiscard]]
	auto end() const
	{
		return m_dense.end();
	}

private:
	std::vector< key_type > m_sparse;  // indirection from sparse index to dense index
	std::vector< value_type > m_dense; // value and indirection from dense index to sparse index
};

template< typename Key, typename T >
using SparseMap = SparseSet< Key, std::pair< Key, T > >;
