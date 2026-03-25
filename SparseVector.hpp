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

template< typename T >
concept index_value_pair = requires( T tuple ) {
	requires std::unsigned_integral< std::remove_cvref_t< decltype( std::get< 0 >( tuple ) ) > >;
};

/**
 * Container with elements stored contiguously in memory, but indicies remain the same after insertion and erasing.
 */
template< typename T >
    requires std::unsigned_integral< T > || index_value_pair< T >
class SparseVector final
{
public:
	using index_type      = std::size_t;
	using value_type      = T;
	using size_type       = std::size_t;
	using reference       = value_type&;
	using const_reference = const value_type&;

private:
	static constexpr auto s_tombstone = std::numeric_limits< index_type >::max();

	auto to_index( const T& value ) -> index_type
	{
		if constexpr( std::is_unsigned_v< T > )
		{
			return value;
		}
		else
		{
			return std::get< 0 >( value );
		}
	}

	void swap( const index_type first, const index_type second )
	{
		if( first == second )
			return;

		std::swap( m_dense[ m_sparse[ first ] ], m_dense[ m_sparse[ second ] ] );
		std::swap( m_sparse[ first ], m_sparse[ second ] );
	}

	void do_insert( const value_type& value )
	{
		const auto index = to_index( value );

		if( index >= m_sparse.size() )
			m_sparse.resize( index + 1u, s_tombstone );

		m_sparse[ index ] = m_dense.size();
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

	bool contains( const index_type& index ) const
	{
		return index < m_sparse.size() and m_sparse[ index ] != s_tombstone;
	}

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
		const auto index = to_index( value );

		if( contains( index ) )
			return false;

		do_insert( value );
		return true;
	}

	/**
	 * Returns true if insertion took place and false if the assignment took place.
	 */
	bool insert_or_assign( const value_type& value )
	{
		const auto index = to_index( value );

		if( contains( index ) )
		{
			m_dense[ m_sparse[ index ] ] = value;
			return false;
		}

		do_insert( value );
		return true;
	}

	auto erase( const index_type& index ) -> bool
	{
		if( not contains( index ) )
			return false;

		// Swap and pop
		swap( index, to_index( m_dense.back() ) );

		// m_data.pop_back();
		m_dense.pop_back();
		m_sparse[ index ] = s_tombstone;

		return true;
	}

	auto at( const index_type& index ) -> reference { return m_dense.at( m_sparse.at( index ) ); }
	auto at( const index_type& index ) const -> const_reference { return m_dense.at( m_sparse.at( index ) ); }

	auto begin() { return m_dense.begin(); }
	auto end() { return m_dense.end(); }

	auto begin() const { return m_dense.begin(); }
	auto end() const { return m_dense.end(); }

private:
	std::vector< index_type > m_sparse; // indirection from sparse index to dense index
	std::vector< T > m_dense;           // indirection from dense index to sparse index
};
