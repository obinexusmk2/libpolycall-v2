#!/bin/bash
cd "$(dirname "$0")"
PASS=0
FAIL=0
for f in core/src/core/*.c core/src/micro/*.c core/src/socket/*.c; do
    if [ ! -f "$f" ]; then continue; fi
    printf "  %-55s " "$f"
    output=$(gcc -fsyntax-only -Wall -Wextra -I core/include -I core/include/libpolycall "$f" 2>&1)
    if [ $? -eq 0 ]; then
        echo "OK"
        PASS=$((PASS+1))
    else
        echo "FAIL"
        echo "$output" | head -5
        FAIL=$((FAIL+1))
    fi
done
echo ""
echo "Results: $PASS passed, $FAIL failed"
