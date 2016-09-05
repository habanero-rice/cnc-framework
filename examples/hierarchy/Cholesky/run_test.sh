#!/bin/bash

# Guess XSTG_ROOT and UCNC_ROOT if they're not already set
export XSTG_ROOT="${XSTG_ROOT-${PWD%/xstg/*}/xstg}"
export UCNC_ROOT="${UCNC_ROOT-${XSTG_ROOT}/apps/hll/cnc}"
UCNC_BIN="${UCNC_ROOT}/bin"

# Sanity check on the above paths
if ! [ -f "${UCNC_ROOT}/bin/ucnc_t" ]; then
    echo "ERROR: Failed to find UCNC_ROOT (looking in '${UCNC_ROOT}')"
    exit 1
fi

# Run the translator if needed
[ -d "cnc_support" ] || "${UCNC_BIN}/ucnc_t"

# Build
make install || exit 1

PREFIX="Result matrix checksum"
EXPECTED="$PREFIX: 69f72511c89b57d9"

OUTPUT="$(make run WORKLOAD_ARGS="2500 ${TILE:-125}")"

echo "$OUTPUT"

if (echo "$OUTPUT" | fgrep -q "$EXPECTED"); then
    echo checksum OK
else
    echo FAILED
    exit 1
fi

