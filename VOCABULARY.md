# XLANG Language Vocabulary & Syntax Reference

This document provides a comprehensive reference of the vocabulary, grammar, type system, built-in functions, and language constructs for **xlang**.

---

## 1. Lexical Structure

### 1.1 Comments
xlang supports both single-line and multi-line comments:
- **Hash Single-line**: Begins with `#` until end-of-line:
  ```xlang
  # This is a single-line comment
  int x = 10 # Inline comment
  ```
- **C-style Single-line**: Begins with `//` until end-of-line:
  ```xlang
  // Standard C-style comment
  int y = 20 // Inline comment
  ```
- **Multi-line Block Comments**: Enclosed between `/*` and `*/`:
  ```xlang
  /* Multi-line comment block
     spanning multiple lines */
  int z = 30
  ```

### 1.2 Identifiers
Identifiers are case-sensitive and must begin with an ASCII letter (`a-z`, `A-Z`) or underscore (`_`), followed by any combination of letters, digits (`0-9`), and underscores.
- Valid: `count`, `_total`, `pointX`, `Animal_2`
- Invalid: `2cool`, `my-var`, `class` (keyword)

### 1.3 Literals
- **Integer**: Sequences of digits, optional sign: `0`, `42`, `-17`, `+100`
- **Float**: Digits with a decimal point: `3.14`, `0.5`, `-12.8`
- **Boolean**: `true` and `false`
- **Character**: Single characters enclosed in single quotes: `'a'`, `'Z'`, `'\n'`
- **String**: Sequences of characters enclosed in double quotes: `"hello"`, `"xlang\n"`
- **Null**: `null` literal representing uninitialized object or pointer reference

### 1.4 Statement Terminators
Statements can be terminated either by a newline or by a semicolon (`;`).
```xlang
int a = 1; int b = 2
int c = 3
```

---

## 2. Keywords

xlang defines 15 reserved keywords:

| Keyword | Description | Example Syntax |
| :--- | :--- | :--- |
| `if` | Conditional branch execution | `if (x == 10) { ... }` |
| `eif` | "Else if" conditional branch | `eif (x > 10) { ... }` |
| `else` | Fallback branch for `if`/`eif` | `else { ... }` |
| `while` | Loop while condition is true | `while (i < 5) { ... }` |
| `for` | Stepper and iteration loop | `for (int i = 0, i++, i < 5) { ... }` |
| `do` | Do-while loop construct | `do { ... } while (i < 5)` |
| `return` | Return from function with optional value | `return a + b` |
| `break` | Terminate loop execution | `break` |
| `continue` | Skip to next loop iteration | `continue` |
| `class` | Define a user-defined class / type | `class Point(Base) { ... }` |
| `static` | Static method or property declaration inside class | `static int add(int a, int b)`, `static int count` |
| `new` | Instantiate class and invoke constructor as an expression | `Calculator c = new Calculator(10)`, `new Box().open()` |
| `import` | Compile-time multi-file modular import | `import "math.xb"`, `import collections` |
| `extern` | Declarative direct C-ABI FFI library block | `extern "libm.so" { float sin(float x) }` |
| `in` | Iterator separator for collection loops | `for (item in list) { ... }` |

---

## 3. Data Types

### 3.1 Primitive Types
| Type | Description | Size | Default / Zero Value |
| :--- | :--- | :--- | :--- |
| `int` | 32-bit signed integer | 4 bytes | `0` |
| `char` | 8-bit character / integer | 1 byte | `'\0'` |
| `string` | Dynamic character string | Pointer | `""` |
| `bool` | Boolean (`true` / `false`) | 1 byte | `false` |
| `float` | 32-bit single-precision float | 4 bytes | `0.0` |
| `long` | 64-bit signed integer | 8 bytes | `0L` |
| `double` | 64-bit double-precision float | 8 bytes | `0.0` |
| `object` | Generic object reference | Pointer | `null` |

### 3.2 Array Types & Subscript Indexing
Arrays are declared with size specified inside square brackets after the type:
```xlang
int[5] numbers
string[3] names
```
Elements are accessed using 0-based indexing with `[index]`:
```xlang
numbers[0] = 42
int first = numbers[0]
```

### 3.3 String Subscript Indexing
Strings support 0-based subscript indexing `str[index]` to read individual character values or test character equality:
```xlang
string word = "racecar"
string first_ch = word[0]     // "r"
int last_idx = strlen(word) - 1
bool matches = (word[0] == word[last_idx]) // true
```

### 3.4 User-Defined Types (Classes)
Classes are registered as custom types once declared. They can be used as variable types, parameter types, and return types:
```xlang
Point pt
Animal cat
```

---

## 4. Operators

### 4.1 Arithmetic Operators
| Operator | Name | Example | Description |
| :---: | :--- | :--- | :--- |
| `+` | Addition / Concatenation | `a + b` | Adds numbers or concatenates strings |
| `-` | Subtraction / Negation | `a - b`, `-x` | Subtracts or negates numbers |
| `*` | Multiplication | `a * b` | Multiplies numbers |
| `/` | Division | `a / b` | Divides numbers |
| `%` | Modulo | `a % b` | Remainder of integer division |

### 4.2 Comparison / Relational Operators
| Operator | Name | Example | Description |
| :---: | :--- | :--- | :--- |
| `==` | Equality | `a == b` | True if operands are equal |
| `!=` | Inequality | `a != b` | True if operands are not equal |
| `<` | Less than | `a < b` | True if `a` is strictly less than `b` |
| `>` | Greater than | `a > b` | True if `a` is strictly greater than `b` |
| `<=` | Less than or equal | `a <= b` | True if `a` is less than or equal to `b` |
| `>=` | Greater than or equal | `a >= b` | True if `a` is greater than or equal to `b` |

### 4.3 Logical Operators
| Operator | Name | Example | Description |
| :---: | :--- | :--- | :--- |
| `&&` | Logical AND | `a && b` | True if both conditions are true |
| `\|\|` | Logical OR | `a \|\| b` | True if either condition is true |
| `!` | Logical NOT | `!a` | Inverts boolean truth value |

### 4.4 Bitwise Operators
| Operator | Name | Example | Description |
| :---: | :--- | :--- | :--- |
| `&` | Bitwise AND | `a & b` | Bitwise AND of integer bits |
| `\|` | Bitwise OR | `a \| b` | Bitwise OR of integer bits |
| `^` | Bitwise XOR | `a ^ b` | Bitwise exclusive OR |
| `~` | Bitwise NOT | `~a` | One's complement bitwise inversion |
| `<<` | Shift Left | `a << 2` | Bitwise shift left |
| `>>` | Shift Right | `a >> 2` | Bitwise shift right |

