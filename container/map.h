#pragma once
#include "tree.h"
#include "allocator.h"
#include <compare>
#include <memory>
#include <utility>

namespace dark {

namespace __detail::__map {

template <typename _Key_t, typename _Value_t>
struct pair {
    const _Key_t  key;
    _Value_t      value;
};


} // namespace __detail::__map

template <
    typename _Key_t,
    typename _Val_t,
    typename _Compare_t = std::compare_three_way,
    typename _Pair_t    = __detail::__map::pair <_Key_t, _Val_t>,
    typename _View_t    = __detail::__tree::pair_view <_Pair_t>
>
struct map : protected __detail::__tree::ordered_tree {
  protected:
    using _Base_t = __detail::__tree::ordered_tree;
    using _Node_t = __detail::__tree::value_node <_Pair_t>;
    using _Alloc_t = allocator <_Node_t>;

    using _Base_Node_t = __detail::__tree::node;
    using enum __detail::__tree::Direction;

    [[no_unique_address]] _Compare_t comp;
  public:
    using iterator          = __detail::__tree::iterator <_Pair_t ,RT, 0>;
    using const_iterator    = __detail::__tree::iterator <_Pair_t, RT, 1>;
    using insert_result_t   = struct { iterator iter; bool success; };
    using value_type        = _Pair_t;

  protected:
    /* Create a new node. */
    template <typename ..._Args>
    _Node_t *create_node(_Args &&...__args) {
        auto __node = _Alloc_t::allocate(1);
        std::construct_at(
            std::addressof(__node->data),
            std::forward <_Args> (__args)...
        );
        return __node;
    }

    /* Make a copy of target node. */
    _Node_t *create_copy(const _Base_Node_t *__copy) {
        auto __cast = static_cast <const _Node_t *> (__copy);
        auto __node = _Alloc_t::allocate(1);
        std::construct_at(std::addressof(__node->data), __cast->data);
        __node->info = __copy->info;
        return __node;
    }

    /* Destroy a node. */
    void destroy_node(_Node_t *__node) {
        std::destroy_at(__node);
        _Alloc_t::deallocate(__node, 1);
    }

    insert_result_t insert_root(_Node_t *__restrict __node) {
        init_root(__node, this->end_node());
        return { iterator{__node}, true };
    }

    _Pair_t &get_max() const { return static_cast <_Node_t *> (this->max_node())->data; }
    _Pair_t &get_min() const { return static_cast <_Node_t *> (this->min_node())->data; }

    template <__detail::__tree::Direction _Dir>
    void copy(_Base_Node_t *__from, const _Base_Node_t *__copy) {
        using __detail::__tree::link_child;
        if (__copy == nullptr)
            return void(__from->child[_Dir] = nullptr);
        auto __node = this->create_copy(__copy);
        link_child <_Dir> (__from, __node);
        this->copy <LT> (__node, __copy->child[LT]);
        this->copy <RT> (__node, __copy->child[RT]);
    }

    /* Copy from a root node. */
    void copy(const _Base_Node_t *__copy) {
        auto __root = this->create_copy(__copy);
        this->copy <LT> (__root, __copy->child[LT]);
        this->copy <RT> (__root, __copy->child[RT]);
        return this->update_root(__root);
    }

    /* Remove a subtree. */
    void remove(_Base_Node_t *__node) {
        if (__node == nullptr) return;
        this->remove(__node->child[LT]);
        this->remove(__node->child[RT]);
        this->destroy_node(static_cast <_Node_t *> (__node));
    }

  public:
    map() = default;
    map(const map &__other) : ordered_tree(__other) {
        if (!__other.has_root()) this->reset();
        else        this->copy(__other.root());
    }
    map(map &&__other) = default;
    ~map() { if (has_root()) this->remove(this->root()); }

    map &operator = (const map &__other) {
        if (this != std::addressof(__other)) {
            this->~map();
            std::construct_at(this, __other);
        } return *this;
    }

    map &operator = (map &&__other) {
        if (this != std::addressof(__other)) {
            this->~map();
            std::construct_at(this, std::move(__other));
        } return *this;
    }

    insert_result_t insert(const _Pair_t &__p) {
        if (!this->has_root())
            return insert_root(this->create_node(__p));
        const auto [__node, __from, __found] =
            locate_key <_View_t> (this->root(), _View_t::key(__p), this->comp);
        if (__found) return { iterator{__node}, false };
        auto __new_node = this->create_node(__p);
        this->insert_aux(__node, __from, __new_node);
        return { iterator{__new_node}, true };
    }

    bool erase(const _Key_t &__key) {
        if (!this->has_root()) return false;
        const auto [__node, __from, __found] =
            locate_key <_View_t> (this->root(), __key, this->comp);
        if (!__found) return false;
        this->erase_aux(__node);
        this->destroy_node(static_cast <_Node_t*> (__node));
        return true;
    }

    iterator erase(iterator __iter) {
        auto __node = static_cast <_Node_t *> (__iter.base());
        auto __next = ++__iter;
        this->erase_aux(__node);
        this->destroy_node(__node);
        return __next;
    }

    const_iterator find(const _Key_t &__key) const {
        if (!this->has_root()) return this->end();
        const auto [__node, __from, __found] =
            locate_key <_View_t> (this->root(), __key, this->comp);
        if (!__found) return this->end();
        return const_iterator {__node};
    }

    iterator find(const _Key_t &__key) {
        return std::as_const(*this).find(__key).de_const();
    }

    iterator begin() { return iterator {this->min_node()}; }
    iterator end()   { return iterator {this->end_node()}; }

    const_iterator begin()  const { return const_iterator {this->min_node()}; }
    const_iterator end()    const { return const_iterator {this->end_node()}; }
    const_iterator cbegin() const { return this->begin();   }
    const_iterator cend()   const { return this->end();     }

    _Pair_t &max() { return this->get_max(); }
    _Pair_t &min() { return this->get_min(); }
    const _Pair_t &max() const { return this->get_max(); }
    const _Pair_t &min() const { return this->get_min(); }

    void clear() noexcept {
        if (has_root()) {
            this->remove(this->root());
            this->reset();
        }
    }

    /* An inner debug method. */
    void debug() const { __detail::__tree::debug(&this->header); }

    using _Base_t::size;
    using _Base_t::empty;
};



} // namespace dark
