# Only execute from the installation directory
if [ -f setup_env.sh ] && [ -f ./bin/ucnc_t ]; then
    if [ -d "$XSTG_ROOT" ]; then
        echo 'Using existing $XSTG_ROOT (CnC-OCR)'
    elif [ -d "$OCR_INSTALL_ROOT" ]; then
        echo 'Using existing $OCR_INSTALL_ROOT (CnC-OCR)'
    elif [ "$(basename $(dirname $PWD))" = hll ]; then
        # assume CnC-OCR is in xstg/apps/hll/cnc
        export XSTG_ROOT=$(dirname $(dirname $(dirname $PWD)))
        echo 'Set $XSTG_ROOT'
    elif [ -d "$CNCROOT" ]; then
        echo 'Using existing $CNCROOT (iCnC)'
    else
        cat <<EOF
The CnC framework can't locate a compatible runtime backend.
You will need to manually configure some environment variables.
Please see the CnC framework's readme for setup instructions.
EOF
    fi

    export UCNC_ROOT=$PWD
    echo 'Set $UCNC_ROOT'

    export PATH=$UCNC_ROOT/bin:$PATH
    echo 'Updated $PATH'
    echo 'Setup complete.'
else
    echo 'ERROR! You must source setup_env.sh from the CnC framework installation directory.'
fi
