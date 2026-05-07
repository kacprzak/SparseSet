#pragma once

#include "SparseMap.hpp"

#include <cassert>
#include <concepts>
#include <iterator>
#include <queue>
#include <vector>

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
 * - `for_each_bfs(root, f)` traverses the subtree rooted at @p root in breadth-first order.
 * - `for_each_dfs()` traverses nodes in pre-order depth-first order across all roots.
 * - `for_each_dfs(root, on_enter, on_leave)` traverses the subtree rooted at @p root in DFS order.
 * - `children_begin()` / `children_next()` allow manual child traversal.
 * - `sort_bfs()` reorders flat storage to match BFS visitation order, improving
 *   cache locality for breadth-first workloads.
 *
 * ### Complexity
 * | Operation       | Time      |
 * |-----------------|-----------|
 * | `insert`        | O(1)      |
 * | `set_parent`    | O(depth)  |
 * | `unset_parent`  | O(1)      |
 * | `erase`         | O(n) — erases entire subtree |
 * | `contains`      | O(1)      |
 * | `at`            | O(1)      |
 * | `for_each_bfs`  | O(n)      |
 * | `for_each_dfs`  | O(n)      |
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
	 * Returns the smallest key that is not currently in use.
	 *
	 * Useful for allocating the next available key without manual tracking.
	 * If all keys in [0, size-1] are occupied, returns `size`, which is a
	 * valid key for the next insertion.
	 */
	[[nodiscard]] constexpr key_type find_slot() const { return m_map.find_slot(); }

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
	 * Moves @p key to become a child of @p new_parent, preserving @p key's subtree.
	 *
	 * The node is unlinked from its current parent (or the root list) and
	 * prepended to @p new_parent's child list. All descendants of @p key move
	 * with it.
	 *
	 * @param  key        Key of the node to move.
	 * @param  new_parent Key of the node that will become the new parent.
	 * @returns `true` if the move succeeded; `false` if:
	 *          - @p key or @p new_parent do not exist,
	 *          - @p key == @p new_parent, or
	 *          - @p new_parent is a descendant of @p key (would create a cycle).
	 * @pre    Both keys must exist in the tree.
	 */
	bool set_parent( const key_type& key, const key_type& new_parent )
	{
		if( not m_map.contains( key ) or not m_map.contains( new_parent ) )
			return false;

		if( key == new_parent )
			return false;

		// Reject if new_parent is already the current parent (no-op that is still valid, so true).
		if( m_relations.at( key ).parent == new_parent )
			return true;

		// Cycle check: walk up from new_parent; if we reach key, it would create a cycle.
		for( auto ancestor = m_relations.at( new_parent ).parent; ancestor != s_invalid;
		     ancestor      = m_relations.at( ancestor ).parent )
		{
			if( ancestor == key )
				return false;
		}

		// Detach key from its current position.
		const auto old_parent = m_relations.at( key ).parent;
		if( old_parent != s_invalid )
		{
			auto& siblings     = m_relations.at( old_parent ).children;
			siblings           = list_remove( siblings, key );
		}
		else
		{
			m_root = list_remove( m_root, key );
		}

		// Prepend key to new_parent's child list.
		auto& new_parent_rel          = m_relations.at( new_parent );
		m_relations.at( key ).next    = new_parent_rel.children;
		m_relations.at( key ).parent  = new_parent;
		new_parent_rel.children       = key;

		return true;
	}

	/**
	 * Moves @p key to become a root node, preserving its subtree.
	 *
	 * The node is unlinked from its current parent (if any) and prepended to
	 * the root list. If @p key is already a root, this is a no-op.
	 *
	 * @param  key Key of the node to promote to root.
	 * @returns `true` if @p key exists (regardless of whether it was moved);
	 *          `false` if @p key does not exist.
	 */
	bool unset_parent( const key_type& key )
	{
		if( not m_map.contains( key ) )
			return false;

		auto& rel = m_relations.at( key );
		if( rel.parent == s_invalid )
			return true; // already a root

		// Detach from current parent.
		auto& siblings  = m_relations.at( rel.parent ).children;
		siblings        = list_remove( siblings, key );

		// Prepend to root list.
		rel.next   = m_root;
		rel.parent = s_invalid;
		m_root     = key;

		return true;
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
	constexpr auto children_begin( const key_type& key ) const noexcept -> const_iterator
	{
		return m_map.find( m_relations.at( key ).children );
	}

	[[nodiscard]]
	constexpr auto children_next( const iterator& it ) noexcept -> iterator
	{
		return m_map.find( m_relations.at( it->first ).next );
	}
	[[nodiscard]]
	constexpr auto children_next( const const_iterator& it ) const noexcept -> const_iterator
	{
		return m_map.find( m_relations.at( it->first ).next );
	}

	/**
	 * Traverses all nodes in breadth-first order, invoking @p f on each node.
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

	/** Const overload of `for_each_bfs(f)`. */
	template< typename Callable >
	void for_each_bfs( Callable&& f ) const
	{
		for_each_bfs_impl( *this, std::forward< Callable >( f ) );
	}

	/**
	 * Traverses all nodes in depth-first pre-order, invoking @p on_enter and @p on_leave on each node.
	 *
	 * Both callbacks receive a reference to the underlying map's key/value pair
	 * (i.e. `std::pair<const key_type, value_type>&`).
	 * @p on_enter is called before a node's children are visited; returning `false` skips
	 * the node's entire subtree while still firing @p on_leave for that node.
	 * @p on_leave is called after all of a node's descendants have been visited.
	 *
	 * NOT re-entrant and NOT thread-safe across const calls on the same instance.
	 *
	 * @tparam OnEnter Invocable with signature `bool(iterator::reference)`.
	 * @tparam OnLeave Invocable with signature `void(iterator::reference)`.
	 * @param  on_enter Callback invoked before a node's children.
	 * @param  on_leave Callback invoked after a node's children.
	 */
	template< typename OnEnter, typename OnLeave >
	void for_each_dfs( OnEnter&& on_enter, OnLeave&& on_leave )
	{
		for_each_dfs_impl( *this, std::forward< OnEnter >( on_enter ), std::forward< OnLeave >( on_leave ) );
	}

	/** Const overload of `for_each_dfs(on_enter, on_leave)`. */
	template< typename OnEnter, typename OnLeave >
	void for_each_dfs( OnEnter&& on_enter, OnLeave&& on_leave ) const
	{
		for_each_dfs_impl( *this, std::forward< OnEnter >( on_enter ), std::forward< OnLeave >( on_leave ) );
	}

	/**
	 * Traverses the subtree rooted at @p root in breadth-first order, invoking @p f on each node.
	 *
	 * Only @p root and its descendants are visited. The callback receives a reference to the
	 * underlying map's key/value pair (i.e. `std::pair<const key_type, value_type>&`).
	 *
	 * NOT re-entrant and NOT thread-safe across const calls on the same instance.
	 *
	 * @tparam Callable Invocable with signature `void(iterator::reference)`.
	 * @param  root Key of the node at which traversal begins.
	 * @param  f    Callback invoked once per visited node.
	 * @pre    @p root must exist in the tree.
	 */
	template< typename Callable >
	void for_each_bfs( const key_type& root, Callable&& f )
	{
		for_each_bfs_from_impl( *this, root, std::forward< Callable >( f ) );
	}

	/** Const overload of `for_each_bfs(root, f)`. */
	template< typename Callable >
	void for_each_bfs( const key_type& root, Callable&& f ) const
	{
		for_each_bfs_from_impl( *this, root, std::forward< Callable >( f ) );
	}

	/**
	 * Traverses the subtree rooted at @p root in depth-first pre-order, invoking @p on_enter
	 * and @p on_leave on each node.
	 *
	 * Only @p root and its descendants are visited. The callback semantics are identical to
	 * `for_each_dfs(on_enter, on_leave)`: @p on_enter returning `false` skips the subtree
	 * while still firing @p on_leave for that node.
	 *
	 * NOT re-entrant and NOT thread-safe across const calls on the same instance.
	 *
	 * @tparam OnEnter Invocable with signature `bool(iterator::reference)`.
	 * @tparam OnLeave Invocable with signature `void(iterator::reference)`.
	 * @param  root     Key of the node at which traversal begins.
	 * @param  on_enter Callback invoked before a node's children.
	 * @param  on_leave Callback invoked after a node's children.
	 * @pre    @p root must exist in the tree.
	 */
	template< typename OnEnter, typename OnLeave >
	void for_each_dfs( const key_type& root, OnEnter&& on_enter, OnLeave&& on_leave )
	{
		for_each_dfs_from_impl( *this, root, std::forward< OnEnter >( on_enter ), std::forward< OnLeave >( on_leave ) );
	}

	/** Const overload of `for_each_dfs(root, on_enter, on_leave)`. */
	template< typename OnEnter, typename OnLeave >
	void for_each_dfs( const key_type& root, OnEnter&& on_enter, OnLeave&& on_leave ) const
	{
		for_each_dfs_from_impl( *this, root, std::forward< OnEnter >( on_enter ), std::forward< OnLeave >( on_leave ) );
	}

private:
	// Drains m_bfs_queue in BFS order. The caller is responsible for seeding the queue.
	template< typename Self, typename Callable >
	static void for_each_bfs_loop( Self& self, Callable&& f )
	{
		auto& queue = self.m_bfs_queue;

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

	// Seeds m_bfs_queue with all roots, then runs the BFS loop.
	template< typename Self, typename Callable >
	static void for_each_bfs_impl( Self& self, Callable&& f )
	{
		assert( self.m_bfs_queue.empty() );

		for( auto root = self.m_root; root != s_invalid; root = self.m_relations.at( root ).next )
			self.m_bfs_queue.push( root );

		for_each_bfs_loop( self, std::forward< Callable >( f ) );
	}

	// Seeds m_bfs_queue with @p start, then runs the BFS loop.
	template< typename Self, typename Callable >
	static void for_each_bfs_from_impl( Self& self, const key_type& start, Callable&& f )
	{
		assert( self.m_bfs_queue.empty() );

		self.m_bfs_queue.push( start );

		for_each_bfs_loop( self, std::forward< Callable >( f ) );
	}

	// Drains m_dfs_stack in DFS pre-order. The caller is responsible for seeding the stack.
	// Uses an explicit stack of (key, is_leave) pairs: on first visit a leave sentinel is pushed
	// and on_enter is called; if it returns false children are skipped, but on_leave still fires.
	template< typename Self, typename OnEnter, typename OnLeave >
	static void for_each_dfs_loop( Self& self, OnEnter&& on_enter, OnLeave&& on_leave )
	{
		auto& stack = self.m_dfs_stack;

		while( not stack.empty() )
		{
			const auto [ current, is_leave ] = stack.back();
			stack.pop_back();

			if( is_leave )
			{
				on_leave( *self.m_map.find( current ) );
				continue;
			}

			stack.emplace_back( current, true );

			if( not on_enter( *self.m_map.find( current ) ) )
				continue;

			const auto children_start = static_cast< std::ptrdiff_t >( stack.size() );
			for( auto child = self.m_relations.at( current ).children; child != s_invalid;
			     child      = self.m_relations.at( child ).next )
			{
				stack.emplace_back( child, false );
			}

			std::reverse( stack.begin() + children_start, stack.end() );
		}
	}

	// Seeds m_dfs_stack with all roots (in reverse list order for correct visitation), then runs the DFS loop.
	template< typename Self, typename OnEnter, typename OnLeave >
	static void for_each_dfs_impl( Self& self, OnEnter&& on_enter, OnLeave&& on_leave )
	{
		auto& stack = self.m_dfs_stack;
		assert( stack.empty() );

		for( auto root = self.m_root; root != s_invalid; root = self.m_relations.at( root ).next )
			stack.emplace_back( root, false );

		std::reverse( stack.begin(), stack.end() );

		for_each_dfs_loop( self, std::forward< OnEnter >( on_enter ), std::forward< OnLeave >( on_leave ) );
	}

	// Seeds m_dfs_stack with @p start, then runs the DFS loop.
	template< typename Self, typename OnEnter, typename OnLeave >
	static void for_each_dfs_from_impl( Self& self, const key_type& start, OnEnter&& on_enter, OnLeave&& on_leave )
	{
		auto& stack = self.m_dfs_stack;
		assert( stack.empty() );

		stack.emplace_back( start, false );

		for_each_dfs_loop( self, std::forward< OnEnter >( on_enter ), std::forward< OnLeave >( on_leave ) );
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
	key_type m_root = s_invalid;                                    //< first root
	mutable std::queue< key_type > m_bfs_queue;                     //< scratch for BFS traversal
	mutable std::vector< std::pair< key_type, bool > > m_dfs_stack; //< scratch for DFS traversal
};

} // namespace sparse
