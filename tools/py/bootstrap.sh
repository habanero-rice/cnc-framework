#!/bin/bash

ROOT="${UCNC_ROOT-"${XSTG_ROOT?Missing UCNC_ROOT or XSTG_ROOT environment variable}/apps/hll/cnc"}"

echo "Installing CnC translator dependencies..."

CLUSTER_CACHE=/opt/rice/share/cnc-ocr-deps-cache
PYPI_SRC="https://pypi.python.org/packages/source"
VENV="virtualenv-1.11.6"

# Figure out what to use as "curl" for downloading stuff
if type curl &>/dev/null; then
    CURL_CMD="curl -s"
elif type wget &>/dev/null; then
    CURL_CMD="wget -q -O-"
else
    echo "ERROR: cannot find curl or wget commands"
    exit 1
fi

install_dep() {
    URL="$1"
    LOCAL_PATH="${CLUSTER_CACHE}/$(basename $URL)"
    echo
    if [ -f "$LOCAL_PATH" ]; then
        echo "Installing from $LOCAL_PATH"
        easy_install "$LOCAL_PATH"
    else
        echo "Installing from $URL"
        easy_install "$URL"
    fi
}

get_venv() {
    URL="$PYPI_SRC/v/virtualenv/${VENV}.tar.gz"
    LOCAL_PATH="${CLUSTER_CACHE}/$(basename $URL)"
    if [ -f "$LOCAL_PATH" ]; then
        tar xzf "$LOCAL_PATH"
    else
        $CURL_CMD "$URL" | tar xz
    fi
}

cd $ROOT/tools/py/

if ! [ -d venv ]; then
    PY=${UCNC_PYTHON:-python}
    CUSTOM_PY="-p $PY"
    (   get_venv \
        && $PY $VENV/virtualenv.py $CUSTOM_PY --no-site-packages venv \
        && source venv/bin/activate \
        && install_dep "$PYPI_SRC/p/pyparsing/pyparsing-2.0.2.tar.gz" \
        && install_dep "$PYPI_SRC/J/Jinja2/Jinja2-2.7.3.tar.gz" \
        && install_dep "$PYPI_SRC/a/argparse/argparse-1.2.1.tar.gz" \
        && install_dep "$PYPI_SRC/o/ordereddict/ordereddict-1.1.tar.gz" \
        && install_dep "$PYPI_SRC/C/Counter/Counter-1.0.0.tar.gz" \
        && install_dep "$PYPI_SRC/s/sympy/sympy-0.7.6.tar.gz" \
        && touch .depsOK
    ) &> setup.log
fi

if ! [ -f .depsOK ]; then
    cat <<EOF
ERROR! Failed to set up python environment.
See $ROOT/tools/py/setup.log for details.
EOF
    exit 1
fi

echo $PATH | fgrep -q "$ROOT" || echo "NOTE: You should add UCNC_ROOT to your PATH."
echo 'Installation complete!'
