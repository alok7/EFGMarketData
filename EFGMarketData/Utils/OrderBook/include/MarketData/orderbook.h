#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <MarketData/splaytree.h>
#include <MarketData/compile_config.h>
#include <iostream>
#include <mutex>

namespace EFG {
namespace MarketData {

  enum order_book_side{ask, bid, undef};
  template<class KeyType, class NormalizedMsgType>
  class OrderBook 
  {
    public:
      typedef typename NormalizedMsgType::price_level ValType;
      typedef _splay_tree_iterator<std::pair<const KeyType, ValType>> iterator;
      typedef _splay_tree_const_iterator<std::pair<const KeyType, ValType>> const_iterator;
    private:
      std::mutex _lock;
      SplayTree<KeyType, ValType, std::less<KeyType>> _asks;
      SplayTree<KeyType, ValType, std::greater<KeyType>> _bids;
      template<typename _container_type>
      void _insert(_container_type& _container, const KeyType& _key, const ValType& _val, iterator hint) {
        _container.insert(_key, _val, hint);
        // _container[_key] = _val;
      }
      template<typename _container_type>
      void _update(_container_type& _container, const KeyType& _key, const ValType& _val) {
        _container[_key] = _val;
      }
      template<typename _container_type>
      bool _remove(_container_type& _container, const KeyType& _key) {
        return _container.erase(_key);
      }
      template<typename _container_type>
      void _remove(_container_type& _container, iterator _it) {
        _container.erase(_it);
      }
      template<typename _container_type, typename DeltaType>
      void _sort(_container_type& _in_container, std::vector<ValType>& _out_container, DeltaType& delta) {
        uint i = 0;
        for(auto it = _in_container.begin(); it != _in_container.end() && i < ORDERBOOK_DEPTH; it++) {
          auto it2 = delta.find(it->first);
          if(it2!=delta.end()) // delete, modify and insert price levels
          {
            _out_container.push_back(it->second);
          }
          else
          {
            _out_container.push_back(ValType((it->second).price,(it->second).qty, (it->second).orders_count)); // unchanged price levels
          } 
          i++;
        }
      }
      template<typename _container_type, typename _out_type>
      void _sort(_container_type& _in_container, _out_type& _out_container) {
        uint i = 0;
        for(auto it = _in_container.begin(); it != _in_container.end() && i < ORDERBOOK_DEPTH; it++) {
          _out_container[i++] = it->second;
        }
      }
      template<typename _container_type>
      iterator _find(_container_type& _container, const KeyType& _key) {
        typename _container_type::iterator it = _container.find(_key);
        if (it == _container.end()) {
          // key not found
        }
        // key was found
        return it;
      }
      template<typename _container_type>
      void _clear(_container_type& _container) {
        _container.clear();
      }
                
    public:
      OrderBook() {}
      inline auto& getLock() {return _lock;}
      inline void lock() {_lock.lock();}
      inline void unlock() {_lock.unlock();}
      void insert(order_book_side side, const KeyType& key, const ValType& val) {
        switch (side) {
          case order_book_side::ask: this->_insert(this->_asks, key, val, _asks.end()); break;
          case order_book_side::bid: this->_insert(this->_bids, key, val, _bids.end()); break;
          default: MD_LOG_WARN << "Error: invalid order type for insert!" << "\n";
        }
      }
      void insert(order_book_side side, const KeyType& key, const ValType& val, iterator hint) {
        // it is suggested that even for an update, a call to insert would do the job
        // inserts a key with the mapping val, if the key exists, then updates it
        switch (side) {
          case order_book_side::ask: this->_insert(this->_asks, key, val, hint); break;
          case order_book_side::bid: this->_insert(this->_bids, key, val, hint); break;
          default: MD_LOG_WARN << "Error: invalid order type for insert!" << "\n";
        }
      }
      void update(order_book_side side, const KeyType& key, const ValType& val) {
        switch (side) {
          case order_book_side::ask: this->_update(this->_asks, key, val); break;
          case order_book_side::bid: this->_update(this->_bids, key, val); break;
          default: MD_LOG_WARN << "Error: invalid order type for update!" << "\n";
        }
      }
      bool remove(order_book_side side, const KeyType& key) {
        // returns true if the key was erased, false if the key was not found
        switch(side) {
          case order_book_side::ask: return this->_remove(this->_asks, key);
          case order_book_side::bid: return this->_remove(this->_bids, key);
          default: MD_LOG_WARN << "Error: invalid order type for remove!" << "\n"; return false;
        }
      }
      void remove(order_book_side side, iterator& it) {
        switch(side) {
          case order_book_side::ask: this->_remove(this->_asks, it); break;
          case order_book_side::bid: this->_remove(this->_bids, it); break;
          default: MD_LOG_WARN << "Error: invalid order type for remove!" << "\n";
        }
      }
      void clear() {
        this->_clear(this->_asks);
        this->_clear(this->_bids);
      }
      template<typename DeltaType>
      void getAsks(std::vector<ValType>& out, DeltaType& delta) {
        this->_sort(this->_asks, out, delta);
      }
      template<typename DeltaType>
      void getBids(std::vector<ValType>& out, DeltaType& delta) {
        this->_sort(this->_bids, out, delta);
      }
      template<typename T>
      void getAsks(T& out) {
        this->_sort(this->_asks, out);
      }
      template<typename T>
      void getBids(T& out) {
        this->_sort(this->_bids, out);
      }
      iterator find(order_book_side side, const KeyType& key) {
        switch(side) {
          case order_book_side::ask: return this->_asks.find(key); break;
          case order_book_side::bid: return this->_bids.find(key); break;
        }
        return {};
      }
      iterator lowerBound(order_book_side side, const KeyType& key) {
        switch(side) {
          case order_book_side::ask: return this->_asks.lowerBound(key); break;
          case order_book_side::bid: return this->_bids.lowerBound(key); break;
        }
        return {};
      }
      iterator begin(order_book_side side) {
        switch(side) {
          case order_book_side::ask: return this->_asks.begin(); break;
          case order_book_side::bid: return this->_bids.begin(); break;
        }
        return {};
      }
      iterator end(order_book_side side) {
        switch(side) {
          case order_book_side::ask: return this->_asks.end(); break;
          case order_book_side::bid: return this->_bids.end(); break;
        }
        return {};
      }
      bool exists(order_book_side side, const KeyType& key) {
        switch(side) {
          case order_book_side::ask: return this->_asks.find(key)!=_asks.end(); break;
          case order_book_side::bid: return this->_bids.find(key)!=_bids.end(); break;
        }
        return false;
      }
  };
}
}

#endif
