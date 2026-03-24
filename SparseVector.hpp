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

		std::swap( m_dense[ m_sparse[ first ] ], m_dense[ m_sparse[ second ] ] );
		std::swap( m_sparseInverse[ m_sparse[ first ] ], m_sparseInverse[ m_sparse[ second ] ] );
		std::swap( m_sparse[ first ], m_sparse[ second ] );
	}

public:
	SparseVector() = default;
	SparseVector( std::initializer_list< T > init ) : m_dense{ init }
	{
		m_sparse.resize( init.size() );
		std::iota( m_sparse.begin(), m_sparse.end(), 0 );

		m_sparseInverse.resize( init.size() );
		std::iota( m_sparseInverse.begin(), m_sparseInverse.end(), 0 );
	}

	bool empty() const { return m_dense.empty(); }
	size_type size() const { return m_dense.size(); }

	bool contains( const index_type& index ) const
	{
		return /* index >= 0 and */ index < m_sparse.size() and m_sparse[ index ] != s_tombstone;
	}

	void clear()
	{
		m_dense.clear();
		m_sparse.clear();
		m_sparseInverse.clear();
	}

	template< typename... Args >
	auto emplace( Args&&... args ) -> reference
	{
		reference result = m_dense.emplace_back( std::forward< Args >( args )... );

		auto it = std::find( m_sparse.begin(), m_sparse.end(), s_tombstone );

		it = m_sparse.insert( it, m_sparseInverse.size() );
		m_sparseInverse.emplace_back( std::distance( m_sparse.begin(), it ) );

		return result;
	}

	auto erase( const index_type& index ) -> bool
	{
		if( not contains( index ) )
			return false;

		// Swap and pop
		swap( index, m_sparseInverse.back() );

		m_dense.pop_back();
		m_sparseInverse.pop_back();
		m_sparse[ index ] = s_tombstone;

		return true;
	}

	auto at( const index_type& index ) -> reference { return m_dense.at( m_sparse.at( index ) ); }
	auto at( const index_type& index ) const -> const_reference { return m_dense.at( m_sparse.at( index ) ); }

	auto begin() { return m_dense.begin(); }
	auto end() { return m_dense.end(); }

private:
	std::vector< T > m_dense;
	std::vector< index_type > m_sparse;        // indirection from sparse index to dense index
	std::vector< index_type > m_sparseInverse; // indirection from dense index to sparse index
};
