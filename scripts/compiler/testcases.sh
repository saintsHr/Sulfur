#!/bin/bash

shopt -s nullglob

CASES_DIR="./testcases/compiler"
TMP_OUT="/tmp/testcases.asm"

trap 'rm -f "$TMP_OUT"' EXIT

total=0
passed=0
failed_names=()
BINS=()

print_pass() {
cat << "EOF"
    ____   ___    _____ _____
   / __ \ /   |  / ___// ___/
  / /_/ // /| |  \__ \ \__ \
 / ____// ___ | ___/ /___/ /
/_/    /_/  |_|/____//____/
EOF
}

print_fail() {
cat << "EOF"
    ______ ___     ____ __
   / ____//   |   /  _// /
  / /_   / /| |   / / / /
 / __/  / ___ | _/ / / /___
/_/    /_/  |_|/___//_____/
EOF
}

for mode in debug release; do
    bin="./build/$mode/compiler/sulfur-compiler"

    if [[ -x "$bin" ]]; then
        BINS+=("$mode:$bin")
    fi
done

echo ""

if [[ ${#BINS[@]} -eq 0 ]]; then
    echo "No compiler found in debug or release builds."
    exit 1
fi

mapfile -t files < <(
    find "$CASES_DIR" -type f -name "*.slfr" | sort
)

if [[ ${#files[@]} -eq 0 ]]; then
    echo "No test cases found in $CASES_DIR."
    exit 1
fi

invalid=0

for file in "${files[@]}"; do
    name=$(basename "$file")

    if [[ "$name" != *_pass* && "$name" != *_fail* ]]; then
        echo "Invalid test name: $name"
        invalid=1
    fi
done

if [[ $invalid -eq 1 ]]; then
    echo ""
    echo "Fix test names before running."
    exit 1
fi

for entry in "${BINS[@]}"; do
    mode="${entry%%:*}"
    BIN="${entry#*:}"

    echo "========================================"
    echo " Running $mode: $BIN"
    echo "========================================"

    mode_total=0
    mode_passed=0
    mode_failed=()

    for file in "${files[@]}"; do
        ((mode_total++))
        name=$(basename "$file")

        if [[ "$name" == *_fail* ]]; then
            expected="fail"
        else
            expected="pass"
        fi

        "$BIN" -i "$file" -o "$TMP_OUT" \
            > /dev/null 2>&1
        exit_code=$?

        if [[ "$expected" == "pass" && $exit_code -eq 0 ]] ||
           [[ "$expected" == "fail" && $exit_code -ne 0 ]]; then
            echo "[PASS] [$mode] $name"
            ((mode_passed++))
        else
            echo "[FAIL] [$mode] $name"
            mode_failed+=("$name")
        fi
    done

    echo ""
    echo "[$mode] Total: $mode_total | Pass: $mode_passed | Fail: $((mode_total - mode_passed))"

    if [[ ${#mode_failed[@]} -ne 0 ]]; then
        echo "[$mode] Failed tests:"
        printf ' - %s\n' "${mode_failed[@]}"
    fi

    total=$((total + mode_total))
    passed=$((passed + mode_passed))
    failed_names+=("${mode_failed[@]}")
    echo ""
done

echo "========================================"
echo " Final Summary"
echo "========================================"
echo "Total: $total | Pass: $passed | Fail: $((total - passed))"
echo ""

if [[ ${#failed_names[@]} -eq 0 ]]; then
    echo -e "\033[0;32m$(print_pass)\033[0m"
else
    echo -e "\033[0;31m$(print_fail)\033[0m"
fi

echo ""

if [[ ${#failed_names[@]} -ne 0 ]]; then
    exit 1
fi

exit 0