### 4.5 Assignment & Compound Operators
| Operator | Description | Example |
| :---: | :--- | :--- |
| `=` | Simple assignment | `x = 10`, `obj.field = 20` |
| `+=` | Add and assign | `x += 5` (equivalent to `x = x + 5`) |
| `-=` | Subtract and assign | `x -= 2` |
| `*=` | Multiply and assign | `x *= 3` |
| `/=` | Divide and assign | `x /= 2` |
| `%=` | Modulo and assign | `x %= 10` |
| `++` | Increment by 1 | `i++` |
| `--` | Decrement by 1 | `i--` |

### 4.6 Member Access, Instantiation & Grouping
| Symbol | Name | Description |
| :---: | :--- | :--- |
| `.` | Dot operator | Access object properties and methods (`obj.prop`, `obj.method()`) |
| `new` | Instantiation operator | Allocates and invokes constructor (`new ClassName(...)`) |
| `( )` | Parentheses | Parameter lists, argument lists, expression grouping, loop specs |
| `[ ]` | Brackets | Array declarations and index subscripting (`arr[i]`, `str[i]`) |
| `{ }` | Braces | Code block delimiters (functions, loops, classes, conditionals) |
| `,` | Comma | Separator for function parameters, arguments, and loop clauses |

---

## 5. Control Flow Statements

### 5.1 If / Eif / Else
```xlang
if (score >= 90) {
    print("Grade: A")
}
eif (score >= 80) {
    print("Grade: B")
}
else {
    print("Grade: C or lower")
}
```

### 5.2 While Loop
```xlang
int i = 0
while (i < 5) {
    print(i)
    i = i + 1
}
```

### 5.3 For Loop
In xlang, the standard `for` loop syntax declares the iterator variable, step expression, and condition inside parentheses:
`for (<type> <var> = <init>, <step_expression>, <condition>) { ... }`

```xlang
for (int i = 0, i++, i < 5) {
    print(i)
}
```
You can also use compound steps (`i += 2`, `i = i + 1`), pre-declared variables (`for (i = 0, i++, i < 5)`), or C-style semicolon delimiters (`for (int i = 0; i < 5; i++)`).

#### 5.3.1 For-in Collection Loop
```xlang
for (item in list) {
    print(item)
}
```

#### 5.3.2 Classic Stepper Loop (Backward Compatible)
```xlang
for i (0, i + 1, i < 5) {
    print(i)
}
```

### 5.4 Do-While Loop
```xlang
int k = 0
do {
    print(k)
    k = k + 1
} while (k < 3)
```

### 5.5 Break and Return
- `break`: Immediately exits the innermost active loop.
- `return [value]`: Exits the current function and returns the computed value.

---

## 6. Functions

### 6.1 Function Definition
Functions specify a return type, name, parenthesized parameter list with types, and a brace-enclosed body:
```xlang
int add(int a, int b) {
    return a + b
}

string greet(string name) {
    return "Hello, " + name
}
```

### 6.2 Function Calling
```xlang
int sum = add(10, 20)
print(sum)
```

---

## 7. Object-Oriented Programming (Classes)

### 7.1 Class Declaration & Fields
```xlang
class Animal() {
    int legs
    string name
}
```

### 7.2 Methods
```xlang
class Animal() {
    int legs
    int speak() {
        print(legs)
    }
}
```

### 7.3 Constructors
Constructors can be declared using the class name:
```xlang
class Point() {
    int x
    int y

    # Default 0-parameter constructor
    Point() {
        x = 0
        y = 0
    }

    # Parameterized constructor
    Point(int a, int b) {
        x = a
        y = b
    }

    # Parameterized constructor with 'this' reference
    # Point(int x, int y) {
    #     this.x = x
    #     this.y = y
    # }

    int get_sum() {
        return x + y
    }
}
```

### 7.4 Object Instantiation

xlang supports both declaration-style instantiation and expression-style `new` instantiation:

#### 7.4.1 Declaration-Style Instantiation
- **Default Instantiation:** Invokes the 0-argument constructor (or zeroes fields if no constructor is defined):
  ```xlang
  Point p1
  ```
- **Parameterized Instantiation:** Invokes the constructor matching the argument count:
  ```xlang
  Point p2(10, 20)
  int vx = 30
  int vy = 40
  Point p3(vx, vy)
  ```

#### 7.4.2 Expression-Style `new` Instantiation
The `new` keyword allocates a new instance, invokes the constructor, and returns the instance as an expression value across all execution modes (tree-walk, VM, JIT, AOT):
```xlang
# Instantiation in variable declaration / assignment
Calculator calc = new Calculator(10)
Box b = new Box()

# Returning newly created instances from functions
Box make_box(int val) {
    return new Box(val)
}
```

#### 7.4.3 Direct Method Invocations and Fluent Chaining on `new`
Methods can be called directly on newly instantiated objects without requiring an intermediate variable binding:
```xlang
# Direct method call on new instance
int initial_val = new Calculator(10).result() // 10

# Fluent method chaining on methods that return 'this'
class Calculator {
    int total
    Calculator(int init) { this.total = init }
    Calculator add(int n) { this.total = this.total + n; return this }
    Calculator mul(int n) { this.total = this.total * n; return this }
    int result() { return this.total }
}

int chained = new Calculator(5).add(15).mul(2).result() // (5 + 15) * 2 = 40
```

#### 7.4.4 Forward References & Recursive Objects
Classes can reference other classes declared later in the file or recursively link to each other:
```xlang
class NodeA {
    NodeB next
    int val
}

class NodeB {
    NodeA prev
    int val
}
```

### 7.5 Inheritance
Inheritance is specified in parentheses following the class name:
```xlang
class Animal() {
    int legs
    int speak() {
        print(legs)
    }
}

class Dog(Animal) {
    int bark() {
        print("Woof")
    }
}

Dog d
d.legs = 4
d.speak()  # Inherited from Animal
d.bark()   # Defined on Dog
```

### 7.6 Self-Reference (`this`)
Inside class methods and constructors, `this` refers to the current object instance:
```xlang
this.x = a
```

### 7.7 Static Methods and Properties
Static members belong to the class itself rather than individual object instances. They are declared using the `static` keyword prefix.

#### Static Properties
Static properties hold class-level state shared across the program:
```xlang
class Counter() {
    static int global_count

    static int get_count() {
        return Counter.global_count
    }
}

# Access and mutation via ClassName.property
Counter.global_count = 100
print(Counter.global_count)  # 100
```

