# divulge
[![CMake](https://github.com/grzegorz-grzeda/divulge/actions/workflows/cmake.yml/badge.svg)](https://github.com/grzegorz-grzeda/divulge/actions/workflows/cmake.yml)

Small HTTP router in C.

This is a [G2EPM](https://github.com/grzegorz-grzeda/g2epm) library.

## How it works?
TBD

## Initialize
Download dependencies by running `g2epm download` in the project root.

## Run tests
Just run `./test.sh`.

## How to compile and link it?

Example `CMakeLists.txt` content:
```
...

add_subdirectory(<PATH TO DIVULGE LIBRARY>)
target_link_libraries(${PROJECT_NAME} PRIVATE divulge)

...
```

## Documentation
If you want, you can run `doxygen` to generate HTML documentation. It will be available in `documentation` 
directory.


# Copyright
This library was written by Grzegorz Grzęda, and is distributed under MIT Licence. Check the `LICENSE` file for
more details.