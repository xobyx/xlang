# C Extensions & Custom Native Libraries Examples

This directory demonstrates two distinct tiers of C/C++ library integration in **xlang**:
1. **Tier 1: High-Performance Vectorcall Extension SDK (`include/xlang.h`)**
   - Native modules utilizing xlang's C Extension API with dynamic object instantiation, memory management, and VM callbacks.
2. **Tier 2 & 3: Direct C-ABI FFI & Native Compilation (`extern "..." { ... }`)**
   - Pure C libraries without any xlang-specific headers or wrapper code, callable via dynamic FFI and compiled directly into standalone native binaries via `xlang build`.

---

## 1. Directory Structure

```text
examples/c_extension/
├── Makefile            # Builds fastmath.so, sqlite3.so, zlib.so, libmylib.so, raylib.so
├── fastmath.c          # Tier 1 Vectorcall C extension (gcd, hypot, reverse)
├── xsqlite3.c          # Tier 1 Vectorcall SQLite3 Engine extension
├── xzlib.c             # Tier 1 Vectorcall Zlib compression extension
├── mylib.c             # Tier 2/3 Pure C shared library (c_custom_calc)
├── fastmath_demo.xb    # xlang script importing and using fastmath
├── sqlite3_demo.xb     # xlang script importing and using sqlite3
├── zlib_demo.xb        # xlang script importing and using zlib
├── calc_app.xb         # xlang script consuming libmylib.so via Direct C-ABI FFI
├── raylib/             # Tier 1 complete 2D/3D Raylib game engine bindings
└── README.md
```

---

## 2. Building the Native Libraries

To compile all shared libraries (`fastmath.so`, `sqlite3.so`, `zlib.so`, `libmylib.so`, and `raylib.so`):

```bash
make -C examples/c_extension
```

This compiles the shared libraries and places them in `lib/`:
- `lib/fastmath.so` (and symlink `lib/libfastmath.so`)
- `lib/sqlite3.so` (and symlinks `lib/libsqlite3.so`, `lib/sqlite.so`, `lib/libsqlite.so`)
- `lib/zlib.so` (and symlink `lib/libzlib.so`)
- `lib/libmylib.so`
- `lib/raylib.so` (and symlink `lib/libraylib.so`)

---

## 3. Tier 1: SQLite3 Engine Vectorcall Extension (`xsqlite3.c` / `sqlite3_demo.xb`)

The SQLite3 extension provides a high-level, safe, and fast database interface:
- **Connection Management**: `connect(path)` (or `open_db(path)`), `disconnect(db)` (or `close_db(db)`)
- **DDL & DML**: `execute(db, sql)`
- **Scalar Queries**: `query_value(db, sql)`, `query_int(db, sql)`, `query_float(db, sql)`
- **Export Queries**: `query_json(db, sql)`, `query_csv(db, sql)`
- **Metadata**: `last_id(db)`, `changes_count(db)`, `engine_version()`, `memory_usage()`
- **Dual Namespace**: Also aliased as `sqlite` with `sqlite.open()`, `sqlite.exec()`, `sqlite.query()`, `sqlite.close()`

### Example in xlang:
```xlang
import "sqlite3"

int db = sqlite3.connect(":memory:")
sqlite3.execute(db, "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, salary REAL)")
sqlite3.execute(db, "INSERT INTO users (name, salary) VALUES ('Alice', 95000.0)")

string name = sqlite3.query_value(db, "SELECT name FROM users WHERE id = 1")
float salary = sqlite3.query_float(db, "SELECT salary FROM users WHERE id = 1")
string json = sqlite3.query_json(db, "SELECT id, name, salary FROM users")

sqlite3.disconnect(db)
```

### Running and Compiling:
```bash
# Run in Bytecode VM
./bin/Release/xlang examples/c_extension/sqlite3_demo.xb

# Build standalone native binary
./bin/Release/xlang build examples/c_extension/sqlite3_demo.xb -o sqlite3_app
./sqlite3_app
```

---

## 4. Tier 1: Zlib Compression Vectorcall Extension (`xzlib.c` / `zlib_demo.xb`)

The Zlib extension provides fast data compression, decompression, checksums, and gzip file I/O:
- **Checksums**: `crc32(crc, text)` (IEEE 802.3), `adler32(adler, text)`
- **In-Memory Deflate / Inflate**: `compress(text)` (returns hex string), `uncompress(hex_str)`, `compress_bound(len)`
- **Gzip File I/O**: `gz_write(path, content)`, `gz_read(path)`
- **File Archiving**: `compress_file(src, dst_gz)`, `decompress_file(src_gz, dst)`

### Example in xlang:
```xlang
import "zlib"

int crc = zlib.crc32(0, "Hello World")
int adler = zlib.adler32(1, "Hello World")

string hex_comp = zlib.compress("Large repetitive payload string...")
string restored = zlib.uncompress(hex_comp)

zlib.gz_write("archive.txt.gz", "Gzip compressed content")
string content = zlib.gz_read("archive.txt.gz")
```

### Running and Compiling:
```bash
# Run in Bytecode VM
./bin/Release/xlang examples/c_extension/zlib_demo.xb

# Build standalone native binary
./bin/Release/xlang build examples/c_extension/zlib_demo.xb -o zlib_app
./zlib_app
```

---

## 5. Tier 1: Vectorcall Extension (`fastmath.c` / `fastmath_demo.xb`)

The C extension uses `include/xlang.h`:

```c
#include "xlang.h"

XLANG_EXPORT int fast_gcd(int a, int b) { ... }

static XValue vec_fast_gcd(XVm* vm, XValue receiver, int argc, const XValue* args) {
    int a = (int)args[0].as.ival;
    int b = (int)args[1].as.ival;
    return xval_int(fast_gcd(a, b));
}

static const XExtensionFunc s_funcs[] = {
    { "gcd", vec_fast_gcd, 2, XLANG_TYPE_INT, { XLANG_TYPE_INT, XLANG_TYPE_INT } },
    { NULL, NULL, 0, XLANG_TYPE_VOID, { XLANG_TYPE_VOID } }
};

XLANG_EXPORT XLANG_EXTENSION_ENTRY {
    return ctx->register_module(ctx, "fastmath", s_funcs);
}
```

In xlang (`fastmath_demo.xb`):

```xlang
import "fastmath"

int g = fastmath.gcd(48, 18)
float h = fastmath.hypot(3.0, 4.0)
string rev = fastmath.reverse("hello xlang")
```

### Running and Compiling:
```bash
# Run in Bytecode VM
./bin/Release/xlang examples/c_extension/fastmath_demo.xb

# Build standalone native binary
./bin/Release/xlang build examples/c_extension/fastmath_demo.xb -o fastmath_app
./fastmath_app
```

---

## 6. Tier 2 & 3: Direct C-ABI FFI (`mylib.c` / `calc_app.xb`)

`mylib.c` contains pure C code with zero dependencies on xlang:

```c
int c_custom_calc(int a, int b) {
    return a * 2 + b * 3;
}
```

In xlang (`calc_app.xb`):

```xlang
extern "libmylib.so" {
    int c_custom_calc(int a, int b)
}

int res = c_custom_calc(10, 25)
int res2 = mylib.c_custom_calc(7, 3)
```

### Running in VM:
```bash
./bin/Release/xlang -Llib examples/c_extension/calc_app.xb
```

### Compiling to Standalone Native Binary (`xlang build`):
```bash
./bin/Release/xlang build examples/c_extension/calc_app.xb -Llib -lmylib -o calc_app
./calc_app
```
