# MCC - Mini Compiler Collection

The stupid Compiler Collection.

MCC is a C-based compiler collection targeting 32-bit and 64-bit x86 Linux ELF.

## Tools

- `mcc`: compiler driver and initial ELF executable smoke-test writer
- `mcc-as`: small x86 assembler that emits ELF relocatable objects
- `mcc-ld`: small linker for the initial MCC object format subset
- `mcc-cpp`: C preprocessor with object-like macros, includes, and conditional directives
- `libmcclib.a`: initial freestanding C standard-library subset

## Build

Requirements: GCC or another C11 compiler, GNU Make, and GNU binutils (`ar`).

```sh
make
```

Build mcclib for a different target ABI with `make clean && make TARGET_ARCH=i386`.
The host tools themselves are built for the host machine.

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

## License

MIT. See [LICENSE](LICENSE).
