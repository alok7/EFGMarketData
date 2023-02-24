#ifndef SPLAYTREE_ALGORITHMS_H
#define SPLAYTREE_ALGORITHMS_H

#include <MarketData/bstree_algorithms.h>

template<class NodeTraits>
struct splaydown_assemble_and_fix_header
{
   typedef typename NodeTraits::node_ptr node_ptr;

   splaydown_assemble_and_fix_header(node_ptr t, node_ptr header, node_ptr leftmost, node_ptr rightmost)
      : t_(t)
      , null_node_(header)
      , l_(null_node_)
      , r_(null_node_)
      , leftmost_(leftmost)
      , rightmost_(rightmost)
   {}

   ~splaydown_assemble_and_fix_header()
   {
      this->assemble();

      //Now recover the original header except for the
      //splayed root node.
      //"t_" is the current root and "null_node_" is the header node
      NodeTraits::set_parent(null_node_, t_);
      NodeTraits::set_parent(t_, null_node_);
      //Recover leftmost/rightmost pointers
      NodeTraits::set_left (null_node_, leftmost_);
      NodeTraits::set_right(null_node_, rightmost_);
   }

   private:

   void assemble()
   {
      //procedure assemble;
      //    left(r), right(l) := right(t), left(t);
      //    left(t), right(t) := right(null), left(null);
      //end assemble;
      {  //    left(r), right(l) := right(t), left(t);

         node_ptr const old_t_left  = NodeTraits::get_left(t_);
         node_ptr const old_t_right = NodeTraits::get_right(t_);
         NodeTraits::set_right(l_, old_t_left);
         NodeTraits::set_left (r_, old_t_right);
         if(old_t_left){
            NodeTraits::set_parent(old_t_left, l_);
         }
         if(old_t_right){
            NodeTraits::set_parent(old_t_right, r_);
         }
      }
      {  //    left(t), right(t) := right(null), left(null);
         node_ptr const null_right = NodeTraits::get_right(null_node_);
         node_ptr const null_left  = NodeTraits::get_left(null_node_);
         NodeTraits::set_left (t_, null_right);
         NodeTraits::set_right(t_, null_left);
         if(null_right){
            NodeTraits::set_parent(null_right, t_);
         }
         if(null_left){
            NodeTraits::set_parent(null_left, t_);
         }
      }
   }

   public:
   node_ptr t_, null_node_, l_, r_, leftmost_, rightmost_;
};

template<class NodeTraits>
class splaytree_algorithms: public bstree_algorithms<NodeTraits> {
	/// @cond
	private:
	
	/// @endcond

	public:
	typedef bstree_algorithms<NodeTraits> bstree_algo;
	typedef typename NodeTraits::node            node;
	typedef NodeTraits                           node_traits;
	typedef typename NodeTraits::node_ptr        node_ptr;
	typedef typename NodeTraits::const_node_ptr  const_node_ptr;

	//! This type is the information that will be
	//! filled by insert_unique_check
	typedef typename bstree_algo::insert_commit_data insert_commit_data;

