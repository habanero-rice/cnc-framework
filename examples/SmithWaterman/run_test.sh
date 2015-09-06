#!/bin/bash

TMPFILE=.tmp.out

# Guess XSTACK_ROOT and UCNC_ROOT if they're not already set
export XSTACK_ROOT="${XSTACK_ROOT-${PWD%/xstack/*}/xstack}"
export UCNC_ROOT="${UCNC_ROOT-${XSTACK_ROOT}/hll/cnc}"

### Setup ###

# Sanity check on the above paths
if ! [ -f "${UCNC_ROOT}/bin/ucnc_t" ]; then
    echo "ERROR: Failed to find UCNC_ROOT (looking in '${UCNC_ROOT}')"
    exit 1
fi

# Run the translator if needed
[ -d "cnc_support" ] || "${UCNC_ROOT}/bin/ucnc_t"

# Build
make || exit 1

DATA_DIR="${XSTACK_ROOT}/apps/smithwaterman/datasets"

### Test 1 (small) ###

echo "Testing string1-small.txt vs string2-small.txt with width=3, height=3"
make run WORKLOAD_ARGS="3 3 ${DATA_DIR}/string1-small.txt ${DATA_DIR}/string2-small.txt" 2>&1 | tee $TMPFILE && fgrep -q 'score: 10' < $TMPFILE
RET1=$?
[ $RET1 = 0 ] && echo OK || echo FAIL

### Test 2 (large) ###

# str1 length factors: 3*59*569
DIM1=$((569))
# str2 length factors: 3*3*17*661
DIM2=$((661))

echo "Testing string1-large.txt vs string2-large.txt with width=${DIM1}, height=${DIM2}"
make run WORKLOAD_ARGS="${DIM1} ${DIM2} ${DATA_DIR}/string1-large.txt ${DATA_DIR}/string2-large.txt" 2>&1 | tee $TMPFILE && fgrep -q 'score: 65386' < $TMPFILE
RET2=$?
[ $RET2 = 0 ] && echo OK || echo FAIL

### Cleanup ###

rm $TMPFILE

[ $RET1 = 0 ] && [ $RET2 = 0 ]
