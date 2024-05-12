Range-based `for` loop for C++03
================================

This directory contains a macro library that provides a fairly close
approximation to the *range-based* `for` loop that was introduced in C++11 and
modified in C++17, C++20, and C++23.  The functionality of the macro attempts
to get as close as possible to C++20 semantics but does not support an
initialization statement, which was added in C++23.

Supported:
----------

* Standard containers
* C-style arrays
* `const` ranges
* Proxy iterators

Limitations:
------------

* The range's `end()` function is called each time through the loop instead of
  once at the start.
* Range must be an lvalue (but can be const), unless the `copyable_range`
  trait is true.
* No emulation of newer C++ features commonly used in range-for loops, such as
  `auto` variables and structured bindings
* No support for C++20 lifetime extension of subparts of range expression
* No support for C++23 initialization clause

Future enhancements:
--------------------

* Support for range-like classes that lack nested `iterator` types.
* Call `end()` only once, before the first loop iteration, instead of before
  each loop iteration.

Syntax:
-------

> `RANGE_FOR(` *variable-decl*`,` *range-expr* `)`
>    *statement*

Examples:
---------

### Print the contents of a `std::vector`:

```
std::vector<int> v = { ... };
RANGE_FOR(int i, v)
  std::cout << i << '\n';
```

C++20 equivalent:


```
std::vector<int> v;
// ...
for (int i : v)
  std::cout << i << '\n';
```

### Double each element in a `vector<int>`:

```
std::vector<int> v;
// ...
RANGE_FOR(int& i, v)
  i *= 2;
```

### Print each element in a `std::map<int, const char*>`:

If the loop variable contains a comma such that it would be seen as two or more
macro arguments, it's definition must be enclosed in parentheses:

```
std::map<int const char*> m;
// ...
RANGE_FOR((const std::map<int const char*>::value_type& thePair), m)
{
  std::cout << '(' << thePair.first << ", " << thePair.second << ")\n";
}
```
