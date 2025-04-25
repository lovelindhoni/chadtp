# chadtp - bare minimal static-site server written in C

> [!NOTE]
> This is a toy implementation

## Build

### Dependencies

- Cmake
- GCC or Clang

### Clone and Build

```bash
git clone https://github.com/lovelindhoni/chadtp
```

```bash
cd chadtp
mkdir build
cd build
```

```bash
cmake ..
cmake --build .
```

### Instructions

Executable `chadtp` would be built

you can specify the directory in which site files are present

```bash
chadtp www
chadtp dist/
```

The default argument is `/` which is relevant to the current directory where `chadtp` is present

## License

MIT
