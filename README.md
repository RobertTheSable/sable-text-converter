# Sable Text Converter

Extensible text converter for Fire Emblem: Monshou no Nazo.

Originally developed for the updated FE3 English patch: https://www.romhacking.net/translations/4969/

Features:
* Text mapping is fully user customizeable.
* Supports multiple formats of text conversion.
* Text files can be converted individually or as part of a table.
* Integrates Asar assembler so your text and other customizations can be inserted at the same time
* UTF-8 support.

Compiling:
* Requires CMake 
* Requires c++17 Filesystem or Boost filesystem libraries.
* Tested compilers: g++, clang++ (Linux), mingw-w64.

Simply clone the repo or download the source and run `cmake .`
