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
    [[no_unique_address]] _Compare_t comp;

    using _Base_t = __detail::__tree::ordered_tree;
    using _Node_t = __detail::__tree::value_node <_Pair_t>;
    using _Alloc_t = allocator <_Node_t>;


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

    void insert_root(_Node_t *__restrict __node) {
        return __detail::__tree::init_root(__node, &this->header);
    }

  public:
    map() = default;

    void insert(const _Pair_t &__p) {
        if (!this->has_root())
            return insert_root(this->create_node(__p));
        auto [__node, __from, __found] =
            locate_key <_View_t> (this->root(), _View_t::key(__p), this->comp);
        if (__found) return;

        auto __new_node = this->create_node(__p);
        __detail::__tree::init_node(__new_node);
        __detail::__tree::link_child{__from}(__node, __new_node);
        __detail::__tree::insert_at(__new_node);
    }

    bool erase(const _Key_t &__key) {
        if (!this->has_root()) return false;
        auto [__node, __from, __found] =
            locate_key <_View_t> (this->root(), __key, this->comp);
        if (!__found) return false;

        __detail::__tree::erase_at(__node);
        this->destroy_node(static_cast <_Node_t*> (__node));
        return true;
    }

    _Node_t *find(const _Key_t &__key) {
        if (!this->has_root()) return nullptr;
        auto [__node, __from, __found] =
            locate_key <_View_t> (this->root(), __key, this->comp);
        if (!__found) return nullptr;
        return static_cast <_Node_t*> (__node);
    }

    /* An inner debug method. */
    void debug() const { __detail::__tree::debug(&this->header); }
};





} // namespace dark
