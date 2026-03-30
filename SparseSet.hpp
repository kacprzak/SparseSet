#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

/**
 * Set of unsigned integers that are stored packed in memory.
 */
template< typename Key >
    requires std::unsigned_integral< Key >
class SparseSet final
{
public:
	using key_type        = Key;
	using value_type      = Key;
	using size_type       = std::size_t;
	using reference       = value_type&;
	using const_reference = const value_type&;

private:
	// Marks unused keys.
	static constexpr auto s_tombstone = std::numeric_limits< key_type >::max();

	void swap( const key_type first, const key_type second )
	{
		if( first == second )
			return;

		std::swap( m_dense[ m_sparse[ first ] ], m_dense[ m_sparse[ second ] ] );
		std::swap( m_sparse[ first ], m_sparse[ second ] );
	}

	void do_insert( const key_type& key )
	{
		if( key >= m_sparse.size() )
			m_sparse.resize( key + 1u, s_tombstone );

		m_sparse[ key ] = m_dense.size();
		m_dense.emplace_back( key );
	}

public:
	SparseSet() = default;

	template< typename InputIt >
	SparseSet( InputIt first, InputIt last )
	{
		reserve( std::distance( first, last ) );

		for( ; first != last; ++first )
			insert( *first );
	}

	SparseSet( std::initializer_list< key_type > init ) : SparseSet{ init.begin(), init.end() } {}

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

	constexpr void reserve( size_type n )
	{
		m_dense.reserve( n );
		m_sparse.reserve( n );
	}

	/**
	 * Returns true if insertion took place and false if there is already value under index.
	 */
	bool insert( const key_type& key )
	{
		if( key == s_tombstone )
			throw std::logic_error{ "Invalid key value." };

		if( contains( key ) )
			return false;

		do_insert( key );
		return true;
	}

	auto erase( const key_type& key ) -> bool
	{
		if( not contains( key ) )
			return false;

		// Swap and pop
		swap( key, m_dense.back() );

		m_dense.pop_back();
		m_sparse[ key ] = s_tombstone;

		return true;
	}

	constexpr void sort()
	{
		std::sort( m_dense.begin(), m_dense.end() );
		std::fill( m_sparse.begin(), m_sparse.end(), s_tombstone );

		for( auto it = m_dense.begin(); it != m_dense.end(); ++it )
			m_sparse[ *it ] = std::distance( m_dense.begin(), it );
	}

	[[nodiscard]]
	constexpr auto begin() const noexcept
	{
		return m_dense.begin();
	}

	[[nodiscard]]
	constexpr auto end() const noexcept
	{
		return m_dense.end();
	}

private:
	std::vector< key_type > m_sparse; // indirection from sparse index to dense index
	std::vector< key_type > m_dense;  // indirection from dense index to sparse index
};
