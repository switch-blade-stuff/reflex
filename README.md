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
    <td>REFLEX_NO_ARITHMETIC_CONV</td>
    <td>-DREFLEX_NO_ARITHMETIC_CONV</td>
    <td>OFF</td>
    <td>Disables generation of default conversions & constructors for arithmetic types (other than `double` & `[u]intmax_t`)</td>
  </tr>
  <tr>
    <td>REFLEX_HEADER_ONLY</td>
    <td>-DREFLEX_HEADER_ONLY</td>
    <td>ON</td>
    <td>Switches the library to header-only mode and toggles build of the `reflex-interface` CMake target</td>
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