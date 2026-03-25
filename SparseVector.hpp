#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

template< typename Key, typename T >
concept key_value_pair
    = requires( T pair ) { requires std::same_as< Key, std::remove_cvref_t< decltype( pair.first ) > >; };

/**
 * Container with elements stored contiguously in memory, but indicies remain the same after insertion and erasing.
 */
template< typename Key, typename T = Key >
    requires std::unsigned_integral< Key > && ( std::same_as< Key, T > || key_value_pair< Key, T > )
class SparseVector final
{
public:
	using key_type        = Key;
	using value_type      = T;
	using size_type       = std::size_t;
	using reference       = value_type&;
	using const_reference = const value_type&;

private:
	static constexpr auto s_tombstone = std::numeric_limits< key_type >::max();

	auto to_key( const T& value ) -> key_type
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

		if( key >= m_sparse.size() )
			m_sparse.resize( key + 1u, s_tombstone );

		m_sparse[ key ] = m_dense.size();
		m_dense.emplace_back( value );
	}

public:
	SparseVector() = default;

	SparseVector( std::initializer_list< T > init )
	{
		m_dense.reserve( init.size() );
		m_sparse.reserve( init.size() );

		for( auto item : init )
			insert( item );
	}

	bool empty() const { return m_dense.empty(); }
	size_type size() const { return m_dense.size(); }

	bool contains( const key_type& key ) const { return key < m_sparse.size() and m_sparse[ key ] != s_tombstone; }

	void clear()
	{
		m_dense.clear();
		m_sparse.clear();
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

		// m_data.pop_back();
		m_dense.pop_back();
		m_sparse[ key ] = s_tombstone;

		return true;
	}

	auto at( const key_type& key ) -> reference { return m_dense.at( m_sparse.at( key ) ); }
	auto at( const key_type& key ) const -> const_reference { return m_dense.at( m_sparse.at( key ) ); }

	auto begin() { return m_dense.begin(); }
	auto end() { return m_dense.end(); }

	auto begin() const { return m_dense.begin(); }
	auto end() const { return m_dense.end(); }

private:
	std::vector< key_type > m_sparse; // indirection from sparse index to dense index
	std::vector< T > m_dense;         // indirection from dense index to sparse index
};
