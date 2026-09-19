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
    echo "Running $test_file..."
    if $BIN "$test_file" > /dev/null 2>&1; then
        echo "PASS: $test_file"
        PASSED=$((PASSED + 1))
    else
        echo "FAIL: $test_file"
        $BIN "$test_file"
        exit 1
    fi
done

echo "========================================"
echo "Results: $PASSED / $TOTAL passed"
