#include <functional>
#include <tuple>
#include <MarketData/splaytree_algorithms.h>
#include <iostream>

#define SPLAYTREE_DEBUGGING_ENABLED false

#define SPLAYTREE_DEBUG(x) do { if (SPLAYTREE_DEBUGGING_ENABLED) std::cerr << x << "\n"; } while (0)
#define SPLAYTREE_DEBUG2(x) do { if (SPLAYTREE_DEBUGGING_ENABLED) std::cerr << #x << ": " << x << "\n"; } while (0)

#ifndef SPLAYTREE_H
#define SPLAYTREE_H

// struct _splay_tree_node_base {
// 	typedef _splay_tree_node_base* _base_ptr;
// 	typedef const _splay_tree_node_base* _const_base_ptr;
	
// 	_base_ptr _parent;
// 	_base_ptr _left;
// 	_base_ptr _right;
// 	_splay_tree_node_base() {}
// 	static _base_ptr _minimum(_base_ptr x) {
//       while (x->_left != 0) x = x->_left;
//       return x;
//     }

//    static _const_base_ptr _minimum(_const_base_ptr x) {
//    	while (x->_left != 0) x = x->_left;
//    	return x;
//    }

//    static _base_ptr _maximum(_base_ptr x) {
//    	while (x->_right != 0) x = x->_right;
//    	return x;
//    }

//    static _const_base_ptr _maximum(_const_base_ptr x) {
//       while (x->_right != 0) x = x->_right;
//       return x;
//     }
// };

template<class _val_type>
struct _splay_tree_node
	//: public _splay_tree_node_base 
	{
	typedef _splay_tree_node<_val_type>		_link_type;
	typedef _link_type*						_link_ptr;
	_val_type _val;
	// static _val_type* _dummy_val;
	_splay_tree_node(const _val_type& val): _val(val) {
		_parent = _left = _right = NULL;
	}
	template<class ..._args>
	_splay_tree_node(_args&&... args): _val(std::forward<_args>(args)...) {
		_parent = _left = _right = NULL;
	}
	// _splay_tree_node(): _val() {}
	// _splay_tree_node(const _key_type& key, const _val_type& val): 
	// 	_key(_key_type(key)), _val(_val_type(val)) {}

	_link_ptr _parent;
	_link_ptr _left;
	_link_ptr _right;

	_val_type* _valPtr() { return &_val; }
	const _val_type* _valPtr() const { return &_val; }

	static _link_ptr _minimum(_link_ptr x) {
		while (x->_left != 0) x = x->_left;
		return x;
	}

	static _link_ptr _maximum(_link_ptr x) {
		while (x->_right != 0) x = x->_right;
		return x;
	}

};

// template<class _key_type, class _val_type>
// _val_type* _splay_tree_node<_key_type, _val_type>::_dummy_val = new _val_type();

//Define splaytree_node_traits
template<class _node_type>
struct _node_traits {
	typedef _node_type				node;
	typedef _node_type*				node_ptr;
	typedef const _node_type*		const_node_ptr;

	static node_ptr get_parent(const_node_ptr n)       {  return n->_parent;   }
	static void set_parent(node_ptr n, node_ptr parent){  n->_parent = parent; }
	static node_ptr get_left(const_node_ptr n)         {  return n->_left;     }
	static void set_left(node_ptr n, node_ptr left)    {  n->_left = left;     }
	static node_ptr get_right(const_node_ptr n)        {  return n->_right;    }
	static void set_right(node_ptr n, node_ptr right)  {  n->_right = right;   }
};

template<class _val_type>
struct _splay_tree_iterator {
	// note that the following 5 members are required for constructing
	// reverse iterator using _splay_tree_iterator
	typedef _val_type		value_type;
	typedef _val_type&		reference;
	typedef _val_type*		pointer;
	typedef std::bidirectional_iterator_tag					iterator_category;
	typedef ptrdiff_t									difference_type;

	typedef _splay_tree_iterator<_val_type>				_self;
	typedef _splay_tree_node<_val_type>					_link_type;
	typedef _link_type*									_link_ptr;
	typedef _node_traits<_link_type>					__node_traits;
	typedef bstree_algorithms_base<__node_traits>		_bstree_base_algo;

