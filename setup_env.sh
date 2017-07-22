# get the absolute dirname of the current file
BASE_PATH=$(cd $(dirname "$_"); pwd)
# Only execute from the installation directory
if [ -d "$XSTG_ROOT" ]; then
    echo 'Using existing $XSTG_ROOT (CnC-OCR)'
elif [ -d "$OCR_INSTALL_ROOT" ]; then
    echo 'Using existing $OCR_INSTALL_ROOT (CnC-OCR)'
elif [ "$(basename $(dirname $BASE_PATH))" = hll ]; then
    # assume CnC-OCR is in xstg/apps/hll/cnc
    export XSTG_ROOT=$(dirname $(dirname $(dirname "$BASE_PATH")))
    echo 'Set $XSTG_ROOT'
elif [ -d "$CNCROOT" ]; then
    echo 'Using existing $CNCROOT (iCnC)'
    echo 'Set $UCNC_PLATFORM'
    export UCNC_PLATFORM=icnc
else
    cat <<EOF
The CnC framework can't locate a compatible runtime backend.
You will need to manually configure some environment variables.
Please see the CnC framework's readme for setup instructions.
EOF
fi

export UCNC_ROOT="$BASE_PATH"
echo 'Set $UCNC_ROOT'

export PATH="$UCNC_ROOT/bin:$PATH"
echo 'Updated $PATH'
echo 'Setup complete.'

