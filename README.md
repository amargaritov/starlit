# Hutter Prize Submission 2021a: Preprocessing STARLIT + cmix
Released by Artemiy Margaritov on May 8, 2021.

This repository includes materials for a Hutter Prize Submission (submission-2021a). It contains:
* the source code of a new preprocessing algorithm that is tailored for enwik9 compression -- the _SorTing ARticLes by sImilariTy_ (STARLIT)
* the source code of a cmix-based compressor for enwik9 amended to meet the Hutter Prize restrictions on running time and usage of RAM
* the source code of phda9-2017 enwik8 preprocessor amended to work with enwik9
* a set of scripts for building and constructing the compressor
* a pre-built executable file of STARLIT compressor for an AMD's Zen 2 processor

The compressor that can be constructed using the sources/tools above can only work with enwik9. As per Hutter Prize Competition requirements, the compressor outputs a binary -- a self-extracting archive (executable) that restores enwik9.

The compressor was tested on Ubuntu 18 and x86 CPU. 

# Submission description
STARLIT beats the current Hutter Prize result when combined with the cmix compressor and phda9 preprocessing. Further in this document STARLIT means a compressor/decompressor that features 1) STARLIT preprocessing algorithm, 2) phda9 preprocessing algorithm, and 3) cmix compressor.  

Below is the current STARLIT compression result (Linux, x86 processor):
| Syntax | Description |
| --- | ----------- |
| STARLIT compressor binary size (S1)| 405924 bytes |
| STARLIT Self-extracting archive size (S2)| 115095976 bytes |
Total size (S): 115501900 bytes
The previous record (L): 116673681 bytes
Previous record relaxation (by May 8 2021): 127 days * 5000 bytes = 635000 bytes
The previous record (L with relaxation): 117308681
STARLIT Improvement: 1.54%
Operating system: Ubuntu 18
Processor: Intel(R) Xeon(R) Silver 4114 CPU @ 2.20GHz ([Geekbenck score 640](https://browser.geekbench.com/processors/intel-xeon-silver-4114)
Running time: 76 hours
RAM usage: 9910MB

# STARLIT algorithm description
STARLIT algorithm is changing the order of articles in the initial enwik9. This algorithm is based on two insights. Firstly, enwik9 is a collection of articles whose titles are sorted by alphabet. As a result, if articles are reordered as part of a compressor, the initial order can be easily restored by a conventional sorting algorithm that won't increase the size of the decompressor much. Secondly, state-of-the-art compressors (phda9, cmix, etc) are based on accumulating context information in a memory buffer that is limited in size. The accumulated context information is used for predicting the next symbol/bit. As a result, it can be beneficial to place similar articles nearby so context information that they share is reused as much as possible before eviction from the buffer.

STARLIT requires finding a new order of articles that minimizes the size of an archive outputted by an existing compressor (we are using cmix). Moreover, the part of RPA that searches for a new order of articles is not limited in complexity (code size) as it is not required to include it into the compressor: only the new order of articles should be included. Based on this observation, I implemented the STARLIT new-order-searching phase in pyspark. During that phase, firstly, each article is mapped to a feature vector using a Doc2Vec model. Secondly, considering each article feature vector to be a point in a Euclidean space, the Traveling Salesman Problem is solved resulting in the shortest path visiting each point. In other words, the shortest path represents the order of all articles where similar articles are placed nearby. 

The submission includes the new order under `./src/readalike_prepr/data/new_article_order`. The nth row of this file shows the index of an article in the original enwik9 file that should be placed as the nth article in the STARLIT-preprocessed file. 

# Changes in cmix
* disabling PAQ8 model
* disabling all layer1 mixers
* reducing the memory budget of PPMD models to 850MB 
* reducing the number of neurons in the LSTM mixer to 1 layer with 180 neurons
* limiting decay of learning rate of the LSTM mixer (learning rate is kept constant after input symbols are processed)
* replacing doubles with floats in few places 
* compiling with profiled guided optimizations
* embedding a compressed English dictionary and a file with new article order as part of the compressor binary

# Instructions for building STARLIT compressor
Creating STARLIT compressor binary includes the following steps:
* building cmix + STARLIT by clang with profile guided optimizations
* compressing the resulting compressor binary with UPX
* compressing the English dictionary by the compressor
* compressing the file with the new order of articles by the compressor
* merging the compressor binary with the compressed versions of the English dictionary and the new order file 

Constructing STARLIT compressor requires clang-12, upx-ucl and make packages. On Ubuntu 18, these packages can be installed by running the following scripts:
`./install_tools/install_upx.sh`
`./install_tools/install_clang-12.sh`

We provide a bash script for constructing STARLIT compressor on Ubuntu 18. It places the compressor binary named `cmix` in `./run` directory. The script can be found under
`./build_and_construct_comp.sh`

# Instructions for running STARLIT compressor
To run the compressor use
```bash
cd ./run
cmix -e <PATH_TO_ENWIK9> enwik9.comp
```
NOTE: the current version of STARLIT compressor can only work when the compressor executable file is named `cmix` and when it is launched from the directory containing the executable file.

# Acknowelegments
The author thanks Byron Knoll for making the source code of cmix compressor publicly available, and Alexander Rhatushnyak for sharing the source code of the phda9 enwik8 preprocessing stage.

# Original cmix sources 
[This is a link to the original cmix repo](https://github.com/byronknoll/cmix)
