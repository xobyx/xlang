#!/bin/bash
set -e

BIN="./bin/Release/xlang"
if [ ! -f "$BIN" ]; then
    make
fi

echo "Running xlang test suite..."
PASSED=0
TOTAL=0

for test_file in tests/*.xb; do
    TOTAL=$((TOTAL + 1))
    echo "----------------------------------------"
    echo "Running $test_file (tree-walk)..."
    if $BIN "$test_file" > /dev/null 2>&1; then
        echo "PASS: $test_file (tree-walk)"
        PASSED=$((PASSED + 1))
    else
        echo "FAIL: $test_file (tree-walk)"
        $BIN "$test_file"
        exit 1
    fi

    TOTAL=$((TOTAL + 1))
    echo "Running $test_file (--vm)..."
    if $BIN "$test_file" --vm > /dev/null 2>&1; then
        echo "PASS: $test_file (--vm)"
        PASSED=$((PASSED + 1))
    else
        echo "FAIL: $test_file (--vm)"
        $BIN "$test_file" --vm
        exit 1
    fi
done

echo "----------------------------------------"
echo "Running disallowed syntax negative tests..."
for dis_file in tests/disallowed/*.xb; do
    TOTAL=$((TOTAL + 1))
    echo "Running (negative) $dis_file..."
    if $BIN "$dis_file" > /dev/null 2>&1 || $BIN --vm "$dis_file" > /dev/null 2>&1; then
        echo "FAIL: $dis_file (expected rejection with exit code 1)"
        exit 1
    else
        echo "PASS: $dis_file (correctly rejected in both tree-walk and VM modes)"
        PASSED=$((PASSED + 1))
    fi
done

echo "========================================"
echo "Results: $PASSED / $TOTAL passed"
