#pragma once

#include "SparseMap.hpp"

#include <cassert>
#include <concepts>
#include <iterator>
#include <queue>

namespace sparse
{

/**
 * An intrusive forest (collection of trees) keyed by unsigned integers.
 *
 * Nodes are stored in a flat `Map<Key, T>` for cache-friendly iteration, with
 * a parallel `Map<Key, Relation>` tracking parent/child/sibling links as an
 * intrusive linked list. This allows O(1) insertion and erasure while keeping
 * values contiguous in memory.
 *
 * ### Structure
 * - Multiple root nodes are supported, forming a **forest**.
 * - Each node may have an arbitrary number of children.
 * - Children and roots are stored as singly-linked lists; new nodes are always
 *   prepended to the relevant list.
 * - Erasing a node also recursively erases its entire subtree.
 *
 * ### Insertion
 * | Method          | Result                                              |
 * |-----------------|-----------------------------------------------------|
 * | `insert(k, v)`           | New root node, prepended to the root list. |
 * | `insert(k, v, parent)`   | New first child of `parent`.               |
 * | `insert_after(k, v, sibling)` | New node directly after `sibling`.    |
 *
 * ### Iteration
 * - `begin()` / `end()` iterate over **all** nodes in flat storage order,
 *   regardless of tree structure.
 * - `for_each_bfs()` traverses nodes in breadth-first order across all roots.
 * - `children_begin()` / `children_next()` allow manual child traversal.
 * - `sort_bfs()` reorders flat storage to match BFS visitation order, improving
 *   cache locality for breadth-first workloads.
 *
 * ### Complexity
 * | Operation       | Time      |
 * |-----------------|-----------|
 * | `insert`        | O(1)      |
 * | `erase`         | O(n) — erases entire subtree |
 * | `contains`      | O(1)      |
 * | `at`            | O(1)      |
 * | `for_each_bfs`  | O(n)      |
 * | `sort_bfs`      | O(n)      |
 *
 * @note `sort_bfs` relies on `Map`'s iterator being random access for O(1)
 *       `std::distance` calls. The overall sort is therefore O(n).
 *
 * @tparam Key Unsigned integer type used to identify nodes.
 * @tparam T   Value type stored at each node. Must be swappable.
 */
template< std::unsigned_integral Key, std::swappable T >
class Tree final
{
public:
	using key_type        = Key;
	using value_type      = T;
	using size_type       = std::size_t;
	using reference       = value_type&;
	using const_reference = const value_type&;
	using iterator        = typename Map< Key, T >::iterator;
	using const_iterator  = typename Map< Key, T >::const_iterator;

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
				return list;
			}

