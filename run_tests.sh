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
    if [ "$test_file" = "tests/test_assert_release.xb" ]; then
        continue
    fi
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
echo "Running assertion profile tests (debug vs. release)..."

TOTAL=$((TOTAL + 1))
echo "Running tests/test_assert_release.xb (debug mode should catch assert)..."
if ! $BIN "tests/test_assert_release.xb" > /dev/null 2>&1; then
    echo "PASS: debug tree-walk caught assertion"
    PASSED=$((PASSED + 1))
else
    echo "FAIL: debug tree-walk should have caught assertion"
    exit 1
fi

TOTAL=$((TOTAL + 1))
echo "Running tests/test_assert_release.xb (debug VM should catch assert)..."
if ! $BIN --vm "tests/test_assert_release.xb" > /dev/null 2>&1; then
    echo "PASS: debug VM caught assertion"
    PASSED=$((PASSED + 1))
else
    echo "FAIL: debug VM should have caught assertion"
    exit 1
fi

TOTAL=$((TOTAL + 1))
echo "Running tests/test_assert_release.xb (--release tree-walk should elide assert)..."
if $BIN --release "tests/test_assert_release.xb" > /dev/null 2>&1; then
    echo "PASS: release tree-walk elided assertion"
    PASSED=$((PASSED + 1))
else
    echo "FAIL: release tree-walk failed"
    $BIN --release "tests/test_assert_release.xb"
    exit 1
fi

TOTAL=$((TOTAL + 1))
echo "Running tests/test_assert_release.xb (--vm --release should elide assert)..."
if $BIN --vm --release "tests/test_assert_release.xb" > /dev/null 2>&1; then
    echo "PASS: release VM elided assertion"
    PASSED=$((PASSED + 1))
else
    echo "FAIL: release VM failed"
    $BIN --vm --release "tests/test_assert_release.xb"
    exit 1
fi

TOTAL=$((TOTAL + 1))
echo "Running native build --release tests/test_assert_release.xb..."
if $BIN build --release tests/test_assert_release.xb -o tests/test_assert_rel > /dev/null 2>&1 && ./tests/test_assert_rel > /dev/null 2>&1; then
    echo "PASS: native release binary elided assertion"
    PASSED=$((PASSED + 1))
    rm -f tests/test_assert_rel tests/test_assert_release.xb.ll
else
    echo "FAIL: native release binary failed"
    rm -f tests/test_assert_rel tests/test_assert_release.xb.ll
    exit 1
fi

TOTAL=$((TOTAL + 1))
echo "Running native build --debug tests/test_assert_release.xb..."
if $BIN build --debug tests/test_assert_release.xb -o tests/test_assert_dbg > /dev/null 2>&1; then
    if ! ./tests/test_assert_dbg > /dev/null 2>&1; then
        echo "PASS: native debug binary caught assertion"
        PASSED=$((PASSED + 1))
        rm -f tests/test_assert_dbg tests/test_assert_release.xb.ll
    else
        echo "FAIL: native debug binary should have failed on assert(false)"
        rm -f tests/test_assert_dbg tests/test_assert_release.xb.ll
        exit 1
    fi
else
    echo "FAIL: native debug compilation failed"
    rm -f tests/test_assert_dbg tests/test_assert_release.xb.ll
    exit 1
fi

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
