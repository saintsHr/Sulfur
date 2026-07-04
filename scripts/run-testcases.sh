#!/bin/bash

shopt -s nullglob

BIN="./build/sulfur"
CASES_DIR="./testcases"
TMP_OUT="/tmp/testcases.asm"

trap 'rm -f "$TMP_OUT"' EXIT

total=0
passed=0
failed_names=()

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

files=("$CASES_DIR"/*.slfr)
if [[ ${#files[@]} -eq 0 ]]; then
    echo "No test cases found in $CASES_DIR."
    exit 1
fi

if [[ ! -x "$BIN" ]]; then
    echo "No compiler found in $BIN"
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

for file in "${files[@]}"; do
    ((total++))
    name=$(basename "$file")

    "$BIN" -i "$file" -o "$TMP_OUT" > /dev/null 2>&1
    exit_code=$?

    if [[ "$name" == *_fail* ]]; then
        expected="fail"
    elif [[ "$name" == *_pass* ]]; then
        expected="pass"
    else
        echo "invalid name: $name (use the suffix _fail or _pass)"
        exit 1
    fi

    if [[ "$expected" == "pass" && $exit_code -eq 0 ]]; then
        echo "[PASS] $name"
        ((passed++))
    elif [[ "$expected" == "fail" && $exit_code -ne 0 ]]; then
        echo "[PASS] $name"
        ((passed++))
    else
        echo "[FAIL] $name"
        failed_names+=("$name")
    fi
done

echo ""

if [[ ${#failed_names[@]} -ne 0 ]]; then
    echo "Failed Tests:"
    printf ' - %s\n' "${failed_names[@]}"
fi

echo ""
echo "Total: $total | Pass: $passed | Fail: $((total-passed))"
echo ""

if [[ ${#failed_names[@]} -eq 0 ]]; then
    echo -e "\033[0;32m$(print_pass)\033[0m"
else
    echo -e "\033[0;31m$(print_fail)\033[0m"
fi

echo ""