			current = relation.next;
		}

		assert( false && "list_remove: to_remove not found in list" );
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
	Tree( std::initializer_list< std::pair< key_type, value_type > > init )
	{
		m_map.reserve( init.size() );
		m_relations.reserve( init.size() );
		for( const auto& [ key, value ] : init )
			insert( key, value );
	}

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

	/**
	 * Inserts a new root node with the given @p key and @p value.
	 *
	 * The new node is prepended to the root list (i.e. it becomes the new
	 * first root) and has no parent or children.
	 *
	 * @param  key   Unique key identifying the node.
	 * @param  value Value to associate with the node.
	 * @returns `true` if the node was inserted; `false` if @p key already exists.
	 */
	bool insert( const key_type& key, const value_type& value )
	{
		if( m_map.contains( key ) )
			return false;

		m_relations.insert( key, Relation{ .next = m_root } );
		m_root = key;
		return m_map.insert( key, value );
	}

	/**
	 * Inserts a new node with the given @p key and @p value as a child of @p parent.
	 *
	 * The new node is prepended to @p parent's child list (i.e. it becomes
	 * the new first child) and inherits @p parent as its parent.
	 *
	 * @param  key    Unique key identifying the new node.
	 * @param  value  Value to associate with the node.
	 * @param  parent Key of an existing node that will become the parent.
	 * @returns `true` if the node was inserted; `false` if @p key already exists.
	 * @pre @p parent must exist in the tree; throws if not found.
	 */
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

	/**
	 * Inserts a new node with the given @p key and @p value immediately after @p sibling
	 * in the sibling list.
	 *
	 * The new node shares the same parent as @p sibling and is placed directly
	 * after it, pushing any subsequent siblings further along the list.
	 *
	 * @param  key     Unique key identifying the new node.
	 * @param  value   Value to associate with the node.
	 * @param  sibling Key of an existing node after which the new node is inserted.
	 * @returns `true` if the node was inserted; `false` if @p key already exists.
	 * @pre @p sibling must exist in the tree; throws if not found.
	 */
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

	/**
	 * Erases the node identified by @p key and its entire subtree.
	 *
	 * The node is unlinked from its parent's child list (or from the root list
	 * if it has no parent), then all of its descendants are recursively erased
	 * before the node itself is removed from storage.
	 *
	 * @param  key Key of the node to erase.
	 * @returns `true` if the node was erased; `false` if @p key was not found.
	 */
	bool erase( const key_type& key )
	{
		if( not m_map.contains( key ) )
			return false;

		const Relation r = m_relations.at( key );

		if( r.parent != s_invalid )
		{
			auto& parent_children = m_relations.at( r.parent ).children;
			parent_children       = list_remove( parent_children, key );
		}
		else
		{
			m_root = list_remove( m_root, key );
		}

		erase_children( r.children );

		m_relations.erase( key );
		const bool erased = m_map.erase( key );

		assert( m_map.size() == m_relations.size() );

		return erased;
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

	/**
	 * Traverses the tree in breadth-first order, invoking @p f on each node.
	 *
	 * The callback receives a reference to the underlying map's key/value pair
	 * (i.e. `std::pair<const key_type, value_type>&`). Visitation order is:
	 * all roots first (in root-list order), then their children, and so on.
	 *
	 * NOT re-entrant and NOT thread-safe across const calls on the same instance.
	 *
	 * @tparam Callable Invocable with signature `void(iterator::reference)`.
	 * @param  f        Callback invoked once per node.
	 */
	template< typename Callable >
	void for_each_bfs( Callable&& f )
	{
		for_each_bfs_impl( *this, std::forward< Callable >( f ) );
	}

	/**
	 * Const overload of for_each_bfs. The callback receives a const reference
	 * to each key/value pair and must not mutate the tree during traversal.
	 *
	 * NOT re-entrant and NOT thread-safe across const calls on the same instance.
	 */
	template< typename Callable >
	void for_each_bfs( Callable&& f ) const
	{
		for_each_bfs_impl( *this, std::forward< Callable >( f ) );
	}

private:
	/**
	 * Shared BFS implementation for both const and non-const overloads.
	 */
	template< typename Self, typename Callable >
	static void for_each_bfs_impl( Self& self, Callable&& f )
	{
		auto& queue = self.m_bfs_queue;
		assert( queue.empty() );

		for( auto root = self.m_root; root != s_invalid; root = self.m_relations.at( root ).next )
			queue.push( root );

		while( not queue.empty() )
		{
			const auto current = queue.front();
			queue.pop();

			for( auto child = self.m_relations.at( current ).children; child != s_invalid;
			     child      = self.m_relations.at( child ).next )
			{
				queue.push( child );
			}

			f( *self.m_map.find( current ) );
		}
	}

public:
	void sort_bfs()
	{
		std::vector< size_type > permutation;
		permutation.reserve( m_map.size() );

		for_each_bfs(
		    [ & ]( const auto& kv )
		    {
			    const auto it = m_map.find( kv.first );
			    permutation.push_back( static_cast< size_type >( std::distance( m_map.begin(), it ) ) );
		    } );

		m_map.reorder( permutation );
	}

private:
	Map< key_type, value_type > m_map;
	Map< key_type, Relation > m_relations;
	key_type m_root = s_invalid;                //< first root
	mutable std::queue< key_type > m_bfs_queue; //< scratch for BFS traversal
};

} // namespace sparse
