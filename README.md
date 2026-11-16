# MCC - Mini Compiler Collection

The stupid Compiler Collection.

MCC is a C-based compiler collection targeting 32-bit and 64-bit x86 Linux ELF.

## Current status

The first milestone implements a native ELF executable writer. It can generate tiny
Linux executables for i386 and x86-64 that exit with status 0. This is an ELF backend
smoke test, not yet a C compiler: parsing, semantic analysis, machine-code generation
from C, object files, and linking are future milestones.

MCC also includes the beginnings of **mcclib**, a small freestanding C library.

## Build requirements

- GCC or another C11 compiler
- GNU Make
- GNU binutils (for `ar`, used to build the library archive)

## Build

```sh
make
```

This builds the `mcc` driver and `libmcclib.a`.

## Generate an ELF executable

```sh
./mcc --target x86_64 -o hello-x86_64.elf
./mcc --target i386 -o hello-i386.elf
```

The generated programs are minimal Linux executables that return exit status 0.
They do not compile source code yet.

## Build mcclib for a target

```sh
make clean
make TARGET_ARCH=i386
```

Supported `TARGET_ARCH` values are `x86_64` and `i386`. The library is freestanding
and does not depend on the host C library.

## Project layout

- `src/`: MCC compiler driver and ELF writer
- `include/mcc/`: MCC internal/public interfaces
- `libc/include/`: mcclib headers
- `libc/src/`: mcclib implementation

## License

MCC is licensed under the MIT License. See [LICENSE](LICENSE).
