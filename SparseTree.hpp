#pragma once

#include "SparseMap.hpp"

#include <concepts>

template< typename Key, typename T >
    requires std::unsigned_integral< Key >
class SparseTree final
{
public:
	using key_type        = Key;
	using value_type      = T;
	using size_type       = std::size_t;
	using reference       = value_type&;
	using const_reference = const value_type&;
	using iterator        = SparseMap< Key, T >::iterator;
	using const_iterator  = SparseMap< Key, T >::const_iterator;

private:
	static constexpr auto s_tombstone = std::numeric_limits< key_type >::max();

	struct Relation
	{
		key_type children = s_tombstone; //< first child
		key_type next     = s_tombstone; //< next sibling
		// key_type prev     = s_tombstone; //< previous sibling
		key_type parent = s_tombstone; //< parent
	};

public:
	SparseTree() = default;
	SparseTree( std::initializer_list< std::pair< key_type, value_type > > init ) : m_map{ init.begin(), init.end() } {}

	[[nodiscard]] bool empty() const noexcept { return m_map.empty(); }
	[[nodiscard]] size_type size() const noexcept { return m_map.size(); }
	[[nodiscard]] bool contains( const key_type& key ) const noexcept { return m_map.contains( key ); }

	bool insert( const key_type& key, const value_type& value ) { return m_map.insert( key, value ); }
	bool insert( const key_type& key, const value_type& value, const key_type& parent )
	{
		if( m_map.contains( key ) )
			return false;

		if( not m_relations.contains( parent ) )
			m_relations.insert( parent, {} );

		// Parent relations needs to be updated
		auto& parent_relations = m_relations.at( parent );

		if( parent_relations.children == s_tombstone )
		{
			// Parent has no children. Set this one as first.
			parent_relations.children = key;
		}
		else
		{
			// Parent has children. Set this as the last one.
			for( auto it = m_map.find( parent_relations.children ); it != end(); it = children_next( it ) )
			{
				auto& child_relations = m_relations.at( ( *it ).first );
				if( child_relations.next == s_tombstone )
					child_relations.next = key;
			}
		}

		m_relations.insert( key, { .parent = parent } );
		return m_map.insert( key, value );
	}

	bool erase( const key_type& key )
	{
		if( m_relations.contains( key ) )
		{
			const auto relations = m_relations.at( key );
			// Erase children
			auto child = relations.children;
			while( child != s_tombstone )
			{
				const auto next = m_relations.at( child ).next;
				erase( child );
				child = next;
			}

			// Detach from parent
			if( relations.parent != s_tombstone )
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
						auto& child_relations = m_relations.at( ( *it ).first );
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

	auto at( const key_type& key ) -> reference { return m_map.at( key ); }
	auto at( const key_type& key ) const -> const_reference { return m_map.at( key ); }

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

	constexpr auto children_next( const iterator& it ) noexcept -> iterator
	{
		return m_map.find( m_relations.at( ( *it ).first ).next );
	}

private:
	SparseMap< Key, T > m_map;
	SparseMap< Key, Relation > m_relations;
};
