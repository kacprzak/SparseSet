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
 * A sparse-set backed map that stores values packed contiguously in memory.
 *
 * Provides O(1) `contains`, `insert`, and `erase` with cache-friendly O(n)
 * iteration, at the cost of additional memory proportional to the largest key
 * ever inserted. Erasure is performed via swap-and-pop, so iteration order is
 * not stable — any mutation may reorder elements. Call `sort` to restore key
 * order.
 *
 * @tparam Key Unsigned integer key type. Memory is allocated for every value
 *             in [0, max_key], so prefer the smallest type that fits your key
 *             range (e.g. `std::uint16_t`).
 * @tparam T   Value type. Must be swappable (required by swap-and-pop erase).
 */
template< std::unsigned_integral Key, std::swappable T >
class Map final
{
public:
	using key_type        = Key;
	using value_type      = T;
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
		std::swap( m_sparse[ m_dense[ first ].first ], m_sparse[ m_dense[ second ].first ] );
		std::swap( m_dense[ first ], m_dense[ second ] );
	}

	void do_insert( const key_type& key, const value_type& value )
	{
		if( key >= m_sparse.size() )
			m_sparse.resize( key + 1u, s_invalid );

		m_sparse[ key ] = m_dense.size();
		m_dense.emplace_back( key, value );
	}

