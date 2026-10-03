#!/bin/bash


BIN="./bin/Release/xlang"
if [ ! -f "$BIN" ]; then
    make
fi

echo "Running xlang test suite..."
PASSED=0
TOTAL=0

echo "llllllllllll"
for test_file in tests/*.xb; do
    # Skip tests dedicated to VM-specific runtime features (e.g., dynamic closures, reflection/eval) or release profile
    if [ "$test_file" = "tests/test_assert_release.xb" ] || \
       [ "$test_file" = "tests/test_vectorcall.xb" ] || \
       [ "$test_file" = "tests/test_vm_functions.xb" ]; then
        continue
    fi

    TOTAL=$((TOTAL + 1))
    base="${test_file%.*}"
    bin_out="${base}_bin"
    ll_out="${test_file}.ll"

    echo "----------------------------------------"
    echo "Running $test_file (native build & run)..."

    # Step 1: Compile native binary
    if ! $BIN build --debug "$test_file" -o "$bin_out" > /dev/null 2>&1; then
        echo "FAIL: $test_file (native compilation failed)"
        $BIN build --debug "$test_file" -o "$bin_out"
        #rm -f "$bin_out" "$ll_out"
        exit 1
    fi

    # Step 2: Execute compiled binary
    if "./$bin_out" > /dev/null 2>&1; then
        echo "PASS: $test_file (native)"
        PASSED=$((PASSED + 1))
        rm -f "$bin_out" "$ll_out"
    else
        echo "FAIL: $test_file (native execution runtime error)"
        "./$bin_out"
        #rm -f "$bin_out" "$ll_out"
        exit 1
    fi
done
