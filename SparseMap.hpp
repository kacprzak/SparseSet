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
 * Map of items that are stored packed in memory.
 *
 * This class allows for fast iteration on all stored items like when using std::vector, but this collection provides
 * 0(1) complexity for `contains`, `insert` and `erase` methods. This is achived at expense of memory.
 *
 * Items are not stored in keys order, unless `sort` is called. Any modification may reorder items.
 *
 * @tparam Key key type. Using large values for keys will require to allocate memory for all smaller key values even if
 * not used. This is why it is recommended to use smallest type possible, for example std::uint16_t.
 * @tparam T value type
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

	void swap( const key_type first, const key_type second )
	{
		std::swap( m_dense[ m_sparse[ first ] ], m_dense[ m_sparse[ second ] ] );
		std::swap( m_sparse[ first ], m_sparse[ second ] );
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

	template< typename InputIt >
	Map( InputIt first, InputIt last )
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
	 * Returns smallest key without value.
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
	 * Returns true if insertion took place and false if the assignment took place.
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
	 * Returns true if erase took place.
	 */
	bool erase( const key_type& key )
	{
		if( not contains( key ) )
			return false;

		// Swap and pop
		swap( key, m_dense.back().first );

		m_dense.pop_back();
		m_sparse[ key ] = s_invalid;

		return true;
	}

	/**
	 * Sorts items in memory in key order.
	 */
	void sort()
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

	constexpr void reorder( std::span< const key_type > order )
	{
		assert( order.size() <= m_dense.size() );

		for( std::size_t i = 0; i < order.size(); ++i )
		{
			key_type curr = m_dense[ i ].first;
			key_type next = order[ i ];

			while( curr != next )
			{
				swap( curr, next );
				curr = next;
				next = order[ m_sparse[ curr ] ];
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

		IteratorProxy() : m_iterator{} {}
		IteratorProxy( IterT it ) : m_iterator{ it } {}
		IteratorProxy( const IteratorProxy& )            = default;
		IteratorProxy& operator=( const IteratorProxy& ) = default;

		auto operator<=>( const IteratorProxy& other ) const = default;

		auto operator*() const -> reference { return { m_iterator->first, m_iterator->second }; }
		auto operator->() const -> pointer { return { { m_iterator->first, m_iterator->second } }; }
		auto operator[]( const difference_type n ) const -> reference { *m_iterator[ n ]; }

		IteratorProxy& operator++() { return ++m_iterator, *this; }
		IteratorProxy operator++( int )
		{
			auto copy = *this;
			++m_iterator;
			return copy;
		}

		IteratorProxy& operator--() { return --m_iterator, *this; }
		IteratorProxy operator--( int )
		{
			auto copy = *this;
			--m_iterator;
			return copy;
		}

		IteratorProxy& operator+=( const difference_type n )
		{
			m_iterator += n;
			return *this;
		}

		IteratorProxy operator+( const difference_type n ) const
		{
			auto copy = *this;
			return copy + n;
		}

		IteratorProxy& operator-=( const difference_type n )
		{
			m_iterator -= n;
			return *this;
		}

		IteratorProxy operator-( const difference_type n ) const
		{
			auto copy = *this;
			return copy - n;
		}

		difference_type operator-( const IteratorProxy& other ) const { return m_iterator - other.m_iterator; }

		friend IteratorProxy operator+( difference_type n, IteratorProxy rhs ) { return rhs + n; }

	private:
		IterT m_iterator;
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
	[[nodiscard]] constexpr auto cbegin() noexcept -> const_iterator { return const_iterator{ m_dense.cbegin() }; }
	[[nodiscard]] constexpr auto cend() noexcept -> const_iterator { return const_iterator{ m_dense.cend() }; }
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
	std::vector< key_type > m_sparse;                         // indirection from sparse index to dense index
	std::vector< std::pair< key_type, value_type > > m_dense; // value and indirection from dense index to sparse index
};

} // namespace sparse
