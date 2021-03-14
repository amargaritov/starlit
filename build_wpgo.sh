#!/bin/bash -x

SEED="$1"
UPDATE_LIMIT="$2"

rm -rf pgo_data 
mkdir -p pgo_data 

CFLAGS_DEFINES="-DSEED=$SEED -DUPDATE_LIMIT=$UPDATE_LIMIT"
make CFLAGS_DEFINES="$CFLAGS_DEFINES" prof_gen -j 

./cmix -c ./prof_input/input ./prof_comp > ./prof_output
rm ./prof_comp ./prof_output 
llvm-profdata merge -output=default.profdata ./pgo_data/*
mv default.profdata pgo_data/

make CFLAGS_DEFINES="$CFLAGS_DEFINES" COREI7=1 prof_use -j
