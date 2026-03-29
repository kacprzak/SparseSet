#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

/**
 * Container with elements stored contiguously in memory, but indicies remain the same after insertion and erasing.
 */
template< typename Key, typename T >
    requires std::unsigned_integral< Key >
class SparseMap final
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

	void swap( const key_type first, const key_type second )
	{
		if( first == second )
			return;

		std::swap( m_dense[ m_sparse[ first ] ], m_dense[ m_sparse[ second ] ] );
		std::swap( m_sparse[ first ], m_sparse[ second ] );
	}

	void do_insert( const key_type& key, const value_type& value )
	{
		if( key == s_tombstone )
			throw std::logic_error{ "Invalid key value." };

		if( key >= m_sparse.size() )
			m_sparse.resize( key + 1u, s_tombstone );

		m_sparse[ key ] = m_dense.size();
		m_dense.emplace_back( key, value );
	}

public:
	SparseMap() = default;

	template< typename InputIt >
	SparseMap( InputIt first, InputIt last )
	{
		const auto& size = std::distance( first, last );

		m_dense.reserve( size );
		m_sparse.reserve( size );

		for( ; first != last; ++first )
			insert( first->first, first->second );
	}

	SparseMap( std::initializer_list< std::pair< key_type, value_type > > init ) : SparseMap{ init.begin(), init.end() }
	{
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
	bool insert( const key_type& key, const value_type& value )
	{
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
		if( contains( key ) )
		{
			m_dense[ m_sparse[ key ] ] = { key, value };
			return false;
		}

		do_insert( key, value );
		return true;
	}

	auto erase( const key_type& key ) -> bool
	{
		if( not contains( key ) )
			return false;

		// Swap and pop
		swap( key, m_dense.back().first );

		m_dense.pop_back();
		m_sparse[ key ] = s_tombstone;

		return true;
	}

	void sort()
	{
		std::sort( m_dense.begin(), m_dense.end(), []( const auto& a, const auto& b ) { return a.first < b.first; } );
		std::fill( m_sparse.begin(), m_sparse.end(), s_tombstone );

		for( auto it = m_dense.begin(); it != m_dense.end(); ++it )
			m_sparse[ it->first ] = std::distance( m_dense.begin(), it );
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

	class IteratorProxy
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using difference_type   = std::ptrdiff_t;
		using value_type        = std::pair< key_type, T >;
		using reference         = std::pair< const key_type&, T& >;
		using pointer           = std::pair< const key_type*, T* >;

		IteratorProxy() : m_iterator{} {}
		IteratorProxy( const std::vector< value_type >::iterator& it ) : m_iterator{ it } {}
		IteratorProxy( const IteratorProxy& ) = default;

		auto operator<=>( const IteratorProxy& other ) const = default;

		auto operator*() const -> reference { return *m_iterator; }
		auto operator->() const -> pointer { return &*m_iterator; }
		auto operator[]( const difference_type n ) const -> reference { *m_iterator[ n ]; }

		IteratorProxy& operator++() { return ++m_iterator, *this; }

		IteratorProxy operator++( int )
		{
			auto temp = *this;
			++m_iterator;
			return temp;
		}

		IteratorProxy& operator--() { return --m_iterator, *this; }

		IteratorProxy operator--( int )
		{
			auto temp = *this;
			--m_iterator;
			return temp;
		}

		IteratorProxy& operator+=( const difference_type n )
		{
			m_iterator += n;
			return *this;
		}

		IteratorProxy operator+( const difference_type n ) const
		{
			auto temp = *this;
			return temp + n;
		}

		IteratorProxy& operator-=( const difference_type n )
		{
			m_iterator -= n;
			return *this;
		}

		IteratorProxy operator-( const difference_type n ) const
		{
			auto temp = *this;
			return temp - n;
		}

		difference_type operator-( const IteratorProxy& other ) const { return m_iterator - other.m_iterator; }

		friend IteratorProxy operator+( difference_type n, IteratorProxy rhs ) { return rhs + n; }

	private:
		std::vector< value_type >::iterator m_iterator;
	};

	// static_assert( std::random_access_iterator< IteratorProxy > );

	[[nodiscard]]
	constexpr auto begin() noexcept -> IteratorProxy
	{
		return IteratorProxy{ m_dense.begin() };
	}

	[[nodiscard]]
	constexpr auto end() noexcept -> IteratorProxy
	{
		return IteratorProxy{ m_dense.end() };
	}

	[[nodiscard]]
	constexpr auto begin() const noexcept -> IteratorProxy
	{
		return IteratorProxy{ m_dense.begin() };
	}

	[[nodiscard]]
	constexpr auto end() const noexcept -> IteratorProxy
	{
		return IteratorProxy{ m_dense.end() };
	}

private:
	std::vector< key_type > m_sparse;                         // indirection from sparse index to dense index
	std::vector< std::pair< key_type, value_type > > m_dense; // value and indirection from dense index to sparse index
};
