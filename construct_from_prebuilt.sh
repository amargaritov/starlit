#!/bin/bash -x

BINARY="$1"
if [ -z "$BINARY" ]; then 
	echo "Please provide a path to prebuilt cmix compressor binary"
	exit 1
fi

# this is a directory where the compressor binary will be placed 
DIR=run
mkdir -p ./$DIR
ROOT=$(pwd)
cp $BINARY $DIR/cmix_orig
git diff > $DIR/patch

# building a selfextracting binary 
pushd $DIR
# creating a compressed version of dictionary
./cmix_orig -c $ROOT/dictionary/english.dic ./comp_dict
# creating a compressed verions of a file with new order 
./cmix_orig -c $ROOT/src/readalike_prepr/data/new_article_order ./comp_order
# creating a header with size of the above files
./cmix_orig -h $(wc -c ./comp_dict | awk '{print $1}') $(wc -c ./comp_order | awk '{print $1}') 0

# merging the above files and setting permissions for the final executable file
cat ./cmix_orig ./comp_dict ./comp_order header.dat > ./cmix
chmod +x ./cmix
