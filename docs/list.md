# list

## Introduction

In our implementation, a list can either be sized or unsized.

A **sized list**, similar to that of std::list, will keep the size information of the list. Consequently, we can get the size of the list in constant time. It may bring many troubles when we want to perform splice operation on the list.

An unsized list, or **raw_list**, will not save the size information of the list. Consequently, we cannot get the size of the list in constant time. However, it will be much easier to perform splice operation on the list.

Both lists are derived from the list_trait class, sharing the same node type and iterator type. We use CRTP to reduce the code duplication while brings no runtime overhead.

In order to keep the size information while providing more support for splice operation, we design much interface to interact between list and raw_list.

## sized_list(default as list)

A list contains only a sized node as its data member. (2 pointers and 1 size_type variable)

## unsized_list(default as raw_list)

A raw_list contains only a normal list node as its data member. (2 pointers)


