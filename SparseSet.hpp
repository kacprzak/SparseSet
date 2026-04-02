#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sparse
{

/**
 * Set of keys that are stored packed in memory.
 *
 * This class allows for fast iteration on all stored keys like when using std::vector, but this collection provides
 * 0(1) complexity for `contains`, `insert` and `erase` methods. This is achieved at expense of memory.
 *
 * Keys are not stored in order, unless `sort` is called. Any modification may reorder items.
 *
 * @tparam Key key type. Using large values for keys will require to allocate memory for all smaller key values even if
 * not used. This is why it is recommended to use smallest type possible, for example std::uint16_t.
 */
template< std::unsigned_integral Key >
class Set final
{
public:
	using key_type        = Key;
	using value_type      = Key;
	using size_type       = std::size_t;
	using reference       = value_type&;
	using const_reference = const value_type&;

private:
	// Marks unused keys.
	static constexpr auto s_invalid = std::numeric_limits< key_type >::max();

	void swap_by_key( const key_type first, const key_type second )
	{
		std::swap( m_dense[ m_sparse[ first ] ], m_dense[ m_sparse[ second ] ] );
		std::swap( m_sparse[ first ], m_sparse[ second ] );
	}

	void swap( const size_type first, const size_type second )
	{
		std::swap( m_sparse[ m_dense[ first ] ], m_sparse[ m_dense[ second ] ] );
		std::swap( m_dense[ first ], m_dense[ second ] );
	}

	void do_insert( const key_type& key )
	{
		if( key >= m_sparse.size() )
			m_sparse.resize( key + 1u, s_invalid );

		m_sparse[ key ] = m_dense.size();
		m_dense.emplace_back( key );
	}

public:
	Set() = default;

	template< std::input_iterator IterT >
	Set( IterT first, IterT last )
	{
		reserve( std::distance( first, last ) );

		for( ; first != last; ++first )
			insert( *first );
	}

	Set( std::initializer_list< key_type > init ) : Set{ init.begin(), init.end() } {}

	[[nodiscard]] constexpr bool empty() const noexcept { return m_dense.empty(); }
	[[nodiscard]] constexpr size_type size() const noexcept { return m_dense.size(); }

	[[nodiscard]]
	constexpr bool contains( const key_type& key ) const noexcept
	{
		return key < m_sparse.size() and m_sparse[ key ] != s_invalid;
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
		if( key == s_invalid )
			throw std::logic_error{ "Invalid key value." };

		if( contains( key ) )
			return false;

		do_insert( key );
		return true;
	}

	/**
	 * Returns true if erase took place.
	 */
	bool erase( const key_type& key )
	{
		if( not contains( key ) )
			return false;

		// Swap and pop
		swap_by_key( key, m_dense.back() );

		m_dense.pop_back();
		m_sparse[ key ] = s_invalid;

		return true;
	}

	/**
	 * Sorts stored keys in memory.
	 */
	constexpr void sort()
	{
		std::sort( m_dense.begin(), m_dense.end() );

		for( std::size_t i = 0; i < m_dense.size(); ++i )
		{
			std::size_t curr = i;
			std::size_t next = m_sparse[ m_dense[ i ] ];

			while( curr != next )
			{
				m_sparse[ m_dense[ curr ] ] = curr;
				curr                        = next;
				next                        = m_sparse[ m_dense[ curr ] ];
			}
		}
	}

	constexpr void reorder( std::span< size_type > permutation )
	{
		assert( permutation.size() == m_dense.size() );

		for( std::size_t i = 0; i < permutation.size(); ++i )
		{
			std::size_t curr = i;
			std::size_t next = permutation[ i ];

			while( curr != next )
			{
				swap( permutation[ curr ], permutation[ next ] );
				permutation[ curr ] = curr;
				curr                = next;
				next                = permutation[ curr ];
			}
		}
	}

	[[nodiscard]] constexpr auto begin() const noexcept { return m_dense.begin(); }
	[[nodiscard]] constexpr auto end() const noexcept { return m_dense.end(); }

private:
	std::vector< key_type > m_sparse; // maps key -> dense index
	std::vector< key_type > m_dense;  // dense array of keys
};

} // namespace sparse