#### Static Methods
Static methods execute without requiring an instance object (`this` is null):
```xlang
class MathHelper() {
    static int add(int a, int b) {
        return a + b
    }

    static int multiply(int a, int b) {
        return a * b
    }

    static int double_val(int x) {
        # Static methods can call other static methods
        return MathHelper.add(x, x)
    }

    static int greet(string name) {
        print("Hello, %s!\n", name)
        return 0
    }
}

# 1. In variable assignment
int sum = MathHelper.add(15, 25)

# 2. In compound expressions
int combo = MathHelper.add(10, 20) + MathHelper.multiply(2, 3)

# 3. As standalone statements
MathHelper.greet("World")
```

#### Factory Methods and Method Chaining
Static methods can construct and return instances of the class (factory methods). The returned instance supports direct method chaining:
```xlang
class Box() {
    int width
    int height

    Box(int w, int h) {
        this.width = w
        this.height = h
    }

    static Box createSquare(int size) {
        Box b(size, size)
        return b
    }

    int area() {
        return this.width * this.height
    }
}

# Factory creation
Box sq = Box.createSquare(8)
print("Square area: %d\n", sq.area())  # 64

# Chained method invocation on returned instance
int chained_area = Box.createSquare(5).area()
print("Chained: %d\n", chained_area)  # 25
```

---

## 8. Built-in Standard Library Functions

| Function Signature | Return Type | Description |
| :--- | :--- | :--- |
| `print(expr)` | `int` | Prints an expression followed by a newline. |
| `print(fmt, ...)` | `int` | Formatted output supporting `%d`, `%s`, `%f`, `%c`, `%ld`, `%%`. |
| `vprint(...)` | `int` | Alias for `print`. |
| `len(str_or_arr)` | `int` | Returns the length of a string or array. |
| `eql(str1, str2)` | `int` | Returns `1` if strings are equal, `0` otherwise. |
| `replace(src, target, rep)` | `string` | Replaces occurrences of `target` with `rep` in `src`. |
| `str(number)` | `string` | Converts an integer or number to its string representation. |
| `time()` | `string` | Returns the current system date and time as a string. |
| `random(max)` | `int` | Returns a pseudo-random integer in the range `[0, max)`. |
| `scan(prompt)` | `int` | Reads input from standard input. |
| `eval(code_string)` | `int` | Dynamically parses and executes an xlang code string. |
| `import(file_path)` | `int` | Imports and runs an external `.xb` script. |
| `echo(variable)` | `int` | Debug inspector that displays variable metadata and value. |
| `exit(code)` | `void` | Exits the process immediately with the given status code. |
| `file_read_all(path)` | `string` | Reads an entire file into a string. Returns empty string on error. Alias: `read_file`. |
| `file_write_all(path, data)` | `int` | Writes string `data` to file at `path`. Overwrites existing content. Returns bytes written or `-1`. Alias: `write_file`. |
| `file_append(path, data)` | `int` | Appends string `data` to end of file at `path`. Returns bytes written or `-1`. |
| `file_exists(path)` | `int` | Returns `1` if file at `path` exists, `0` otherwise. |
| `file_remove(path)` | `int` | Deletes file at `path`. Returns `0` on success, `-1` on failure. Alias: `file_delete`. |
| `file_size(path)` | `int` | Returns file size in bytes or `-1` on error. |
| `file_open(path, mode)` | `int` | Opens file with mode (`"r"`, `"w"`, `"a"`, etc.). Returns integer handle (`> 0`) or `-1`. |
| `file_read(handle, bytes)` | `string` | Reads up to `bytes` from open file handle. Returns read string. |
| `file_write(handle, data)` | `int` | Writes string `data` to open file handle. Returns bytes written or `-1`. |
| `file_close(handle)` | `int` | Closes open file handle. Returns `0` on success or `-1`. |
| `get_argc()` | `int` | Returns the number of command-line arguments passed after the script path. |
| `get_arg(index)` | `string` | Returns the script argument at 0-based `index` (or empty string if out of range). |
| `exec(command)` | `int` | Executes a shell command via `system()`. Returns exit code. Alias: `system_exec`. |
| `getenv(var_name)` | `string` | Retrieves the value of an environment variable. Alias: `system_getenv`. |
| `setenv(var_name, val)` | `int` | Sets an environment variable (`0` on success, `-1` on failure). Alias: `system_setenv`. |
| `regex_match(str, pattern)` | `int` | Returns `1` if PCRE `pattern` matches anywhere in `str`, `0` otherwise. |
| `regex_find(str, pattern)` | `string` | Returns the first matched substring of PCRE `pattern` in `str` (or empty string). |
| `regex_replace(str, pat, rep)` | `string` | Replaces all non-overlapping occurrences of PCRE `pat` in `str` with `rep`. |
| `substr(str, start, len)` | `string` | Returns substring starting at `start` for `len` characters. |
| `index_of(str, needle)` | `int` | Returns 0-based index of first occurrence of `needle` in `str` (`-1` if not found). Alias: `find`. |
| `trim(str)` | `string` | Trims leading and trailing whitespace from `str`. |
| `to_lower(str)` | `string` | Returns lowercase copy of `str`. |
| `to_upper(str)` | `string` | Returns uppercase copy of `str`. |
| `starts_with(str, prefix)` | `int` | Returns `1` if `str` begins with `prefix`, `0` otherwise. |
| `ends_with(str, suffix)` | `int` | Returns `1` if `str` ends with `suffix`, `0` otherwise. |
| `sqrt(x)` | `float` | Returns square root of `x`. Alias: `math_sqrt`. |
| `pow(base, exp)` | `float` | Returns `base` raised to `exp`. Alias: `math_pow`. |
| `abs(x)` | `int`/`float` | Returns absolute value of `x`. Alias: `math_abs`. |
| `min(a, b)` | `int`/`float` | Returns minimum of `a` and `b`. Alias: `math_min`. |
| `max(a, b)` | `int`/`float` | Returns maximum of `a` and `b`. Alias: `math_max`. |
| `floor(x)` | `float` | Returns floor of `x`. Alias: `math_floor`. |
| `ceil(x)` | `float` | Returns ceil of `x`. Alias: `math_ceil`. |
| `round(x)` | `float` | Returns nearest integer to `x`. Alias: `math_round`. |
| `sin(x)` | `float` | Returns sine of `x` (radians). Alias: `math_sin`. |
| `cos(x)` | `float` | Returns cosine of `x` (radians). Alias: `math_cos`. |
| `tan(x)` | `float` | Returns tangent of `x` (radians). Alias: `math_tan`. |
| `log(x)` | `float` | Returns natural logarithm of `x`. Alias: `math_log`. |
| `socket_create(type)` | `int` | Creates a new socket (`"tcp"` or `"udp"`). Returns `sockfd >= 0` or `-1`. Alias: `socket`. |
| `socket_connect(s, host, port)` | `int` | Connects socket `s` to remote `host` and `port`. Returns `0` or `-1`. Alias: `connect`. |
| `socket_bind(s, host, port)` | `int` | Binds socket `s` to local `host` and `port`. Returns `0` or `-1`. Alias: `bind`. |
| `socket_listen(s, backlog)` | `int` | Listens for incoming connections on socket `s`. Returns `0` or `-1`. Alias: `listen`. |
| `socket_accept(s)` | `int` | Accepts client connection on listening socket `s`. Returns client `sockfd >= 0` or `-1`. Alias: `accept`. |
| `socket_send(s, data)` | `int` | Sends string `data` over socket `s`. Returns bytes sent or `-1`. Alias: `send`. |
| `socket_recv(s, max_bytes)` | `string` | Receives up to `max_bytes` of string data from socket `s`. Alias: `recv`. |
| `socket_close(s)` | `int` | Closes socket `s`. Returns `0` or `-1`. Alias: `close`. |
| `socket_set_timeout(s, sec)` | `int` | Sets send and receive timeout on socket `s` in seconds. Returns `0` or `-1`. |
| `socket_set_reuseaddr(s, opt)` | `int` | Sets `SO_REUSEADDR` on socket `s` (`1` = enable). Returns `0` or `-1`. |
| `socket_sendto(s, data, host, port)` | `int` | Sends UDP datagram `data` to `host:port`. Returns bytes sent or `-1`. |
| `socket_recvfrom(s, max_bytes)` | `string` | Receives UDP datagram data up to `max_bytes`. |
| `http_get(url)` | `string` | Performs an HTTP GET request to `url` and returns the response string. |
| `gc_collect()` | `int` | Explicitly triggers mark-and-sweep garbage collection. Returns collected bytes. |
| `gc_allocated_bytes()` | `int` | Returns current total heap bytes allocated and managed by the GC. |
| `gc_total_objects()` | `int` | Returns current count of active heap objects tracked by the GC. |
| `gc_enable()` | `int` | Enables automated safe-point garbage collection (returns `1`). |
| `gc_disable()` | `int` | Disables automated safe-point garbage collection (returns `0`). |
| `gc_set_threshold(bytes)` | `int` | Sets memory allocation threshold (in bytes) triggering auto collection. |
| `gc_dump()` | `int` | Prints comprehensive GC heap diagnostic statistics to stdout. |
| `assert(cond)` | `void` | Evaluates `cond`; terminates execution if falsy. Elided in `--release` builds. |
| `assert(cond, message)` | `void` | Evaluates `cond`; aborts with `message` if falsy. Elided in `--release` builds. |
| `strlen(str)` | `int` | Returns the number of characters in string `str`. |
| `json_is_valid(str)` | `int` | Returns `1` if `str` is valid JSON, `0` otherwise. |
| `json_parse(str)` | `Map` | Parses JSON string into a structured `Map`. |
| `json_stringify(map)` | `string` | Serializes `Map` into a formatted JSON string. |
| `datetime_now()` | `int` | Returns current Unix epoch timestamp in seconds. |
| `datetime_year(ts)` | `int` | Extracts year component from Unix timestamp `ts`. |
| `datetime_month(ts)` | `int` | Extracts month (1–12) from Unix timestamp `ts`. |
| `datetime_day(ts)` | `int` | Extracts day of month (1–31) from Unix timestamp `ts`. |
| `datetime_hour(ts)` | `int` | Extracts hour (0–23) from Unix timestamp `ts`. |
| `datetime_minute(ts)` | `int` | Extracts minute (0–59) from Unix timestamp `ts`. |
| `datetime_second(ts)` | `int` | Extracts second (0–59) from Unix timestamp `ts`. |
| `datetime_format(ts, fmt)` | `string` | Formats Unix timestamp `ts` using strftime format `fmt`. |
| `datetime_clock_ms()` | `int` | Returns high-resolution monotonic clock in milliseconds. |
| `clock_ms()` | `int` | Returns current high-resolution monotonic timestamp in milliseconds. Alias: `time_ms`. |
| `time_ms()` | `int` | Alias for `clock_ms()`. |

