#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

/**
 * Container with elements stored contiguously in memory, but indicies remain the same after insertion and erasing.
 */
template< typename T >
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

	void swap( const index_type first, const index_type second )
	{
		if( first == second )
			return;

		std::swap( m_data[ m_sparse[ first ] ], m_data[ m_sparse[ second ] ] );
		std::swap( m_dense[ m_sparse[ first ] ], m_dense[ m_sparse[ second ] ] );
		std::swap( m_sparse[ first ], m_sparse[ second ] );
	}

	void do_insert( const index_type& index, const value_type& value )
	{
		if( index >= m_sparse.size() )
			m_sparse.resize( index + 1, s_tombstone );

		m_sparse[ index ] = m_data.size();
		m_dense.emplace_back( index );
		m_data.emplace_back( value );
	}

public:
	SparseVector() = default;
	SparseVector( std::initializer_list< T > init ) : m_data{ init }
	{
		m_sparse.resize( init.size() );
		std::iota( m_sparse.begin(), m_sparse.end(), 0 );

		m_dense.resize( init.size() );
		std::iota( m_dense.begin(), m_dense.end(), 0 );
	}

	bool empty() const { return m_data.empty(); }
	size_type size() const { return m_data.size(); }

	bool contains( const index_type& index ) const
	{
		return /* index >= 0 and */ index < m_sparse.size() and m_sparse[ index ] != s_tombstone;
	}

	void clear()
	{
		m_data.clear();
		m_dense.clear();
		m_sparse.clear();
	}

	template< typename... Args >
	auto emplace( Args&&... args ) -> reference
	{
		auto it = std::find( m_sparse.begin(), m_sparse.end(), s_tombstone );

		if( it != m_sparse.end() )
			*it = m_dense.size();
		else
			it = m_sparse.insert( it, m_dense.size() );

		m_dense.emplace_back( std::distance( m_sparse.begin(), it ) );

		return m_data.emplace_back( std::forward< Args >( args )... );
	}

	/**
	 * Returns true if insertion took place and false if there is already value under index.
	 */
	bool insert( const index_type& index, const value_type& value )
	{
		if( contains( index ) )
			return false;

		do_insert( index, value );
		return true;
	}

	/**
	 * Returns true if insertion took place and false if the assignment took place.
	 */
	bool insert_or_assign( const index_type& index, const value_type& value )
	{
		if( contains( index ) )
		{
			m_data[ m_sparse[ index ] ] = value;
			return false;
		}

		do_insert( index, value );
		return true;
	}

	auto erase( const index_type& index ) -> bool
	{
		if( not contains( index ) )
			return false;

		// Swap and pop
		swap( index, m_dense.back() );

		m_data.pop_back();
		m_dense.pop_back();
		m_sparse[ index ] = s_tombstone;

		return true;
	}

	auto at( const index_type& index ) -> reference { return m_data.at( m_sparse.at( index ) ); }
	auto at( const index_type& index ) const -> const_reference { return m_data.at( m_sparse.at( index ) ); }

	auto begin() { return m_data.begin(); }
	auto end() { return m_data.end(); }

	auto begin() const { return m_data.begin(); }
	auto end() const { return m_data.end(); }

private:
	std::vector< T > m_data;
	std::vector< index_type > m_sparse; // indirection from sparse index to dense index
	std::vector< index_type > m_dense;  // indirection from dense index to sparse index
};
