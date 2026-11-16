# MCC - Mini Compiler Collection

The stupid Compiler Collection.

MCC is a C-based compiler collection targeting 32-bit and 64-bit x86 Linux ELF.

## Tools and libraries

- `mcc`: compiler driver and initial ELF executable smoke-test writer
- `mcc-as`: small x86 assembler that emits ELF relocatable objects
- `mcc-ld`: small linker for the initial MCC object format subset
- `mcc-cpp`: C preprocessor with object-like macros, includes, and conditional directives
- `libmcclib.a`: initial freestanding C library subset
- `mcrt0.o`: minimal C runtime entry object for i386 or x86-64

## Build

Requirements: GCC or another C11 compiler, GNU Make, and GNU binutils (`ar`).

```sh
make -j3
```

Build mcclib and the startup object for a different target ABI with:

```sh
make clean
make -j3 TARGET_ARCH=i386
```

The host tools themselves are built for the host machine. The target-specific
library and startup object are built for `TARGET_ARCH`.

## mcclib

The current freestanding library includes basic string and integer conversion
routines, plus `printf` and `vprintf`.

The initial formatted-output conversions are:

- `%s`, `%c`
- `%d`, `%i`, `%u`
- `%x`, `%X`, `%o`
- `%p`, `%%`
- `%ld`, `%lld`, `%zd` and corresponding unsigned forms

`printf` writes to Linux file descriptor 1 using the target's `write` syscall.
It uses a small internal buffer. This is not yet a hosted stdio implementation:
there is no `FILE`, `fopen`, `fclose`, `fprintf`, or stream buffering API.

## C runtime startup

`mcrt0.o` defines the ELF entry point `_start`. It reads the initial Linux
process stack, passes `argc`, `argv`, and `envp` to `main`, and exits using
the Linux `exit` syscall with the return value from `main`.

The startup object is built for the selected `TARGET_ARCH`. It is deliberately
minimal: it does not run constructors, initialize TLS, or provide `atexit`.
The current `mcc-ld` cannot yet link multiple objects or resolve symbols, so
use a system linker for programs that need `mcrt0.o` and `libmcclib.a`.

For example, after compiling an application object for the same target:

```sh
gcc -m64 -ffreestanding -fno-builtin -fno-pie -Ilibc/include -c hello.c -o hello.o
gcc -m64 -nostdlib -static -no-pie -Wl,-e,_start -o hello mcrt0.o hello.o libmcclib.a -lgcc
```

Use `-m32` instead of `-m64` for an i386 build, and build the library and
startup object with `TARGET_ARCH=i386` first.

## Assembler and linker

The initial assembler supports `nop`, `ret`, `syscall` (x86-64), `int 0x80`,
a few register-immediate and xor instructions, and `.byte`. It accepts a small
Intel-syntax-like source subset and emits ELF32 or ELF64 `ET_REL` files.

```sh
./mcc-as --target x86_64 -o start.o start.s
./mcc-ld --target x86_64 -o program.elf start.o
```

The initial linker accepts one MCC-generated object, links its `.text` section,
and uses the beginning of that section as the entry point. It does not yet process
symbols, relocations, multiple input objects, data sections, or dynamic linking.

## Preprocessor

```sh
./mcc-cpp -DDEBUG=1 -Iinclude source.c
```

The initial preprocessor supports object-like `#define` and `#undef`, `#include`,
`#ifdef`, `#ifndef`, `#else`, `#endif`, and `#error`. Function-like macros,
expression evaluation in `#if`, and full standard conformance are not implemented yet.

## ELF writer

`mcc` can generate minimal Linux ELF executables for i386 and x86-64:

```sh
./mcc --target x86_64 -o hello.elf
./mcc --target i386 -o hello32.elf
```

These smoke-test programs exit with status 0. MCC is not yet a C compiler; the
C frontend and full code-generation pipeline remain future work.

## Project layout

- `src/`: compiler driver and ELF writer
- `src/as/`, `src/ld/`, `src/cpp/`: tool implementations
- `include/mcc/`: MCC interfaces
- `libc/include/`, `libc/src/`: mcclib headers and implementation
- `libc/crt/`: C runtime startup sources

## License

MIT. See [LICENSE](LICENSE).