---

## 9. Networking & Sockets

xlang provides comprehensive cross-platform BSD/POSIX socket support (Linux, macOS, Windows/Winsock) with both procedural and object-oriented paradigms.

### 9.1 TCP Client / Server Example
```xlang
# TCP Server
int server = socket_create("tcp")
socket_set_reuseaddr(server, 1)
socket_bind(server, "127.0.0.1", 8080)
socket_listen(server, 5)

# TCP Client
int client = socket_create("tcp")
socket_connect(client, "127.0.0.1", 8080)

# Accept and exchange messages
int peer = socket_accept(server)
socket_send(client, "Hello from client!")
string msg = socket_recv(peer, 1024)
print("Server received: %s", msg)

socket_close(client)
socket_close(peer)
socket_close(server)
```

### 9.2 UDP Datagram Example
```xlang
int s_recv = socket_create("udp")
socket_bind(s_recv, "127.0.0.1", 9000)

int s_send = socket_create("udp")
socket_sendto(s_send, "UDP Ping", "127.0.0.1", 9000)

string umsg = socket_recvfrom(s_recv, 1024)
print("UDP received: %s", umsg)

socket_close(s_recv)
socket_close(s_send)
```

### 9.3 HTTP Client
```xlang
string response = http_get("http://127.0.0.1:8080/api/status")
print("Response: %s", response)
```

---

## 10. File I/O

xlang provides both one-shot whole-file operations and stream-based file handle operations. Note that when running scripts, file paths are relative to the directory containing the executed script.

### 10.1 Quick Whole-File Operations
```xlang
# Write string content to file (overwrites existing)
int written = file_write_all("data.txt", "Hello xlang!")

# Check existence and size
if (file_exists("data.txt") == 1) {
    int sz = file_size("data.txt")
    print("File size: %d bytes", sz)
}

# Append content
file_append("data.txt", " More text.")

# Read entire file into string
string content = file_read_all("data.txt")
print("Content: %s", content)

# Delete file
file_remove("data.txt")
```

### 10.2 Stream / Handle-Based File Operations
```xlang
# Open file in read mode ("r", "w", "a", "rb", "wb")
int fd = file_open("data.txt", "r")
if (fd > 0) {
    string chunk = file_read(fd, 256)
    print("Read: %s", chunk)
    file_close(fd)
}
```

### 10.3 Object-Oriented File Class (`lib/file.xb`)
```xlang
import("file.xb")

File f("data.txt")
f.write_all("Hello OOP File!")

if (f.exists() == 1) {
    print("File size: %d bytes", f.size())
}

string content = f.read_all()
print("Content: %s", content)

f.append(" Appended line.")

f.open("r")
string chunk = f.read(5)
print("Chunk: %s", chunk)
f.close()

f.remove()
```

---

## 11. System & CLI Arguments