	_splay_tree_iterator() noexcept : _iter_node() {}
	explicit _splay_tree_iterator(_link_ptr x) noexcept : _iter_node(x) {}

	reference operator*() const noexcept {
		return *(_iter_node->_valPtr());
	}
  	pointer operator->() const noexcept { 
  		return _iter_node->_valPtr();
  	}
	
	_self& operator++() noexcept {
		_iter_node = _bstree_base_algo::next_node(_iter_node);
		return *this;
	}

	_self operator++(int) noexcept {
		_self tmp = *this;
		_iter_node = _bstree_base_algo::next_node(_iter_node);
		return tmp;
	}

	_self& operator--() noexcept {
		_iter_node = _bstree_base_algo::prev_node(_iter_node);
		return *this;
	}

	_self operator--(int) noexcept {
		_self tmp = *this;
		_iter_node = _bstree_base_algo::prev_node(_iter_node);
		return tmp;
	}

	inline _link_ptr getNode() { return _iter_node; }

	bool operator==(const _self& x) const noexcept { return _iter_node == x._iter_node; }
	bool operator!=(const _self& x) const noexcept { return _iter_node != x._iter_node; }
	
	private:
	_link_ptr _iter_node;
};

template<typename _val_type>
struct _splay_tree_const_iterator {
	// note that the following 5 members are required for constructing
        // const reverse iterator using _splay_tree_const_iterator
	typedef _val_type			value_type;
	typedef const _val_type& 	reference;
	typedef const _val_type* 	pointer;
	typedef _splay_tree_iterator<_val_type> 			_iterator;
	typedef std::bidirectional_iterator_tag					iterator_category;
	typedef ptrdiff_t									difference_type;

	typedef _splay_tree_const_iterator<_val_type>		_self;
	typedef _splay_tree_node<_val_type>					_link_type;
	typedef const _link_type 							_const_link_type;
	typedef _const_link_type*							_const_link_ptr;
	typedef _node_traits<_link_type>					__node_traits;
	typedef bstree_algorithms_base<__node_traits>		_bstree_base_algo;

	_splay_tree_const_iterator() noexcept: _iter_node() {}
	explicit _splay_tree_const_iterator(_link_type x) noexcept : _iter_node(x) {}
	_splay_tree_const_iterator(const _iterator& it) noexcept : _iter_node(it._iter_node) {}

	_iterator _constCast() const noexcept { return _iterator(const_cast<typename _iterator::_link_ptr>(_iter_node)); }

	reference operator*() const noexcept { return *(_iter_node->_valPtr()); }
	pointer operator->() const noexcept { return _iter_node->_valPtr(); }

	_self& operator++() noexcept {
		_iter_node = _bstree_base_algo::next_node(_iter_node);
		return *this;
	}

	_self operator++(int) noexcept {
		_self tmp = *this;
		_iter_node = _bstree_base_algo::next_node(_iter_node);
		return tmp;
	}

	_self& operator--() noexcept {
		_iter_node = _bstree_base_algo::prev_node(_iter_node);
		return *this;
	}

	_self operator--(int) noexcept {
		_self tmp = *this;
		_iter_node = _bstree_base_algo::prev_node(_iter_node);
		return tmp;
	}

	inline _const_link_ptr getNode() { return _iter_node; }

	bool operator==(const _self& x) const noexcept { return _iter_node == x._iter_node; }
	bool operator!=(const _self& x) const noexcept { return _iter_node != x._iter_node; }

	private:
	_const_link_ptr _iter_node;
};

template<class _val_type, class _key_cmp>
struct _key_node_ptr_cmp {
	typedef _splay_tree_node<_val_type> _node_type;
	_key_cmp _cmp;
	_key_node_ptr_cmp () {}
	template<class _key_type>
	bool operator()(const _node_type *a, const _node_type *b) {
		return _cmp(a->val.first, b->_val.first);
	}
	template<class _key_type>
	bool operator()(const _key_type& a, const _node_type *b) {
		return _cmp(a, b->_val.first);
	}
	template<class _key_type>
	bool operator()(const _node_type *a, const _key_type& b) {
		return _cmp(a->_val.first, b);
	}
	template<class _key_type>
	bool operator()(const _key_type& a, const _key_type& b) {
		return _cmp(a, b);
	}
};

