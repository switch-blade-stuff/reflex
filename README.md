# Reflection EX

Runtime facet-based reflection system for C++20 and beyond.
The library provides the following features:

* Compile-time type name generation
* Dynamic objects and dynamic-typing containers (`reflex::any`, `reflex::object`)
* Dynamic inheritance, type conversions & factories
* Facet-based type erasure (duck-typing et al)
* Enum serialization

## Build

**reflex** can be used as a header-only library (see options), or as a statically and/or dynamically compiled binary.
In order to build the library as a static or dynamic target, a compiler capable of C++20 is required (with a GCC/Clang
or MSVC compatible syntax) and CMake with minimal version 3.20.

## Library options

<table>
  <tr><th>#define macro</th><th>CMake option</th><th>Default value</th><th>Description</th></tr>
  <tr>
    <td>REFLEX_NO_ARITHMETIC</td>
    <td>-DREFLEX_NO_ARITHMETIC</td>
    <td>OFF</td>
    <td>Disables generation of default conversions & constructors for arithmetic types</td>
  </tr>
  <tr>
    <td>REFLEX_HEADER_ONLY</td>
    <td>-DREFLEX_HEADER_ONLY</td>
    <td>ON</td>
    <td>Switches the library to header-only mode and toggles build of the `reflex-header-only` CMake target</td>
  </tr>
  <tr>
    <td>N/A</td>
    <td>-DREFLEX_BUILD_SHARED</td>
    <td>ON</td>
    <td>Toggles build of the `reflex-shared` CMake target</td>
  </tr>
  <tr>
    <td>N/A</td>
    <td>-DREFLEX_BUILD_STATIC</td>
    <td>ON</td>
    <td>Toggles build of the `reflex-static` CMake target</td>
  </tr>
  <tr>
    <td>N/A</td>
    <td>-DREFLEX_TESTS</td>
    <td>OFF</td>
    <td>Toggles build of the unit test target</td>
  </tr>
</table>

## Default arithmetic conversions

Unless `REFLEX_NO_ARITHMETIC` is defined, conversions, factories and comparisons for builtin arithmetic types such
as `int`, `char`, `float`, etc. are generated by default. This makes conversions & comparison between dynamic
objects (`reflex::any`) of different arithmetic types possible, but does generate quite a bit (as much as 500-600KiB
after stripping symbols!) extra code and data required for the dynamic type system to function. If binary size is at a
premium, defining `REFLEX_NO_ARITHMETIC` is recommended. Note that if `REFLEX_NO_ARITHMETIC` is defined, any
dynamic-typing functionality for builtin arithmetic types must be reflected by the end-user.

As an example, the following code will only work if default arithmetic conversions are enabled,
or if the types are explicitly reflected as constructible, convertible & comparable.

```cpp
const auto a0 = reflex::make_any<std::int32_t>(0);
const auto a1 = reflex::make_any<std::int64_t>(0);

a0.type().construct(a1);    // Construction from a different value type
a0.conv(a1.type());         // Conversion to a different value type
a0 == a1;                   // Comparison of different value types
```