#!/bin/bash

TEST_ROOT="${UCNC_ROOT-"${XSTACK_ROOT?Missing UCNC_ROOT or XSTACK_ROOT environment variable}/hll/cnc"}/test"
TOTAL=0
FAILURES=0
TEST_LOG="$TEST_ROOT/test.log"

# Clear test log
echo -n > "$TEST_LOG"

# Run tests in all sub-directories
cd "$TEST_ROOT"
for d in *; do
    # Skip non-directories
    [ -d "$d" ] || continue
    TOTAL=$(($TOTAL + 1))

    echo ">>> Running test $d" | tee -a "$TEST_LOG"

    if true; then
        cd $d && ${CNC_T:-ucnc_t} && ./implementSteps.sh
    fi &>> "$TEST_LOG"
    RES1="$?"
    EXPECTED_OUTPUT=`tail -n1 README`
    [ $RES1 = 0 ] && make run 2>&1 | tee -a "$TEST_LOG" | fgrep -q "$EXPECTED_OUTPUT"
    RES2="$?"

    if [ $RES2 = 0 ]; then
        echo $'    OK\n' | tee -a "$TEST_LOG"
    else
        echo ">>> Expected: $EXPECTED_OUTPUT" >> "$TEST_LOG"
        echo $'    FAILED\n' | tee -a "$TEST_LOG"
        FAILURES=$(($FAILURES + 1))
    fi

    cd "$TEST_ROOT"
done

echo "Ran $TOTAL tests, with $FAILURES failures."
echo "See '$TEST_LOG' for details."

# Only successful if there were no failures
[ $FAILURES = 0 ]
