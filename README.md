# Sable Text Converter

[![codecov](https://codecov.io/gh/RobertTheSable/sable-text-converter/branch/master/graph/badge.svg)](https://codecov.io/gh/RobertTheSable/sable-text-converter)

Extensible text converter for Fire Emblem: Monshou no Nazo.

Originally developed for the updated FE3 English patch: https://www.romhacking.net/translations/4969/

## Features:
* Text mapping is fully user customizeable.
* Supports multiple formats of text conversion.
* Text files can be converted individually or as part of a table.
* Integrates the Asar assembler so your text and other customizations can be inserted at the same time.
* UTF-8 support.
* Code page switching.

## Usage

Run from the terminal/command prompt. Either in in the root of your text project, or pass the path of your project via an argument:

```sable.exe --project C:\\Users\\You\\Your-Project\\```

Use `sable.exe --help` for more detailed options.

## Compiling:
* Requires CMake 
* Requires c++17 Filesystem.
* Requires ICU (il8n, uc, and data)
* Tested compilers: g++, clang (Linux), clang-cl, mingw-w64.

Simply clone the repo or download the source and run 
* `conan install .`
* `cd build`
* `cmake .. -G Ninja <include toolchain/presets etc>`
* `cmake --build .`

## Coverage

* Run cmake with CODE_COVERAGE on.
* Run tests.
* Run the coverage tool from inside the build folder.

### gcovr
```bash
gcovr -r ../src .
```

### lcov
```bash
lcov --directory . -b ../src --capture --output-file coverage.info
lcov --remove coverage.info '/usr/*' "*${HOME}*" '*/.cache/*' '*tests/*' '*asardll*' --output-file coverage.info
lcov --list coverage.info
```
