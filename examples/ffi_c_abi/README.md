# Direct C-ABI FFI Examples (Tier 2 & Tier 3)

This directory showcases **xlang's Direct C-ABI Foreign Function Interface (FFI)**, enabling zero-wrapper interaction with existing dynamic system libraries (`.so`, `.dylib`, `.dll`) without writing glue code, C wrappers, or recompiling.

---

## 1. Overview of the 3-Tier Native Integration Model

| Tier | Name | Mechanism | Primary Use Case |
| :--- | :--- | :--- | :--- |
| **Tier 1** | **Vectorcall Extension SDK** | `XVectorFn`, `include/xlang.h` | High-performance C extensions with rich VM interaction, object allocation, and custom classes (e.g. Raylib engine). |
| **Tier 2** | **Direct C-ABI FFI** | Declarative `extern "<lib>" { ... }`, `libffi`, `dlsym` | Zero-wrapper binding to unmodified existing shared libraries (e.g. `libm.so`, `libsqlite3.so`). |
| **Tier 3** | **LLVM AOT Integration** | LLVM IR `declare ...`, Clang linker forwarding | Standalone native binary generation via `xlang build -L... -l... -o app`. |

---

## 2. Examples in this Directory

### `math_demo.xb`
Demonstrates consumption of standard system math (`libm.so`) and C library utilities (`libc.so.6`):
- Mathematical functions: `sin()`, `cos()`, `sqrt()`, `pow()`, `ceil()`, `floor()`
- C library functions: `abs()`, `atoi()`, `atof()`
- Namespaced module dispatch: `libm.sin()` and `m.sin()`
- Real-world calculation: 2D Euclidean distance

#### Run via VM:
```bash
./bin/Release/xlang examples/ffi_c_abi/math_demo.xb
```

#### Compile to Standalone Native Binary:
```bash
./bin/Release/xlang build examples/ffi_c_abi/math_demo.xb -o math_demo_bin
./math_demo_bin
```

---

### `sqlite_demo.xb`
Demonstrates binding to an unmodified existing third-party C library (`libsqlite3.so.0`):
- Querying runtime engine metadata (`sqlite3_libversion()`, `sqlite3_sourceid()`)
- Querying engine memory subsystem counters (`sqlite3_memory_used()`)
- Invoking native SQLite string pattern-matching functions (`sqlite3_strglob()`, `sqlite3_strlike()`)

#### Run via VM:
```bash
./bin/Release/xlang examples/ffi_c_abi/sqlite_demo.xb
```

#### Compile to Standalone Native Binary:
```bash
./bin/Release/xlang build examples/ffi_c_abi/sqlite_demo.xb -o sqlite_demo_bin
./sqlite_demo_bin
```

---

### `zlib_demo.xb`
Demonstrates binding to standard data compression library `libz.so`:
- Querying library version (`zlibVersion()`)
- Computing CRC-32 checksums (`crc32()`), including incremental streaming across chunks
- Computing Adler-32 checksums (`adler32()`)
- Dual-dispatch calling via `z.zlibVersion()`

#### Run via VM:
```bash
./bin/Release/xlang examples/ffi_c_abi/zlib_demo.xb
```

#### Compile to Standalone Native Binary:
```bash
./bin/Release/xlang build examples/ffi_c_abi/zlib_demo.xb -lz -o zlib_demo_bin
./zlib_demo_bin
```

---

## 3. How Declarative FFI Works

### Syntax:
```xlang
extern "libm.so" {
    float sin(float x)
    float cos(float x)
    float pow(float base, float exp)
}
```

### Supported Types:
| xlang Type | C ABI Equivalent | LLVM IR Type |
| :--- | :--- | :--- |
| `int` | `int32_t` / `int` | `i32` |
| `long` | `int64_t` / `long` | `i64` |
| `float` / `double` | `double` | `double` |
| `float32` | `float` | `float` |
| `bool` | `uint8_t` / `bool` | `i1` |
| `char` | `int8_t` / `char` | `i8` |
| `string` | `const char*` | `i8*` |
| `object` | `void*` | `i8*` |
| `void` | `void` | `void` |

### Dual Dispatch:
When declared inside `extern "libm.so"`, functions are registered under two identifiers:
1. **Direct call**: `sin(0.0)`
2. **Module namespace**: `libm.sin(0.0)` and `m.sin(0.0)` (prefix `lib` stripped automatically)

---

## 4. Native Binary Compilation (`xlang build`)
When compiling with `xlang build`:
1. The compiler generates LLVM external function declarations:
   ```llvm
   declare double @sin(double)
   declare i32 @sqlite3_strglob(i8*, i8*)
   ```
2. Direct call instructions are emitted without any FFI interpreter trampoline.
3. The Clang linker automatically links `-lm`, `-l:libsqlite3.so.0`, or custom `-lmylib` specified via `-l`.
4. Embedded RPATH entries (`-Wl,-rpath`) ensure the executable locates shared libraries at runtime without manual environment variable setup.
