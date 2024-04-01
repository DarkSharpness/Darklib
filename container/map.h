#pragma once
#include "tree.h"
#include "allocator.h"

namespace dark {

namespace __detail::__map {

template <typename _Key_t, typename _Value_t>
struct pair {
    const _Key_t  key;
    _Value_t      value;
};


} // namespace __detail::__map

using _Key_t    = int;
using _Val_t    = int;
using _Pair_t   = __detail::__map::pair <_Key_t, _Val_t>;
using _View_t   = __detail::__tree::pair_key <_Pair_t>;
using _Compare_t = std::compare_three_way;

struct map : __detail::__tree::ordered_tree {
  protected:
    using _Base_t = __detail::__tree::ordered_tree;
    using _Node_t = __detail::__tree::value_node <_Pair_t>;
    using _Alloc_t = allocator <_Node_t>;
    using enum __detail::__tree::Direction;

    [[no_unique_address]] _Compare_t comp;
  public:
    using iterator          = __detail::__tree::iterator <_Pair_t ,RT, 0>;
    using const_iterator    = __detail::__tree::iterator <_Pair_t, LT, 1>;
    using insert_result_t   = struct { iterator iter; bool success; };
  protected:
    template <typename ..._Args>
    _Node_t *create_node(_Args &&...__args) {
        static int cnt = 0;
        auto *__node = _Alloc_t::allocate(1);
        __node->size = ++cnt;
        std::construct_at(&__node->value, std::forward <_Args> (__args)...);
        return __node;
    }

    void destroy_node(_Node_t *__node) {
        std::destroy_at(__node);
        _Alloc_t::deallocate(__node, 1);
    }

    insert_result_t insert_root(_Node_t *__restrict __node) {
        __detail::__tree::init_root(__node, &this->header);
        return { iterator{__node}, true };
    }

    _Pair_t &get_max() const { return static_cast <_Node_t *> (this->max_node())->value; }
    _Pair_t &get_min() const { return static_cast <_Node_t *> (this->min_node())->value; }

  public:
    map() = default;

    insert_result_t insert(const _Pair_t &__p) {
        if (!this->has_root())
            return insert_root(this->create_node(__p));
        const auto [__node, __from, __found] =
            locate_key <_View_t> (this->root(), _View_t::key(__p), this->comp);
        if (__found) return { iterator{__node}, false };
        auto __new_node = this->create_node(__p);
        this->insert_impl(__node, __from, __new_node);
        return { iterator{__new_node}, true };
    }

    bool erase(const _Key_t &__key) {
        if (!this->has_root()) return false;
        const auto [__node, __from, __found] =
            locate_key <_View_t> (this->root(), __key, this->comp);
        if (!__found) return false;
        this->erase_impl(__node);
        this->destroy_node(static_cast <_Node_t*> (__node));
        return true;
    }

    _Node_t *find(const _Key_t &__key) {
        if (!this->has_root()) return nullptr;
        const auto [__node, __from, __found] =
            locate_key <_View_t> (this->root(), __key, this->comp);
        if (!__found) return nullptr;
        return static_cast <_Node_t*> (__node);
    }

    /* An inner debug method. */
    void debug() const { __detail::__tree::debug(&this->header); }

    iterator begin() { return iterator {this->min_node()}; }
    iterator end()   { return iterator {&this->header}; }

    const_iterator begin()  const { return const_iterator {this->min_node()}; }
    const_iterator end()    const { return const_iterator {&this->header}; }
    const_iterator cbegin() const { return this->begin(); }
    const_iterator cend()   const { return this->end(); }

    _Pair_t &max() { return this->get_max(); }
    _Pair_t &min() { return this->get_min(); }
    const _Pair_t &max() const { return this->get_max(); }
    const _Pair_t &min() const { return this->get_min(); }
};



} // namespace dark
