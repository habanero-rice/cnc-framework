#!/bin/bash

# Guess XSTACK_ROOT and UCNC_ROOT if they're not already set
export XSTACK_ROOT="${XSTACK_ROOT-${PWD%/xstack/*}/xstack}"
export UCNC_ROOT="${UCNC_ROOT-${XSTACK_ROOT}/hll/cnc}"
UCNC_BIN="${UCNC_ROOT}/bin"

# Sanity check on the above paths
if ! [ -f "${UCNC_ROOT}/bin/ucnc_t" ]; then
    echo "ERROR: Failed to find UCNC_ROOT (looking in '${UCNC_ROOT}')"
    exit 1
fi

# Run the translator if needed
[ -d "cnc_support" ] || "${UCNC_BIN}/ucnc_t"

# Build
make || exit 1

PREFIX="Result matrix checksum"
EXPECTED="$PREFIX: d5ff728615a593f"

OUTPUT="$(make run WORKLOAD_ARGS="2500 ${TILE:-125}")"

echo "$OUTPUT"

if (echo "$OUTPUT" | fgrep -q "$EXPECTED"); then
    echo checksum OK
else
    echo FAILED
    exit 1
fi