template<class KeyType, class MappedType, class KeyCmp = std::less<KeyType>>
class SplayTree {
	/*
		Note that KeyCmp is struct, an instance of which is callable
		with two arguments of the type const reference KeyType
	*/
	protected:
		typedef SplayTree<KeyType, MappedType>					_self;
		// typedef _splay_tree_node_base*            			_base_ptr;
		// typedef const _splay_tree_node_base*      			_const_base_ptr;
		typedef std::pair<const KeyType, MappedType>			_val_type;
		typedef _splay_tree_node<_val_type>						_link_type;
		typedef const _link_type 								_const_link_type;
		typedef _node_traits<_link_type>						__node_traits;
		typedef typename __node_traits::node 	 				_node;
		typedef typename __node_traits::node_ptr 				_node_ptr;
		typedef _node&											_node_ref;
		typedef const _node 									_const_node;
		typedef const _node_ptr									_const_node_ptr;
		typedef _const_node&									_const_node_ref;
		typedef splaytree_algorithms<__node_traits>				_splaytree_algo;
		typedef bstree_algorithms<__node_traits>				_bstree_algo;
		typedef typename _splaytree_algo::insert_commit_data	_insert_commit_data;
		typedef _key_node_ptr_cmp<_val_type, KeyCmp>			_node_ptr_cmp;
	private:
		const size_t _max_size;
		size_t _size;
		_node _header;
		_node_ptr_cmp _cmp;
	public:
		typedef _splay_tree_iterator<_val_type>			       	iterator;
		typedef _splay_tree_const_iterator<_val_type>		 	const_iterator;
		typedef std::reverse_iterator<iterator>				reverse_iterator;
		typedef std::reverse_iterator<const_iterator>			const_reverse_iterator;

		SplayTree(): _header(), _size(0), _max_size(1000) {
			// _header = new _node;
			_splaytree_algo::init_header(&this->_header);
		}

		// note that the name end() can be a bit misleading, the name is just to keep the interface
		// similar to the stl map interface. although In fact end() returns the header,
		// and the prev_node to header is the maximum node so it does make sense
		iterator end() noexcept { return iterator(&this->_header); }
		const_iterator end() const noexcept { return const_iterator(&this->_header); }
		iterator begin() noexcept { return iterator(_bstree_algo::begin_node(&this->_header)); }
		const_iterator begin() const noexcept { return const_iterator(_bstree_algo::begin_node(&this->_header)); }
		reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
		const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
		reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
		const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
		