Scripts can access arguments passed on the command line after the script path, inspect and modify environment variables, and invoke external shell commands.

### 11.1 Procedural Functions
```xlang
# CLI Arguments (e.g. running: xlang myscript.xb alpha beta)
int count = get_argc()
print("Argument count: %d", count)

if (count > 0) {
    string first_arg = get_arg(0)
    print("First argument: %s", first_arg)
}

# Environment Variables
setenv("APP_ENV", "production")
string env_val = getenv("APP_ENV")
print("APP_ENV: %s", env_val)

# Shell Execution
int exit_code = exec("mkdir -p output_dir")
print("Command exited with: %d", exit_code)
```

### 11.2 Object-Oriented System Class (`lib/system.xb` / `lib/sys.xb`)
```xlang
import("sys.xb")

System sys
print("Arguments: %d", sys.argc())
string a0 = sys.arg(0)

sys.setenv("ENV_KEY", "value")
string val = sys.getenv("ENV_KEY")

int rc = sys.exec("echo hello")
```

---

## 12. Regular Expressions & String Utilities

xlang includes PCRE (Perl Compatible Regular Expressions) for pattern matching, extraction, and substitution, as well as common string manipulation functions.

### 12.1 Native Object-Oriented String Methods
All `string` variables support fluent OOP methods directly:
```xlang
string s = "   Hello World 123   "

# Slicing & Trimming
string trimmed = s.trim()                  # "Hello World 123"
string sub = trimmed.substr(0, 5)          # "Hello"
int idx = trimmed.index_of("World")        # 6

# Case Transformation
string lower = trimmed.to_lower()          # "hello world 123"
string upper = trimmed.to_upper()          # "HELLO WORLD 123"

# Matching Prefixes & Suffixes
int sw = trimmed.starts_with("Hello")      # 1
int ew = trimmed.ends_with("123")          # 1

# PCRE Regular Expressions on Strings
int has_digits = trimmed.regex_match("\d+") # 1
string digits = trimmed.regex_find("\d+")   # "123"
string rep = trimmed.regex_replace("\d+", "999") # "Hello World 999"
```

### 12.2 Object-Oriented Regex Class (`lib/regex.xb`)
```xlang
import("regex.xb")

Regex r("\d+")
int matched = r.match("user_4821")          # 1
string num = r.find("user_4821")            # "4821"
string replaced = r.replace("id: 99", "100") # "id: 100"
```

### 12.3 Object-Oriented String Wrapper (`lib/str.xb`)
```xlang
import("str.xb")

Str st("  padded text  ")
string t = st.trim()
print("Length: %d", st.len())
```

---

## 13. Mathematical Standard Library

The math module provides common mathematical utilities and trigonometric functions.

### 13.1 Procedural Functions
```xlang
# Roots & Powers
float s = sqrt(16.0)     # 4.0
float p = pow(2.0, 3.0)   # 8.0

# Min / Max / Abs
int val_abs = abs(0 - 50) # 50
int m_min = min(10, 20)   # 10
int m_max = max(10, 20)   # 20

# Rounding
float fl = floor(3.7)     # 3.0
float cl = ceil(3.2)      # 4.0
float rd = round(3.5)     # 4.0

# Trigonometry & Logarithms
float s0 = sin(0.0)       # 0.0
float c0 = cos(0.0)       # 1.0
float t0 = tan(0.0)       # 0.0
float l1 = log(1.0)       # 0.0
```

### 13.2 Object-Oriented Math Class (`lib/math.xb`)
```xlang
import("math.xb")

Math m
float sq = m.sqrt(25.0)    # 5.0
float pw = m.pow(2.0, 4.0)  # 16.0
int val = m.abs(0 - 42)    # 42
int mn = m.min(10, 20)     # 10
int mx = m.max(10, 20)     # 20
float fl = m.floor(3.7)    # 3.0
float cl = m.ceil(3.2)     # 4.0
float rd = m.round(3.5)    # 4.0
float s = m.sin(0.0)       # 0.0
float c = m.cos(0.0)       # 1.0
float l = m.log(1.0)       # 0.0
```

---

## 14. Compile-Time Multi-File Modular Imports

xlang provides a built-in compile-time module system via the `import` statement.

### 14.1 Import Syntax Forms
- **String literal path:**
  ```xlang
  import "math.xb"
  import "path/to/module.xb"
  import "module"       # .xb is automatically inferred
  ```
- **Bare module identifier:**
  ```xlang
  import collections    # Resolves to collections.xb
  import math           # Resolves to math.xb
  ```
- **Backward-compatible function call syntax:**
  ```xlang
  import("math.xb")
  ```

### 14.2 Module Resolution Rules
When an import statement is encountered, the module locator checks:
1. Exact file path as provided.
2. File path with `.xb` appended.
3. In `lib/` (standard library directory).
4. In `../lib/`.
5. In parent directory (`../`).
6. Basename in the current script directory.

### 14.3 Circular Import Protection & Idempotency
- **Deduplication:** Modules are indexed by their canonical system path. Importing the same module multiple times across different files only compiles it once.
- **Cycle Detection:** If Module A imports Module B, and Module B imports Module A, circular recursion is safely detected and aborted without crashing or stack overflowing.

---

## 15. Dynamic Collections (Lists & Hash Maps)

xlang includes high-performance dynamic collections available in `lib/list.xb`, `lib/map.xb`, and `lib/collections.xb`.

### 15.1 Dynamic List (`List`)
The `List` class provides a dynamically-resizing array collection supporting multiple data types and native subscript indexing `[index]`.

```xlang
import "list.xb"

List nums
nums.add_int(10)
nums.add_int(20)
nums.add_int(30)

# Native subscript indexing: read & write
int first = nums[0]         # 10
nums[1] = 999               # Mutate element at index 1
int second = nums[1]        # 999

# Methods
int sz = nums.size()        # 3
int contains = nums.contains_int(999)  # 1
int idx = nums.index_of_int(999)       # 1
nums.remove_at(0)           # Removes 10, shifts items left
int popped = nums.pop_int() # Removes and returns last element

# String Lists & Joining
List fruits
fruits.add("apple")
fruits.add("banana")
fruits.add("cherry")
string joined = fruits.join(", ")      # "apple, banana, cherry"
string str_rep = fruits.to_str()       # "[apple, banana, cherry]"
```

### 15.2 Hash Map (`Map` / `HashMap`)
The `Map` (and `HashMap`) class provides a fast hash table with string keys, dynamic rehashing, and support for integer, string, and float values.

