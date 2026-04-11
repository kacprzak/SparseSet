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

	/**
	 * Removes key from linked list.
	 *
	 * Returns new list front.
	 */
	key_type list_remove( key_type list, key_type to_remove )
	{
		// If to_remove is first element.
		if( list == to_remove )
			return m_relations.at( to_remove ).next;

		// Scan to find and update previous element.
		auto current = list;
		while( current != s_invalid )
		{
			auto& relation = m_relations.at( current );

			if( relation.next == to_remove )
			{
				relation.next = m_relations.at( to_remove ).next;
				break;
			}

			current = relation.next;
		}

		return list;
	}

	void erase_children( key_type list )
	{
		auto current = list;
		while( current != s_invalid )
		{
			const auto& relation = m_relations.at( current );
			const auto next      = relation.next;

			erase_children( relation.children );
			m_relations.erase( current );
			m_map.erase( current );

			current = next;
		}
	}

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
		m_root = s_invalid;
	}

	constexpr void reserve( size_type n )
	{
		m_map.reserve( n );
		m_relations.reserve( n );
	}

	bool insert( const key_type& key, const value_type& value )
	{
		m_relations.insert( key, Relation{ .next = m_root } );
		m_root = key;

		return m_map.insert( key, value );
	}

	bool insert( const key_type& key, const value_type& value, const key_type& parent )
	{
		if( m_map.contains( key ) )
			return false;

		Relation relation{ .parent = parent };

		// Parent relation needs to be updated
		auto& parent_relations = m_relations.at( parent );

		relation.next             = parent_relations.children;
		parent_relations.children = key;

		m_relations.insert( key, relation );
		return m_map.insert( key, value );
	}

	bool insert_after( const key_type& key, const value_type& value, const key_type& sibling )
	{
		if( m_map.contains( key ) )
			return false;

		auto& sibling_relation = m_relations.at( sibling );

		Relation relation{ .next = sibling_relation.next, .parent = sibling_relation.parent };

		sibling_relation.next = key;

		m_relations.insert( key, relation );
		return m_map.insert( key, value );
	}

	bool erase( const key_type& key )
	{
		if( not m_map.contains( key ) )
			return false;

		const auto& relation = m_relations.at( key );

		// Remove from children list or roots list.
		auto& list = relation.parent != s_invalid ? m_relations.at( relation.parent ).children : m_root;
		list       = list_remove( list, key );

		// Erase children
		erase_children( relation.children );

		m_relations.erase( key );

		return m_map.erase( key );
	}

	[[nodiscard]] auto at( const key_type& key ) -> reference { return m_map.at( key ); }
	[[nodiscard]] auto at( const key_type& key ) const -> const_reference { return m_map.at( key ); }

	[[nodiscard]]
	auto parent( const key_type& key ) -> iterator
	{
		return m_map.find( m_relations.at( key ).parent );
	}

	[[nodiscard]]
	auto parent( const key_type& key ) const -> const_iterator
	{
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
	key_type m_root = s_invalid;    //< first root
	std::queue< key_type > m_queue; //< for BFS
};

} // namespace sparse
