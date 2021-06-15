# Installing Skybison

This guide details the recommended Linux setup.

## Preparation

Install CMake and either Make or Ninja. On Fedora, this can be done with:

```sh
$ sudo dnf install cmake ninja-build
```

## Build preparation (cmake)

```sh
$ git clone https://github.com/facebookexperimental/skybison
$ cd skybison
$ mkdir build
$ cmake -B build -GNinja -DCMAKE_TOOLCHAIN_FILE=util/linux.cmake
# or, with Make
$ cmake -B build -GMake -DCMAKE_TOOLCHAIN_FILE=util/linux.cmake
```

## Build

```sh
$ ninja -C build
# or, with Make
$ make -C build
```

## Enjoy!

```sh
$ ./build/bin/python
Python 3.7.4+ (default)
[GCC Clang 9.0.20190721 ] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> print("Hello " + "world!")
Hello world!
>>> import sys
>>> sys.implementation.name
'skybison'
```

---

## Build modes

You can change build type by adding the one of the following to the `cmake` command:

```
-DCMAKE_BUILD_TYPE=Release  // default
-DCMAKE_BUILD_TYPE=Debug    // -g
-DCMAKE_BUILD_TYPE=DebugOpt // -O2 -g with debug checks but faster
-DCMAKE_BUILD_TYPE=RelWithDebInfo // like a release build but has debug symbols -- useful for callgrind
```
To run tests with address sanitizer (ASAN), add

```
-DSKYBISON_ASAN=1
```

*before* the platform. Like `cmake -DSKYBISON_ASAN=1 ...`

Since you will likely have several `build-XYZ` folders hanging around, and it's
useful for those not to show up in `git status`, they should be ignored by
default in `.gitignore`.