```xlang
import "map.xb"

Map config
config.put("host", "localhost")
config.put_int("port", 8080)
config.put_float("timeout", 5.5)

# Retrieval
string host = config.get("host")          # "localhost"
int port = config.get_int("port")         # 8080
float timeout = config.get_float("timeout") # 5.500000

# Membership & Removal
int has_host = config.has("host")         # 1
config.remove("host")
int has_after = config.has("host")        # 0

# Keys, Values, and String Representation
string keys_list = config.keys()          # "port, timeout"
string values_list = config.values()      # "8080, 5.500000"
string formatted = config.to_str()        # "{\"port\": \"8080\", \"timeout\": \"5.500000\"}"
```

---

## 16. Memory Management & Garbage Collection (`GC`)

xlang features an automated mark-and-sweep garbage collection subsystem (`xgc`) preventing unbounded memory growth during long-running loops, expression evaluations, function calls, and object/collection allocations.

### 16.1 Architecture & Safe-Point Model
- **Heap Allocator**: All dynamic memory blocks (primitive value buffers, strings, class instances, call frames, collection buffers) are prefixed with a managed header (`gc_block_t`) tracked in a doubly linked list.
- **Root Set Traversal**:
  - Global variable stack (`varss`)
  - Active call frames stack (`g_gc_state.call_stack` via `fcall`)
  - Static class properties (`types`)
  - Explicit roots registered by compiler execution nodes (such as loop condition expressions in `while` and `for`)
- **Tri-Color Cycle Prevention**: Object traversal tracks block marks (0 = white, 1 = grey / reachable buffer, 2 = black / fully scanned composite). Objects containing self-references (`this`) or mutual cyclic references are cleanly detected and scanned without recursion loops.
- **Safe-Point Automation**: Automatic garbage collection checks (`gc_check_auto`) occur at safe points:
  - Iteration boundaries of `while` and `for` loops
  - Top-level and block statement boundaries
  - When allocated heap bytes exceed the threshold (`threshold_bytes`) or temporary variable stack exceeds limits (`t_varss->size > 32`)

### 16.2 Standard Library OOP `GC` Class (`lib/gc.xb`)
Import `gc.xb` to manage the garbage collector via the `GC` OOP interface:

```xlang
import "gc.xb"

# Inspect current heap allocation
int bytes = GC.allocated_bytes()
int objects = GC.total_objects()
print("Allocated bytes: %d, Objects: %d", bytes, objects)

# Manually trigger mark-and-sweep collection
int collected = GC.collect()
print("Collected bytes: %d", collected)

# Adjust collection threshold (in bytes)
GC.set_threshold(1048576) # Set to 1 MB

# Enable or disable automated safe-point collection
GC.disable() # Suspends auto collection (returns 0)
GC.enable()  # Resumes auto collection (returns 1)

# Diagnostic report to stdout
GC.dump()
```

### 16.3 GC Diagnostic Dump
Calling `GC.dump()` (or native `gc_dump()`) outputs detailed internal state metrics:
```text
=== XLANG Garbage Collector Diagnostics ===
  Status:               ENABLED
  Allocated Memory:     82744 bytes (80.80 KB)
  Collection Threshold: 524288 bytes (512.00 KB)
  Active Heap Objects:  91
  Collections Triggered:21
  Total Bytes Swept:    346194 bytes (338.08 KB)
  Call Stack Depth:     2 frames
  Temp Stack Count:     7 vars
============================================
```

### 16.4 Loop Stress & Collection Safety
Safe-point GC automatically reclaims dead temporary variables and intermediate allocations without corrupting live variables or collections:

```xlang
import "gc.xb"
import "list.xb"
import "map.xb"

# Dynamic collections remain completely intact across collections
List list1
list1.add_int(100)
list1.add("live_item")

Map map1
map1.put("k", "v")

# Loop iterations create and discard temporary vars reclaimed at safe points
int i = 0
int sum = 0
while (i < 1000) {
    sum = sum + i
    i = i + 1
}

# Explicitly trigger collection while collections are in scope
GC.collect()

# Collections and loop results remain valid
print("Sum: %d", sum)
print("List size: %d", list1.size())
print("Map item: %s", map1.get("k"))
```

---

## 17. JSON & DateTime Standard Library

xlang includes built-in standard library modules for JSON parsing/serialization and DateTime manipulation located in `lib/json.xb` and `lib/datetime.xb`.

### 17.1 JSON Module (`lib/json.xb`)

The `JSON` class provides static methods to validate, parse, and serialize JSON text:

```xlang
import "lib/json.xb"

// 1. Syntax validation without memory overhead
string payload = "{\"user\": \"alice\", \"score\": 100, \"active\": true}"
if (JSON.is_valid(payload)) {
    print("Valid JSON payload")
}

// 2. Parsing into Map structure
Map data = JSON.parse(payload)
print("User: %s", data.get("user"))     // alice
print("Score: %s", data.get("score"))   // 100

// 3. Serialization back to JSON
data.put("rank", "pro")
string serialized = JSON.stringify(data)
print("Serialized: %s", serialized)
data.free()
```

Low-level built-in functions `json_is_valid(str)`, `json_parse(str)`, and `json_stringify(map)` can also be invoked directly without importing `lib/json.xb`.

### 17.2 DateTime Module (`lib/datetime.xb`)

The `DateTime` class provides an object-oriented interface for calendar timestamps and high-resolution profiling:

```xlang
import "lib/datetime.xb"

// Instantiate with current system time
DateTime now = DateTime.now()

// Component getters
print("Year:   %d", now.year())
print("Month:  %d", now.month())
print("Day:    %d", now.day())
print("Hour:   %d", now.hour())
print("Minute: %d", now.minute())
print("Second: %d", now.second())

// Standard formatting (YYYY-MM-DD HH:MM:SS)
string dt_str = now.to_str()
print("Timestamp: %s", dt_str)

// Custom strftime formatting
string formatted = now.format("%Y/%m/%d %H:%M")
print("Custom: %s", formatted)

// High-resolution monotonic clock (milliseconds)
int t0 = DateTime.clock_ms()
// ... execution workload ...
int t1 = DateTime.clock_ms()
print("Elapsed: %d ms", t1 - t0)
```

Direct procedural built-ins are also available: `datetime_now()`, `datetime_year(ts)`, `datetime_month(ts)`, `datetime_day(ts)`, `datetime_hour(ts)`, `datetime_minute(ts)`, `datetime_second(ts)`, `datetime_format(ts, fmt)`, and `datetime_clock_ms()`.

---

## 18. Testing, Assertions & Build Profiles

xlang integrates built-in assertion primitives and dual build profiles (`--debug` vs `--release`), enabling robust unit testing during development without paying runtime penalties in production.

### 18.1 Built-in `assert` Primitive

`assert(condition, [message])` evaluates an expression. If the condition evaluates to `0` or `false`, the runtime immediately halts execution with an assertion failure:

