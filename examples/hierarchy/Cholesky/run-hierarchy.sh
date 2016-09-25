#!/bin/bash

# intel mpi setup
source /opt/intel/tools/impi/5.1.1.109/bin64/mpivars.sh

# icnc setup
source /opt/rice/intel/cnc/1.0.100/bin/cncvars.sh

# ocr setup
export XSTG_ROOT=/home/nbvrvilo/local/sources/xstg

# habanero cnc setup
cd /home/nbvrvilo/projects/hierarchy
source setup_env.sh

# cholesky
cd /home/nbvrvilo/projects/hierarchy/examples/hierarchy/Cholesky

WORK_ROOT=`mktemp -d`
[ -d $WORK_ROOT ] || exit 1

export MPI_HOSTS=$WORK_ROOT/mpihosts.txt
export WORKLOAD_BUILD=$WORK_ROOT/build
export WORKLOAD_INSTALL=$PWD/$PBS_JOBID-install

cat $PBS_NODEFILE | sort -u > $MPI_HOSTS

for C in 0 i; do
    for T in 0 i r; do
        for U in 0 i r c; do
            if [ $C = 0 -o $T = 0 -o $U = 0 ] && [ $C$T$U != 000  ]; then
                # only do 0 (static) for all 3 collections
                echo SKIPPING $C,$T,$U
            else
                for SZ in 1 4; do
                    if [ $C = 0 -a $SZ != 1 ]; then
                        # don't tile the static version
                        echo SKIPPING $C,$T,$U tiled $SZ
                    else
                        echo $C,$T,$U,$SZ
                        make -f Makefile.icnc-mpi clean install C_TAG=$C T_TAG=$T U_TAG=$U C_CHUNK_SIZE=$SZ T_CHUNK_SIZE=$SZ U_CHUNK_SIZE=$SZ
                        for x in {1..5}; do
                            time timeout 5m make -f Makefile.icnc-mpi run WORKLOAD_ARGS="8100 50"
                        done
                    fi
                done
            fi
        done
    done
done &> out.$PBS_JOBID.txt

rm -rf $WORK_ROOT $WORKLOAD_INSTALL