		inline size_t size() { return _size; }
		void prune() {
			// prune should be called when _size exceeds _max_size
			while(_size > _max_size) {
				// remove the last element
				iterator it = end();
				erase(--it);
			}
		}
		iterator insertAfter(const KeyType& key, const MappedType& new_val, iterator hint) {
			// hint is supposed to be the lower bound of key for efficient insertion using this method
			// inserts in the first position after hint where cmp(hint->second, new_val);
			while(!cmp(key, hint->first)) {
				hint++;
			}
			// _hint is now the upper bound of key
			this->insert(key, new_val, hint);

		}
		// insertBefore is just to keep the interface neat, one can achieve the same functionality using insert
		iterator insertBefore(const KeyType& key, const MappedType& new_val, iterator hint) {
			// hint is supposed to be the upper bound of key for efficient insertion using this method
			this->insert(key, new_val, hint);
		}
		inline iterator insert(const KeyType& new_key, const MappedType& new_val) { return insert(new_key, new_val, end()); }
		iterator insert(const KeyType& new_key, const MappedType& new_val, iterator hint) {
			// if the upper bound of key is know, pass its iterator as hint for constant time insertion
			_insert_commit_data tmp_insert_commit_data;
			// check if a the key already exists in the tree
			_node_ptr new_node; bool not_present;
			std::tie(new_node, not_present) = _splaytree_algo::insert_unique_check(
				&this->_header, hint.getNode(), new_key, this->_cmp, tmp_insert_commit_data);
			// if the key already exists, then return the node_ptr
			// else create the node and insert using the data obtained in tmp_insert_commit_data;
			if (not_present) {
				SPLAYTREE_DEBUG("Node is not present in the tree. Will insert it now!");
				new_node = new _node(_val_type(new_key, new_val));
				_bstree_algo::insert_unique_commit(&this->_header, new_node, tmp_insert_commit_data);
				_size++;
			} else {
				SPLAYTREE_DEBUG("Node is already present in the tree!");
			}
			if (_size > _max_size) prune();
			// Debug statement
			// if (_bstree_algo::size(&_header) > _max_size) MD_LOG_INFO << "Splaytree size exceeded" << "\n";
			return iterator(new_node);
		}
		void erase(iterator& it) {
			// invalidates this iterator
			if (it == this->end()) {
				// cannot erase header iterator itself
				return;
			}
			_splaytree_algo::erase(&this->_header, it.getNode());
			_size--;
			// deallocate the space allocated for node
			if (it.getNode()) delete it.getNode();
		}
		bool erase(const KeyType& key) {
			// first locate the node for this key if it exists
			_node_ptr to_be_erased_node;
			to_be_erased_node = _splaytree_algo::find(&this->_header, key, _cmp);
			if (to_be_erased_node == &this->_header) {
				SPLAYTREE_DEBUG("Node to be erased wasn't found!");
			} else {
				SPLAYTREE_DEBUG("Node to be erased has been found! About to erase it");
				_splaytree_algo::erase(&this->_header, to_be_erased_node);
				_size--;
				// deallocate space allocated for node
				if (to_be_erased_node) delete to_be_erased_node;
				return true;
			}
			return false;
		}
		iterator lowerBound(const KeyType& key) {
			return iterator(_splaytree_algo::lower_bound(&this->_header, key, _cmp));
		}
		iterator upperBound(const KeyType& key) {
			return iterator(_splaytree_algo::upper_bound(&this->_header, key, _cmp));
		}
		iterator find(const KeyType& key) {
			// first locate the node for this key if it exists
			_node_ptr found_node;
			found_node = _splaytree_algo::find(&this->_header, key, _cmp);
			if (found_node == &this->_header) {
				SPLAYTREE_DEBUG("Node was not found!");
				return this->end();
			}
			SPLAYTREE_DEBUG("Node was found!");
			return iterator(found_node);
			//return *(_link_type::_dummy_val);
		}
		MappedType& operator [](const KeyType& key) {
			_insert_commit_data tmp_insert_commit_data;
			_node_ptr new_node; bool not_present;
			std::tie(new_node, not_present) = _splaytree_algo::insert_unique_check(
				&this->_header, key, this->_cmp, tmp_insert_commit_data);
			if (not_present) {
				// key not present, create a node with this key and return the reference
				SPLAYTREE_DEBUG("Key not found. Creating a node with this key!");
				new_node = new _node(std::piecewise_construct,
					    			std::tuple<const KeyType&>(key),
					    			std::tuple<>());
				_bstree_algo::insert_unique_commit(&this->_header, new_node, tmp_insert_commit_data);
				_size++;
			} else {
				SPLAYTREE_DEBUG("Key found, returning the reference to the value");
			}
			if (_size > _max_size) prune();
			return new_node->_val.second;
		}
		void clear() {
			// clears all the nodes in the tree and reinitializes header
			_splaytree_algo::clear(&this->_header);
			_size = 0;
		}
};

#endif	/* SPLAYTREE_H */
/*
int main() {
	SplayTree<double, int> st;
	st[1.2] = 4;
	st[2.3] = 16;
	st[3.4] = 64;
	st[4.5] = 256;
	st.find(1.0);
	st.erase(2.3);
	st[2.3] = 3;
	st.find(1.0);
	st.find(2.0);
	SplayTree<double, int>::iterator it = st.end();
	//uncomment the following code to test consistency of iterator
	//it._iter_node = NULL;
	MD_LOG_INFO << it->first << ' ' << it->second << "\n";
	for(SplayTree<double, int>::reverse_iterator rit = st.rbegin(); rit != st.rend(); rit++) {
		MD_LOG_INFO << rit->first << ' ' << rit->second << "\n";
	}
	return 0;
}
*/