```xlang
int x = 10
int y = 20

// Single argument assertion
assert(x < y)

// Two-argument assertion with descriptive failure message
assert(x + y == 30, "Sum of x and y must equal 30")
```

If an assertion fails, the engine outputs diagnostics and terminates:
```text
Assertion failed: Sum of x and y must equal 30
```

### 18.2 Debug vs. Release Profiles

- **Debug Mode (`--debug`, `-g` - Default)**:
  - All `assert(...)` statements are actively evaluated at runtime.
  - Full source line location, symbols, and diagnostics are preserved in VM bytecode and LLVM IR.
- **Release Mode (`--release`)**:
  - The compiler completely elides all `assert(...)` statements during parsing and code generation (zero AST nodes, zero VM opcodes, and zero LLVM IR instructions).
  - Produces optimized binaries with maximum execution throughput and minimal binary footprint.

Example CLI invocation:
```bash
# Debug execution with assertions enabled
xlang script.xb
xlang --vm script.xb

# Optimized release binary compilation (asserts stripped)
xlang build --release script.xb -o app
```

---

## 19. C Extensions SDK & Native Modules

xlang features a high-performance native C extension system based on a uniform **Vectorcall** calling convention (`XVectorFn`), defined in `include/xlang.h`. This allows seamless integration of C/C++ libraries (such as math accelerators, game engines, or OS bindings) with both the Bytecode VM and the LLVM AOT/JIT compiler.

### 19.1 The Extension SDK (`include/xlang.h`)

Native modules export the standard initialization entrypoint `xlang_module_init` using `XLANG_EXTENSION_ENTRY`:

```c
#include "xlang.h"

// 1. Direct C function
XLANG_EXPORT int fast_gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a < 0 ? -a : a;
}

// 2. Vectorcall wrapper for xlang VM and dynamic calls
static XValue vec_fast_gcd(XVm* vm, XValue receiver, int argc, const XValue* args) {
    (void)vm; (void)receiver;
    int a = (argc > 0 && args[0].type == XLANG_VAL_INT) ? (int)args[0].as.ival : 0;
    int b = (argc > 1 && args[1].type == XLANG_VAL_INT) ? (int)args[1].as.ival : 0;
    return xval_int(fast_gcd(a, b));
}

// 3. Function descriptors table
static const XExtensionFunc s_funcs[] = {
    { "gcd", vec_fast_gcd, 2, XLANG_TYPE_INT, { XLANG_TYPE_INT, XLANG_TYPE_INT } },
    { NULL, NULL, 0, XLANG_TYPE_VOID, { XLANG_TYPE_VOID } }
};

// 4. Module entrypoint
XLANG_EXPORT XLANG_EXTENSION_ENTRY {
    return ctx->register_module(ctx, "fastmath", s_funcs);
}
```

### 19.2 Loading Native Modules in xlang

Shared objects (`.so` on Linux, `.dylib` on macOS, `.dll` on Windows) are loaded using standard `import` syntax:

```xlang
import "fastmath"

int g = fastmath.gcd(48, 18)
print("GCD: %d\n", g) // 6
```

### 19.3 First-Class Native Integrations: Raylib 2D/3D Engine

The xlang repository includes complete Raylib bindings (`examples/c_extension/raylib/`), enabling rich real-time graphics, physics simulations, audio, and interactive game loops directly from xlang:

```xlang
import "examples/c_extension/raylib/raylib.so"

InitWindow(800, 600, "xlang + Raylib")
SetTargetFPS(60)

while (!WindowShouldClose()) {
    BeginDrawing()
    ClearBackground(245, 245, 245, 255)
    DrawText("Hello from xlang Native Engine!", 190, 200, 20, 20, 20, 20, 255)
    DrawCircle(400, 350, 40.0, 230, 41, 55, 255)
    EndDrawing()
}

CloseWindow()
```

### 19.4 Tier 2: Direct C-ABI FFI (Zero-Wrapper System Library Binding)

xlang provides a zero-wrapper dynamic C-ABI Foreign Function Interface that allows consuming unmodified existing shared libraries (such as `libm.so`, `libc.so`, `libsqlite3.so`, or any custom `.so` / `.dylib` / `.dll`) directly without writing glue code, C wrappers, or recompiling.

#### Declarative Syntax: `extern "<lib>" { ... }`
Functions exported by standard dynamic shared libraries can be declared using the `extern` block:

```xlang
extern "libm.so" {
    float sin(float x)
    float cos(float x)
    float pow(float base, float exp)
}

extern "libc.so.6" {
    int abs(int x)
    int atoi(string s)
    double atof(string s)
}
```

#### Calling Conventions & Dual Dispatch
Functions declared inside `extern` blocks are automatically registered:
1. **Direct Symbol Call**: Callable directly by name:
   ```xlang
   float s = sin(0.0)      // 0.0
   int val = abs(-42)      // 42
   ```
2. **Namespaced Module Call**: Also exposed under their module name (e.g. `libm.<func>`, `m.<func>`, `libc.<func>`):
   ```xlang
   float s2 = libm.sin(0.0)
   float c2 = m.cos(0.0)
   int v2 = libc.abs(-100)
   ```

#### Dynamic Frame Synthesis via `libffi`
During VM and bytecode execution, `extern` blocks are backed by:
- Dynamic symbol resolution (`dlopen` and `dlsym`).
- Dynamic call frame synthesis via `libffi` (`ffi_prep_cif` and `ffi_call`).
- Automated type marshalling between xlang `XValue` representations and native C types (`int`, `long`, `float`, `double`, `bool`, `char`, `string` / `char*`, `object` / `void*`).

### 19.5 Tier 3: LLVM AOT Native Compiler Integration (`xlang build`)

When producing standalone native binaries via `xlang build`, xlang eliminates all interpreter and FFI overhead by compiling C function calls directly to native assembly instructions via LLVM.

#### LLVM IR External Declarations
The compiler parses the function declarations from the `extern` block and emits exact LLVM IR external declarations at the top of the compilation unit:

```llvm
; Direct C-ABI FFI External Declarations
; From "libmylib.so"
declare i32 @c_custom_calc(i32, i32)
declare double @sin(double)
```

Direct call sites generate standard LLVM `call` instructions conforming to the platform ABI:
```llvm
%t1 = call i32 @c_custom_calc(i32 10, i32 5)
```

#### Linker Flag Forwarding & Automatic RPATH Baking
`xlang build` seamlessly integrates with the host Clang/LLVM linker:
- **Search Paths (`-L`) and Libraries (`-l`)**: Automatically passed to Clang:
  ```bash
  xlang build app.xb -L/usr/local/lib -lmylib -o app
  ```