public:
	Map() = default;

	template< std::input_iterator IterT >
	Map( IterT first, IterT last )
	{
		reserve( std::distance( first, last ) );

		for( ; first != last; ++first )
			insert( first->first, first->second );
	}

	Map( std::initializer_list< std::pair< key_type, value_type > > init ) : Map{ init.begin(), init.end() } {}

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
	 * Returns the smallest key that is not currently in use.
	 *
	 * Useful for allocating the next available key without manual tracking.
	 * If all keys in [0, size-1] are occupied, returns `size`, which is a
	 * valid key for the next insertion.
	 */
	[[nodiscard]]
	constexpr key_type find_slot() const
	{
		const auto it = std::find( m_sparse.begin(), m_sparse.end(), s_invalid );

		return std::distance( m_sparse.begin(), it );
	}

	/**
	 * Returns true if insertion took place and false if there is already value under index.
	 */
	bool insert( const key_type& key, const value_type& value )
	{
		if( key == s_invalid )
			throw std::logic_error{ "Invalid key value." };

		if( contains( key ) )
			return false;

		do_insert( key, value );
		return true;
	}

	/**
	 * Inserts @p value at @p key if the key is not already present.
	 *
	 * @param  key   Key to insert at. Must not equal the sentinel value
	 *               (`std::numeric_limits<key_type>::max()`).
	 * @param  value Value to store.
	 * @returns `true` if the value was inserted; `false` if @p key already exists.
	 * @throws std::logic_error if @p key is the sentinel value.
	 */
	bool insert_or_assign( const key_type& key, const value_type& value )
	{
		if( key == s_invalid )
			throw std::logic_error{ "Invalid key value." };

		if( contains( key ) )
		{
			m_dense[ m_sparse[ key ] ] = { key, value };
			return false;
		}

		do_insert( key, value );
		return true;
	}

	/**
	 * Erases the entry at @p key, if present.
	 *
	 * Uses swap-and-pop internally, so the erased slot is filled by the last
	 * element. Iteration order is not preserved after erasure.
	 *
	 * @param  key Key of the entry to erase.
	 * @returns `true` if an entry was erased; `false` if @p key was not found.
	 */
	bool erase( const key_type& key )
	{
		if( not contains( key ) )
			return false;

		// Swap and pop
		swap_by_key( key, m_dense.back().first );

		m_dense.pop_back();
		m_sparse[ key ] = s_invalid;

		return true;
	}

	/**
	 * Sorts the dense storage by key in ascending order.
	 *
	 * After this call, iterating via `begin()`/`end()` visits entries in key
	 * order. Invalidates all iterators. Complexity: O(n log n).
	 */
	constexpr void sort()
	{
		std::sort( m_dense.begin(), m_dense.end(), []( const auto& a, const auto& b ) { return a.first < b.first; } );

		for( std::size_t i = 0; i < m_dense.size(); ++i )
		{
			std::size_t curr = i;
			std::size_t next = m_sparse[ m_dense[ i ].first ];

			while( curr != next )
			{
				m_sparse[ m_dense[ curr ].first ] = curr;
				curr                              = next;
				next                              = m_sparse[ m_dense[ curr ].first ];
			}
		}
	}

	/**
	 * Reorders dense storage according to the given index permutation.
	 *
	 * @p permutation must be the same size as the current number of elements,
	 * where `permutation[i]` is the index that element `i` should move to.
	 * The permutation is applied in-place and consumed (modified) during the call.
	 *
	 * @param permutation A span of destination indices, one per element.
	 * @pre   `permutation.size() == size()`.
	 */
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

	[[nodiscard]]
	auto at( const key_type& key ) -> reference
	{
		return m_dense.at( m_sparse.at( key ) ).second;
	}

	[[nodiscard]]
	auto at( const key_type& key ) const -> const_reference
	{
		return m_dense.at( m_sparse.at( key ) ).second;
	}

	template< class Reference >
	struct ArrowProxy
	{
		Reference r;
		Reference* operator->() { return &r; }
	};

	template< typename IterT, typename ValueT, typename ReferenceT >
	class IteratorProxy
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using iterator_concept  = std::random_access_iterator_tag;
		using difference_type   = std::ptrdiff_t;
		using value_type        = ValueT;
		using reference         = ReferenceT;
		using pointer           = ArrowProxy< reference >;

		constexpr IteratorProxy() noexcept : m_current{} {}
		explicit constexpr IteratorProxy( IterT it ) noexcept : m_current{ it } {}
		constexpr IteratorProxy( const IteratorProxy& ) noexcept            = default;
		constexpr IteratorProxy& operator=( const IteratorProxy& ) noexcept = default;

		constexpr auto operator<=>( const IteratorProxy& other ) const = default;

		constexpr auto operator*() const -> reference { return { m_current->first, m_current->second }; }
		constexpr auto operator->() const -> pointer { return { { m_current->first, m_current->second } }; }

		constexpr auto operator[]( const difference_type n ) const -> reference
		{
			return { m_current[ n ].first, m_current[ n ].second };
		}

		constexpr auto operator++() -> IteratorProxy& { return ++m_current, *this; }
		constexpr auto operator++( int ) -> IteratorProxy
		{
			auto copy = *this;
			++m_current;
			return copy;
		}
		constexpr auto operator+=( const difference_type n ) -> IteratorProxy&
		{
			m_current += n;
			return *this;
		}
		constexpr auto operator+( const difference_type n ) const -> IteratorProxy
		{
			return IteratorProxy{ m_current + n };
		}

		constexpr auto operator--() -> IteratorProxy& { return --m_current, *this; }
		constexpr auto operator--( int ) -> IteratorProxy
		{
			auto copy = *this;
			--m_current;
			return copy;
		}
		constexpr auto operator-=( const difference_type n ) -> IteratorProxy&
		{
			m_current -= n;
			return *this;
		}
		constexpr auto operator-( const difference_type n ) const -> IteratorProxy
		{
			return IteratorProxy{ m_current - n };
		}

		constexpr auto operator-( const IteratorProxy& other ) const -> difference_type
		{
			return m_current - other.m_current;
		}

		friend constexpr auto operator+( difference_type n, IteratorProxy rhs ) -> IteratorProxy { return rhs + n; }

	private:
		IterT m_current; //< The underlying iterator
	};

	using iterator       = IteratorProxy< typename std::vector< std::pair< Key, T > >::iterator,
	                                      typename std::pair< Key&, T& >,
	                                      typename std::pair< const Key&, T& > >;
	using const_iterator = IteratorProxy< typename std::vector< std::pair< Key, T > >::const_iterator,
	                                      typename std::pair< Key&, T& >,
	                                      typename std::pair< const Key&, const T& > >;

	static_assert( std::random_access_iterator< iterator > );
	static_assert( std::random_access_iterator< const_iterator > );

	[[nodiscard]] constexpr auto begin() noexcept -> iterator { return iterator{ m_dense.begin() }; }
	[[nodiscard]] constexpr auto end() noexcept -> iterator { return iterator{ m_dense.end() }; }
	[[nodiscard]] constexpr auto begin() const noexcept -> const_iterator { return const_iterator{ m_dense.begin() }; }
	[[nodiscard]] constexpr auto end() const noexcept -> const_iterator { return const_iterator{ m_dense.end() }; }
	[[nodiscard]] constexpr auto cbegin() const noexcept -> const_iterator
	{
		return const_iterator{ m_dense.cbegin() };
	}
	[[nodiscard]] constexpr auto cend() const noexcept -> const_iterator { return const_iterator{ m_dense.cend() }; }

	constexpr auto find( const key_type& key ) -> iterator
	{
		if( contains( key ) )
			return iterator{ m_dense.begin() + m_sparse[ key ] };
		else
			return end();
	}

	constexpr auto find( const key_type& key ) const -> const_iterator
	{
		if( contains( key ) )
			return const_iterator{ m_dense.begin() + m_sparse[ key ] };
		else
			return end();
	}

private:
	std::vector< key_type > m_sparse;                         // maps key -> dense index
	std::vector< std::pair< key_type, value_type > > m_dense; // dense array of (key, value) pairs
};

} // namespace sparse
