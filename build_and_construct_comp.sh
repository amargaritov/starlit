#!/bin/bash -x

#SEED="$1"
#UPDATE_LIMIT="$2"

#SEED="3198"
SEED="9489"
UPDATE_LIMIT="3000"

rm -rf pgo_data 
mkdir -p pgo_data 

CFLAGS_DEFINES="-DSEED=$SEED -DUPDATE_LIMIT=$UPDATE_LIMIT"
make CFLAGS_DEFINES="$CFLAGS_DEFINES" prof_gen -j 

./cmix -c ./prof_input/input ./prof_comp > ./prof_output
rm ./prof_comp ./prof_output 
llvm-profdata-12 merge -output=default.profdata ./pgo_data/*
mv default.profdata pgo_data/

make CFLAGS_DEFINES="$CFLAGS_DEFINES" prof_use -j
upx-ucl -9 cmix 

DIR=run
mkdir -p ./$DIR
ROOT=$(pwd)
cp ./cmix $DIR/cmix_orig
git diff > $DIR/patch

pushd $DIR
./cmix_orig -c $ROOT/dictionary/english.dic ./comp_dict
./cmix_orig -c $ROOT/src/readalike_prepr/data/new_article_order ./comp_order
./cmix_orig -h $(wc -c ./comp_dict | awk '{print $1}') $(wc -c ./comp_order | awk '{print $1}') 0

cat ./cmix_orig ./comp_dict ./comp_order header.dat > ./cmix
chmod +x ./cmix




# run comp
#pushd tmp
#./cmix -e ./prof_input/enwik9 comp & pid=$!
#echo $pid > pid 
#psrecord --log acivity.log --interval 1 $pid & 
#popd
