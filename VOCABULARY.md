# XLANG Language Vocabulary & Syntax Reference

This document provides a comprehensive reference of the vocabulary, grammar, type system, built-in functions, and language constructs for **xlang**.

---

## 1. Lexical Structure

### 1.1 Comments
Single-line comments begin with `#`. Everything following `#` on that line is ignored by the compiler.
```xlang
# This is a single-line comment
int x = 10 # Inline comment
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

### 1.4 Statement Terminators
Statements can be terminated either by a newline or by a semicolon (`;`).
```xlang
int a = 1; int b = 2
int c = 3
```

---

## 2. Keywords

xlang defines 11 reserved keywords:

| Keyword | Description | Example Syntax |
| :--- | :--- | :--- |
| `if` | Conditional branch execution | `if (x == 10) { ... }` |
| `eif` | "Else if" conditional branch | `eif (x > 10) { ... }` |
| `else` | Fallback branch for `if`/`eif` | `else { ... }` |
| `while` | Loop while condition is true | `while (i < 5) { ... }` |
| `for` | 3-part stepper loop | `for i (0, i + 1, i < 5) { ... }` |
| `do` | Do-while loop construct | `do { ... } while (i < 5)` |
| `return` | Return from function with optional value | `return a + b` |
| `break` | Terminate loop execution | `break` |
| `class` | Define a user-defined class / type | `class Point(Base) { ... }` |
| `static` | Static method or property declaration inside class | `static int add(int a, int b)`, `static int count` |
| `import` | Compile-time multi-file modular import | `import "math.xb"`, `import collections` |

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

### 3.2 Array Types
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

### 3.3 User-Defined Types (Classes)
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

### 4.4 Assignment Operator
| Operator | Description | Example |
| :---: | :--- | :--- |
| `=` | Assigns value on RHS to variable on LHS | `x = 10`, `obj.field = 20` |

### 4.5 Member Access & Grouping
| Symbol | Name | Description |
| :---: | :--- | :--- |
| `.` | Dot operator | Access object properties and methods (`obj.prop`, `obj.method()`) |
| `( )` | Parentheses | Parameter lists, argument lists, expression grouping, loop specs |
| `[ ]` | Brackets | Array declarations and index subscripting (`arr[i]`) |
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
In xlang, the `for` loop syntax specifies an existing iterator variable, initialization value, step expression, and termination condition:
`for <var> (<init>, <step_expression>, <condition>) { ... }`

> **Note:** The iterator variable `<var>` must be declared prior to the loop (e.g. `int i = 0`).

```xlang
int i = 0
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

## 17. Comprehensive Example

```xlang
# Define a 2D Vector class with constructors, methods, and inheritance
class Vector2D() {
    int x
    int y

    Vector2D() {
        x = 0
        y = 0
    }

    Vector2D(int a, int b) {
        x = a
        y = b
    }

    int magnitude_squared() {
        return (x * x) + (y * y)
    }
}

# Instantiation with constructor arguments
Vector2D v1(3, 4)
print("Magnitude squared: %d", v1.magnitude_squared())

# Conditional check
if (v1.magnitude_squared() == 25) {
    print("Vector is 3-4-5 triangle")
} else {
    print("Other vector")
}

# Stepper loop
int i = 0
for i (0, i + 1, i < 3) {
    print("Step: %d", i)
}
```