	public:
	static void erase(node_ptr header, node_ptr z)
	{
		//posibility 1
		if(NodeTraits::get_left(z)){
			splay_up(bstree_algo::prev_node(z), header);
		}

		//possibility 2
		//if(NodeTraits::get_left(z)){
		//   node_ptr l = NodeTraits::get_left(z);
		//   splay_up(l, header);
		//}

		//if(NodeTraits::get_left(z)){
		//   node_ptr l = bstree_algo::prev_node(z);
		//   splay_up_impl(l, z);
		//}

		//possibility 4
		//splay_up(z, header);

		bstree_algo::erase(header, z);
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::transfer_unique
	template<class NodePtrCompare>
	static bool transfer_unique
		(node_ptr header1, NodePtrCompare comp, node_ptr header2, node_ptr z)
	{
		typename bstree_algo::insert_commit_data commit_data;
		bool const transferable = bstree_algo::insert_unique_check(header1, z, comp, commit_data).second;
		if(transferable){
			erase(header2, z);
			bstree_algo::insert_commit(header1, z, commit_data);
			splay_up(z, header1);
		}
		return transferable;
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::transfer_equal
	template<class NodePtrCompare>
	static void transfer_equal
		(node_ptr header1, NodePtrCompare comp, node_ptr header2, node_ptr z)
	{
		insert_commit_data commit_data;
		splay_down(header1, z, comp);
		bstree_algo::insert_equal_upper_bound_check(header1, z, comp, commit_data);
		erase(header2, z);
		bstree_algo::insert_commit(header1, z, commit_data);
	}

	template<class KeyType, class KeyNodePtrCompare>
	static std::size_t count
		(node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
	{
		std::pair<node_ptr, node_ptr> ret = equal_range(header, key, comp);
		std::size_t n = 0;
		while(ret.first != ret.second){
			++n;
			ret.first = next_node(ret.first);
		}
		return n;
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::count(const const_node_ptr&,const KeyType&,KeyNodePtrCompare)
	//! Additional note: no splaying is performed
	template<class KeyType, class KeyNodePtrCompare>
	static std::size_t count
		(const_node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
	{  return bstree_algo::count(header, key, comp);  }

	//! @copydoc ::boost::intrusive::bstree_algorithms::lower_bound(const const_node_ptr&,const KeyType&,KeyNodePtrCompare)
	//! Additional notes: the first node of the range is splayed.
	template<class KeyType, class KeyNodePtrCompare>
	static node_ptr lower_bound
		(node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
	{
		splay_down(header, key, comp);
		node_ptr y = bstree_algo::lower_bound(header, key, comp);
		//splay_up(y, detail::uncast(header));
		return y;
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::lower_bound(const const_node_ptr&,const KeyType&,KeyNodePtrCompare)
	//! Additional note: no splaying is performed
	template<class KeyType, class KeyNodePtrCompare>
	static node_ptr lower_bound
		(const_node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
	{  return bstree_algo::lower_bound(header, key, comp);  }

	//! @copydoc ::boost::intrusive::bstree_algorithms::upper_bound(const const_node_ptr&,const KeyType&,KeyNodePtrCompare)
	//! Additional notes: the first node of the range is splayed.
	template<class KeyType, class KeyNodePtrCompare>
	static node_ptr upper_bound
		(node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
	{
		splay_down(header, key, comp);
		node_ptr y = bstree_algo::upper_bound(header, key, comp);
		//splay_up(y, detail::uncast(header));
		return y;
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::upper_bound(const const_node_ptr&,const KeyType&,KeyNodePtrCompare)
	//! Additional note: no splaying is performed
	template<class KeyType, class KeyNodePtrCompare>
	static node_ptr upper_bound
		(const_node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
	{  return bstree_algo::upper_bound(header, key, comp);  }

	//! @copydoc ::boost::intrusive::bstree_algorithms::find(const const_node_ptr&, const KeyType&,KeyNodePtrCompare)
	//! Additional notes: the found node of the lower bound is splayed.
	template<class KeyType, class KeyNodePtrCompare>
	static node_ptr find
		(node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
	{
		splay_down(header, key, comp);
		return bstree_algo::find(header, key, comp);
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::find(const const_node_ptr&, const KeyType&,KeyNodePtrCompare)
	//! Additional note: no splaying is performed
	template<class KeyType, class KeyNodePtrCompare>
	static node_ptr find
		(const_node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
	{  return bstree_algo::find(header, key, comp);  }

	//! @copydoc ::boost::intrusive::bstree_algorithms::equal_range(const const_node_ptr&,const KeyType&,KeyNodePtrCompare)
	//! Additional notes: the first node of the range is splayed.
	template<class KeyType, class KeyNodePtrCompare>
	static std::pair<node_ptr, node_ptr> equal_range
		(node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
	{
		splay_down(header, key, comp);
		std::pair<node_ptr, node_ptr> ret = bstree_algo::equal_range(header, key, comp);
		//splay_up(ret.first, detail::uncast(header));
		return ret;
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::equal_range(const const_node_ptr&,const KeyType&,KeyNodePtrCompare)
	//! Additional note: no splaying is performed
	template<class KeyType, class KeyNodePtrCompare>
	static std::pair<node_ptr, node_ptr> equal_range
		(const_node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
	{  return bstree_algo::equal_range(header, key, comp);  }

	//! @copydoc ::boost::intrusive::bstree_algorithms::lower_bound_range(const const_node_ptr&,const KeyType&,KeyNodePtrCompare)
	//! Additional notes: the first node of the range is splayed.
	template<class KeyType, class KeyNodePtrCompare>
	static std::pair<node_ptr, node_ptr> lower_bound_range
		(node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
	{
		splay_down(header, key, comp);
		std::pair<node_ptr, node_ptr> ret = bstree_algo::lower_bound_range(header, key, comp);
		//splay_up(ret.first, detail::uncast(header));
		return ret;
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::lower_bound_range(const const_node_ptr&,const KeyType&,KeyNodePtrCompare)
	//! Additional note: no splaying is performed
	template<class KeyType, class KeyNodePtrCompare>
	static std::pair<node_ptr, node_ptr> lower_bound_range
		(const_node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
	{  return bstree_algo::lower_bound_range(header, key, comp);  }

	//! @copydoc ::boost::intrusive::bstree_algorithms::bounded_range(const const_node_ptr&,const KeyType&,const KeyType&,KeyNodePtrCompare,bool,bool)
	//! Additional notes: the first node of the range is splayed.
	template<class KeyType, class KeyNodePtrCompare>
	static std::pair<node_ptr, node_ptr> bounded_range
		(node_ptr header, const KeyType &lower_key, const KeyType &upper_key, KeyNodePtrCompare comp
		, bool left_closed, bool right_closed)
	{
		splay_down(header, lower_key, comp);
		std::pair<node_ptr, node_ptr> ret =
			bstree_algo::bounded_range(header, lower_key, upper_key, comp, left_closed, right_closed);
		//splay_up(ret.first, detail::uncast(header));
		return ret;
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::bounded_range(const const_node_ptr&,const KeyType&,const KeyType&,KeyNodePtrCompare,bool,bool)
	//! Additional note: no splaying is performed
	template<class KeyType, class KeyNodePtrCompare>
	static std::pair<node_ptr, node_ptr> bounded_range
		(const_node_ptr header, const KeyType &lower_key, const KeyType &upper_key, KeyNodePtrCompare comp
		, bool left_closed, bool right_closed)
	{  return bstree_algo::bounded_range(header, lower_key, upper_key, comp, left_closed, right_closed);  }

	//! @copydoc ::boost::intrusive::bstree_algorithms::insert_equal_upper_bound(node_ptr,node_ptr,NodePtrCompare)
	//! Additional note: the inserted node is splayed
	template<class NodePtrCompare>
	static node_ptr insert_equal_upper_bound
		(node_ptr header, node_ptr new_node, NodePtrCompare comp)
	{
		splay_down(header, new_node, comp);
		return bstree_algo::insert_equal_upper_bound(header, new_node, comp);
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::insert_equal_lower_bound(node_ptr,node_ptr,NodePtrCompare)
	//! Additional note: the inserted node is splayed
	template<class NodePtrCompare>
	static node_ptr insert_equal_lower_bound
		(node_ptr header, node_ptr new_node, NodePtrCompare comp)
	{
		splay_down(header, new_node, comp);
		return bstree_algo::insert_equal_lower_bound(header, new_node, comp);
	}  

	//! @copydoc ::boost::intrusive::bstree_algorithms::insert_equal(node_ptr,node_ptr,node_ptr,NodePtrCompare)
	//! Additional note: the inserted node is splayed
	template<class NodePtrCompare>
	static node_ptr insert_equal
		(node_ptr header, node_ptr hint, node_ptr new_node, NodePtrCompare comp)
	{
		splay_down(header, new_node, comp);
		return bstree_algo::insert_equal(header, hint, new_node, comp);
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::insert_before(node_ptr,node_ptr,node_ptr)
	//! Additional note: the inserted node is splayed
	static node_ptr insert_before
		(node_ptr header, node_ptr pos, node_ptr new_node)
	{
		bstree_algo::insert_before(header, pos, new_node);
		splay_up(new_node, header);
		return new_node;
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::push_back(node_ptr,node_ptr)
	//! Additional note: the inserted node is splayed
	static void push_back(node_ptr header, node_ptr new_node)
	{
		bstree_algo::push_back(header, new_node);
		splay_up(new_node, header);
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::push_front(node_ptr,node_ptr)
	//! Additional note: the inserted node is splayed
	static void push_front(node_ptr header, node_ptr new_node)
	{
		bstree_algo::push_front(header, new_node);
		splay_up(new_node, header);
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::insert_unique_check(const const_node_ptr&,const KeyType&,KeyNodePtrCompare,insert_commit_data&)
	//! Additional note: nodes with the given key are splayed
	template<class KeyType, class KeyNodePtrCompare>
	static std::pair<node_ptr, bool> insert_unique_check
		(node_ptr header, const KeyType &key
		,KeyNodePtrCompare comp, insert_commit_data &commit_data)
	{
		splay_down(header, key, comp);
		return bstree_algo::insert_unique_check(header, key, comp, commit_data);
	}

	//! @copydoc ::boost::intrusive::bstree_algorithms::insert_unique_check(const const_node_ptr&,const node_ptr&,const KeyType&,KeyNodePtrCompare,insert_commit_data&)
	//! Additional note: nodes with the given key are splayed
	template<class KeyType, class KeyNodePtrCompare>
	static std::pair<node_ptr, bool> insert_unique_check
		(node_ptr header, node_ptr hint, const KeyType &key
		,KeyNodePtrCompare comp, insert_commit_data &commit_data)
	{
		splay_down(header, key, comp);
		return bstree_algo::insert_unique_check(header, hint, key, comp, commit_data);
	}

	// bottom-up splay, use data_ as parent for n    | complexity : logarithmic    | exception : nothrow
	static void splay_up(node_ptr node, node_ptr header)
	{  priv_splay_up<true>(node, header); }

	// top-down splay | complexity : logarithmic    | exception : strong, note A
	template<class KeyType, class KeyNodePtrCompare>
	static node_ptr splay_down(node_ptr header, const KeyType &key, KeyNodePtrCompare comp, bool *pfound = 0)
	{  return priv_splay_down<true>(header, key, comp, pfound);   }

	private:

	/// @cond

	// bottom-up splay, use data_ as parent for n    | complexity : logarithmic    | exception : nothrow
	template<bool SimpleSplay>
	static void priv_splay_up(node_ptr node, node_ptr header)
	{
		// If (node == header) do a splay for the right most node instead
		// this is to boost performance of equal_range/count on equivalent containers in the case
		// where there are many equal elements at the end
		node_ptr n((node == header) ? NodeTraits::get_right(header) : node);
		node_ptr t(header);

		if( n == t ) return;

		for( ;; ){
			node_ptr p(NodeTraits::get_parent(n));
			node_ptr g(NodeTraits::get_parent(p));

			if( p == t )   break;

			if( g == t ){
				// zig
				rotate(n);
			}
			else if ((NodeTraits::get_left(p) == n && NodeTraits::get_left(g) == p)    ||
						(NodeTraits::get_right(p) == n && NodeTraits::get_right(g) == p)  ){
				// zig-zig
				rotate(p);
				rotate(n);
			}
			else {
				// zig-zag
				rotate(n);
				if(!SimpleSplay){
					rotate(n);
				}
			}
		}
	}

	template<bool SimpleSplay, class KeyType, class KeyNodePtrCompare>
	static node_ptr priv_splay_down(node_ptr header, const KeyType &key, KeyNodePtrCompare comp, bool *pfound = 0)
	{
		//Most splay tree implementations use a dummy/null node to implement.
		//this function. This has some problems for a generic library like Intrusive:
		//
		// * The node might not have a default constructor.
		// * The default constructor could throw.
		//
		//We already have a header node. Leftmost and rightmost nodes of the tree
		//are not changed when splaying (because the invariants of the tree don't
		//change) We can back up them, use the header as the null node and
		//reassign old values after the function has been completed.
		node_ptr const old_root  = NodeTraits::get_parent(header);
		node_ptr const leftmost  = NodeTraits::get_left(header);
		node_ptr const rightmost = NodeTraits::get_right(header);
		if(leftmost == rightmost){ //Empty or unique node
			if(pfound){
				*pfound = old_root && !comp(key, old_root) && !comp(old_root, key);
			}
			return old_root ? old_root : header;
		}
		else{
			//Initialize "null node" (the header in our case)
			NodeTraits::set_left (header, node_ptr());
			NodeTraits::set_right(header, node_ptr());
			//Class that will backup leftmost/rightmost from header, commit the assemble(),
			//and will restore leftmost/rightmost to header even if "comp" throws
			splaydown_assemble_and_fix_header<NodeTraits> commit(old_root, header, leftmost, rightmost);
			bool found = false;

			for( ;; ){
				if(comp(key, commit.t_)){
					node_ptr const t_left = NodeTraits::get_left(commit.t_);
					if(!t_left)
						break;
					if(comp(key, t_left)){
						bstree_algo::rotate_right_no_parent_fix(commit.t_, t_left);
						commit.t_ = t_left;
						if( !NodeTraits::get_left(commit.t_) )
							break;
						link_right(commit.t_, commit.r_);
					}
					else{
						link_right(commit.t_, commit.r_);
						if(!SimpleSplay && comp(t_left, key)){
							if( !NodeTraits::get_right(commit.t_) )
								break;
							link_left(commit.t_, commit.l_);
						}
					}
				}
				else if(comp(commit.t_, key)){
					node_ptr const t_right = NodeTraits::get_right(commit.t_);
					if(!t_right)
						break;

					if(comp(t_right, key)){
							bstree_algo::rotate_left_no_parent_fix(commit.t_, t_right);
							commit.t_ = t_right;
							if( !NodeTraits::get_right(commit.t_) )
								break;
							link_left(commit.t_, commit.l_);
					}
					else{
						link_left(commit.t_, commit.l_);
						if(!SimpleSplay && comp(key, t_right)){
							if( !NodeTraits::get_left(commit.t_) )
								break;
							link_right(commit.t_, commit.r_);
						}
					}
				}
				else{
					found = true;
					break;
				}
			}

			//commit.~splaydown_assemble_and_fix_header<NodeTraits>() will first
			//"assemble()" + link the new root & recover header's leftmost & rightmost
			if(pfound){
				*pfound = found;
			}
			return commit.t_;
		}
	}

	// break link to left child node and attach it to left tree pointed to by l   | complexity : constant | exception : nothrow
	static void link_left(node_ptr & t, node_ptr & l)
	{
		//procedure link_left;
		//    t, l, right(l) := right(t), t, t
		//end link_left
		NodeTraits::set_right(l, t);
		NodeTraits::set_parent(t, l);
		l = t;
		t = NodeTraits::get_right(t);
	}

	// break link to right child node and attach it to right tree pointed to by r | complexity : constant | exception : nothrow
	static void link_right(node_ptr & t, node_ptr & r)
	{
		//procedure link_right;
		//    t, r, left(r) := left(t), t, t
		//end link_right;
		NodeTraits::set_left(r, t);
		NodeTraits::set_parent(t, r);
		r = t;
		t = NodeTraits::get_left(t);
	}

	// rotate n with its parent                     | complexity : constant    | exception : nothrow
	static void rotate(node_ptr n)
	{
		//procedure rotate_left;
		//    t, right(t), left(right(t)) := right(t), left(right(t)), t
		//end rotate_left;
		node_ptr p = NodeTraits::get_parent(n);
		node_ptr g = NodeTraits::get_parent(p);
		//Test if g is header before breaking tree
		//invariants that would make is_header invalid
		bool g_is_header = bstree_algo::is_header(g);

		if(NodeTraits::get_left(p) == n){
			NodeTraits::set_left(p, NodeTraits::get_right(n));
			if(NodeTraits::get_left(p))
				NodeTraits::set_parent(NodeTraits::get_left(p), p);
			NodeTraits::set_right(n, p);
		}
		else{ // must be ( p->right == n )
			NodeTraits::set_right(p, NodeTraits::get_left(n));
			if(NodeTraits::get_right(p))
				NodeTraits::set_parent(NodeTraits::get_right(p), p);
			NodeTraits::set_left(n, p);
		}

		NodeTraits::set_parent(p, n);
		NodeTraits::set_parent(n, g);

		if(g_is_header){
			if(NodeTraits::get_parent(g) == p)
				NodeTraits::set_parent(g, n);
			else{//must be ( g->right == p )
				ASSERT(false);
				NodeTraits::set_right(g, n);
			}
		}
		else{
			if(NodeTraits::get_left(g) == p)
				NodeTraits::set_left(g, n);
			else  //must be ( g->right == p )
				NodeTraits::set_right(g, n);
		}
	}

	/// @endcond
};

#endif   /* SPLAYTREE_ALGORITHMS_H */
