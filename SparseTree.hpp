#pragma once

#include "SparseMap.hpp"

#include <cassert>
#include <concepts>
#include <queue>

namespace sparse
{

template< std::unsigned_integral Key, std::swappable T >
class Tree final
{
public:
	using key_type        = Key;
	using value_type      = T;
	using size_type       = std::size_t;
	using reference       = value_type&;
	using const_reference = const value_type&;
	using iterator        = Map< Key, T >::iterator;
	using const_iterator  = Map< Key, T >::const_iterator;

private:
	static constexpr auto s_invalid = std::numeric_limits< key_type >::max();

	struct Relation
	{
		key_type children = s_invalid; //< first child
		key_type next     = s_invalid; //< next sibling
		key_type parent   = s_invalid; //< parent
	};

public:
	Tree() = default;
	Tree( std::initializer_list< std::pair< key_type, value_type > > init ) : m_map{ init.begin(), init.end() } {}

	[[nodiscard]] bool empty() const noexcept { return m_map.empty(); }
	[[nodiscard]] size_type size() const noexcept { return m_map.size(); }
	[[nodiscard]] bool contains( const key_type& key ) const noexcept { return m_map.contains( key ); }

	constexpr void clear() noexcept
	{
		m_map.clear();
		m_relations.clear();
	}

	constexpr void reserve( size_type n ) { m_map.reserve( n ); }

	bool insert( const key_type& key, const value_type& value ) { return m_map.insert( key, value ); }
	bool insert( const key_type& key, const value_type& value, const key_type& parent )
	{
		if( m_map.contains( key ) )
			return false;

		if( not m_relations.contains( parent ) )
			m_relations.insert( parent, {} );

		// Parent relations needs to be updated
		auto& parent_relations = m_relations.at( parent );

		if( parent_relations.children == s_invalid )
		{
			// Parent has no children. Set this one as first.
			parent_relations.children = key;
		}
		else
		{
			// Parent has children. Set this as the last one.
			for( auto it = m_map.find( parent_relations.children ); it != end(); it = children_next( it ) )
			{
				auto& child_relations = m_relations.at( it->first );
				if( child_relations.next == s_invalid )
					child_relations.next = key;
			}
		}

		m_relations.insert( key, { .parent = parent } );
		return m_map.insert( key, value );
	}

	bool erase( const key_type& key )
	{
		if( not m_map.contains( key ) )
			return false;

		if( m_relations.contains( key ) )
		{
			const auto relations = m_relations.at( key );
			// Erase children
			auto child = relations.children;
			while( child != s_invalid )
			{
				const auto next = m_relations.at( child ).next;
				erase( child );
				child = next;
			}

			// Detach from parent
			if( relations.parent != s_invalid )
			{
				auto& parent_relations = m_relations.at( relations.parent );
				if( parent_relations.children == key )
				{
					// If first child.
					parent_relations.children = relations.next;
				}
				else
				{
					// Scan parent children until previous found.
					for( auto it = m_map.find( parent_relations.children ); it != end(); it = children_next( it ) )
					{
						auto& child_relations = m_relations.at( it->first );
						if( child_relations.next == key )
						{
							child_relations.next = relations.next;
							break;
						}
					}
				}
			}
			m_relations.erase( key );
		}

		return m_map.erase( key );
	}

	[[nodiscard]] auto at( const key_type& key ) -> reference { return m_map.at( key ); }
	[[nodiscard]] auto at( const key_type& key ) const -> const_reference { return m_map.at( key ); }

	[[nodiscard]]
	auto parent( const key_type& key ) -> iterator
	{
		if( not m_relations.contains( key ) )
			return end();

		return m_map.find( m_relations.at( key ).parent );
	}

	[[nodiscard]]
	auto parent( const key_type& key ) const -> const_iterator
	{
		if( not m_relations.contains( key ) )
			return end();

		return m_map.find( m_relations.at( key ).parent );
	}

	[[nodiscard]] constexpr auto begin() noexcept -> iterator { return m_map.begin(); }
	[[nodiscard]] constexpr auto end() noexcept -> iterator { return m_map.end(); }
	[[nodiscard]] constexpr auto begin() const noexcept -> const_iterator { return m_map.begin(); }
	[[nodiscard]] constexpr auto end() const noexcept -> const_iterator { return m_map.end(); }
	[[nodiscard]] constexpr auto cbegin() noexcept -> const_iterator { return m_map.cbegin(); }
	[[nodiscard]] constexpr auto cend() noexcept -> const_iterator { return m_map.cend(); }
	[[nodiscard]] constexpr auto cbegin() const noexcept -> const_iterator { return m_map.cbegin(); }
	[[nodiscard]] constexpr auto cend() const noexcept -> const_iterator { return m_map.cend(); }

	[[nodiscard]]
	constexpr auto children_begin( const key_type& key ) noexcept -> iterator
	{
		if( not m_relations.contains( key ) )
			return end();

		return m_map.find( m_relations.at( key ).children );
	}

	[[nodiscard]]
	constexpr auto children_next( const iterator& it ) noexcept -> iterator
	{
		return m_map.find( m_relations.at( it->first ).next );
	}

	template< typename Callable >
	void for_each_bfs( Callable f )
	{
		assert( m_queue.empty() );

		// Add all root nodes
		for( const auto& [ k, _ ] : m_map )
		{
			if( parent( k ) == end() )
				m_queue.push( k );
		}

		while( not m_queue.empty() )
		{
			const auto curr = m_queue.front();
			m_queue.pop();

			// Add children to queue
			if( m_relations.contains( curr ) )
			{
				for( auto it = m_map.find( m_relations.at( curr ).children ); it != end(); it = children_next( it ) )
					m_queue.push( it->first );
			}

			// Visit
			f( *m_map.find( curr ) );
		}
	}

	void sort_bfs()
	{
		std::vector< size_type > permutation;
		permutation.reserve( m_map.size() );

		for_each_bfs(
		    [ & ]( const auto& kv )
		    {
			    const auto it = m_map.find( kv.first );
			    permutation.emplace_back( std::distance( m_map.begin(), it ) );
		    } );

		m_map.reorder( permutation );
	}

private:
	Map< key_type, value_type > m_map;
	Map< key_type, Relation > m_relations;
	std::queue< key_type > m_queue; //< for BFS
};

} // namespace sparse