- **Automated Library Detection**: Any library declared in an `extern "<lib>.so"` block is automatically tracked and translated to `-l<lib>` for Clang linkage.
- **Embedded RPATHs**: Linker flags automatically embed `-Wl,-rpath,'$ORIGIN'`, `-Wl,-rpath,'$ORIGIN/lib'`, and the `-L` search paths into the binary header, ensuring that standalone executables locate their dependencies at runtime without requiring manual `LD_LIBRARY_PATH` configuration.

---

## 20. Tooling, CLI Flags & Interactive Compiler Explorer

The `xlang` unified executable provides a comprehensive toolchain supporting tree-walk interpretation, bytecode compilation, virtual machine execution, LLVM native binary compilation, and interactive web visualization.

### 20.1 Command-Line Interface Usage

```text
xlang [options] <script.xb | bytecode.xbc> [args...]
xlang build [--debug|--release] <script.xb> [-o <binary>] [-L<dir>] [-l<lib>]
```

| Flag | Purpose | Description |
| :--- | :--- | :--- |
| `script.xb` | Interpreter | Executes script using the tree-walking runtime. |
| `--vm` | Virtual Machine | Compiles to bytecode in-memory and executes via the high-speed VM. |
| `--trace-vm` | VM Debugger | Executes in VM while printing every opcode instruction executed and stack state. |
| `-c`, `--compile` | Bytecode Compiler | Compiles script to persistent binary bytecode file (`.xbc`). |
| `-o <file>` | Output File | Specifies target path for output bytecode, LLVM IR, or compiled binary. |
| `--emit-llvm`, `-S` | LLVM IR Emitter | Generates textual LLVM IR (`.ll`) representation of the program. |
| `build` | Native Compiler | Compiles script to a standalone native ELF/Mach-O/PE executable via LLVM AOT. |
| `--release` | Release Profile | Generates optimized release binary with assertions elided and maximum optimization. |
| `--debug`, `-g` | Debug Profile | Builds native executable with debug symbols and runtime assertion checks enabled. |
| `-L<dir>` | Linker Path | Adds library search directory for native module linking. |
| `-l<lib>` | Link Library | Links native shared or static library into executable. |
| `--jit` | LLVM JIT Engine | Compiles and executes in-process using LLVM ORC JIT. |
| `--dump-ast` | AST Visualizer | Parses script and dumps structured Abstract Syntax Tree to terminal. |
| `--dump-ir` | IR Disassembler | Disassembles compiled bytecode instructions and constant pools. |
| `--view` | HTML Compiler Explorer | Generates a modern self-contained interactive HTML inspection tool (`xir_view.html`). |
| `--stats` | Profiler | Prints detailed execution metrics (elapsed time, memory usage, GC cycles). |

### 20.2 Interactive HTML Compiler & AST Explorer (`--view`)

Executing `xlang --view <script.xb>` generates a zero-dependency, self-contained HTML explorer (`xir_view.html`):
- **Split View**: Synchronized side-by-side view highlighting high-level source lines alongside their corresponding VM bytecode instructions.
- **AST Explorer**: Interactive hierarchical tree navigator displaying tokens, expression nodes, statement blocks, and class declarations with folding.
- **Bytecode Disassembly**: Full opcode listing with instruction hex offsets, register operands, jump targets, and constant pool indices.
- **LLVM IR Tab**: Syntax-highlighted LLVM Intermediate Representation generated by the compiler.
- **Symbol & Constant Tables**: String tables, class prototypes, field offsets, and function signatures.
- **Analytics & Metrics**: Opcodes distribution chart, heap memory stats, and compiler timing breakdowns.

---

## 21. Formal PEG Grammar Reference

The formal syntax of xlang is specified in a machine-readable Parsing Expression Grammar located at [`grammar/xlang.peg`](file:///home/xobyx/xlang/grammar/xlang.peg).

Key grammatical rules include:
- **Lexical Tokens**: Whitespace `_`, keywords (`class`, `new`, `static`, `if`, `while`, `for`, `do`, `return`, `break`, `continue`, `import`, `in`), identifiers `[a-zA-Z_][a-zA-Z0-9_]*`, integer, float, string, and character literals.
- **Expressions & Precedence**: Primary, postfix (`.`, `()`, `[]`), unary (`!`, `-`, `~`, `++`, `--`), multiplicative (`*`, `/`, `%`), additive (`+`, `-`), bitwise shifts (`<<`, `>>`), relational (`<`, `<=`, `>`, `>=`), equality (`==`, `!=`), bitwise AND/XOR/OR (`&`, `^`, `|`), logical AND (`&&`), logical OR (`||`), and assignment (`=`, `+=`, `-=`, etc.).
- **Object Model**: Class declarations with single inheritance, fields, instance methods, static methods, static properties, and constructors.
- **Instantiation**: Support for both declaration syntax `ClassName var(args)` and operator syntax `new ClassName(args)`.
- **Iteration Constructs**: Standard C-style 3-part for loops `for (int i=0, i++, i<5)`, iterator for loops `for (item in collection)`, stepper loops `for i (0, i+1, i<5)`, while loops, and do-while loops.

---

## 22. Comprehensive Modern Example

```xlang
import "lib/json.xb"
import "lib/datetime.xb"

// 1. Class definition with constructor, fields, methods, and static factory
class Vector2D() {
    int x
    int y

    Vector2D() {
        this.x = 0
        this.y = 0
    }

    Vector2D(int a, int b) {
        this.x = a
        this.y = b
    }

    static Vector2D origin() {
        return new Vector2D(0, 0)
    }

    int magnitude_squared() {
        return (this.x * this.x) + (this.y * this.y)
    }

    Vector2D add(Vector2D other) {
        return new Vector2D(this.x + other.x, this.y + other.y)
    }
}

// 2. Object creation via 'new' and method invocation
Vector2D v1 = new Vector2D(3, 4)
assert(v1.magnitude_squared() == 25, "Vector magnitude check")

Vector2D v2 = new Vector2D(1, 2)
Vector2D v3 = v1.add(v2)
assert(v3.x == 4 && v3.y == 6, "Vector addition check")

// 3. String indexing and character validation
string greeting = "Hello, xlang!"
assert(greeting[0] == "H", "First character check")
assert(strlen(greeting) == 13, "Length check")

// 4. DateTime and JSON integration
DateTime now = DateTime.now()
print("Execution Timestamp: %s", now.to_str())

string json_data = "{\"project\": \"xlang\", \"version\": \"2.0\"}"
if (JSON.is_valid(json_data)) {
    Map m = JSON.parse(json_data)
    print("Loaded project: %s", m.get("project"))
    m.free()
}

// 5. C-style for loop with compound assignment
int total = 0
for (int i = 0, i++, i < 5) {
    total += i
}
assert(total == 10, "Sum verification")

print("All verifications succeeded!")
```
