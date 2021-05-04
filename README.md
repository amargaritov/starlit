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
STARLIT compressor binary size (S1): 405924 bytes
STARLIT Self-extracting archive size (S2): 115095976 bytes
Total size (S): 115501900 bytes
Previous record (L): 116673681 bytes
Previous record relaxation (by May 8 2021): 127 days * 5000 bytes = 635000 bytes
Previous record (L with relaxation): 117308681
STARLIT Improvement: 1.54%
Operating system: Ubuntu 18
Processor: Intel(R) Xeon(R) Silver 4114 CPU @ 2.20GHz ([Geekbenck score 640](https://browser.geekbench.com/processors/intel-xeon-silver-4114)
Running time: 76 hours
RAM usage: 9910MB

# STARLIT algorithm description
STARLIT algorithm is changing the order of articles in the initial enwik9. This algorithm is based on two insights. Firstly, enwik9 is a collection of articles whose titles are sorted by alphabet. As a result, if articles are reordered as part of a compressor, the initial order can be easily restored by a conventional sorting algorithm that won't increase the size of the decompressor much. Secondly, state-of-the-art compressors (phda9, cmix, etc) are based on accumulating context information in a memory buffer that is limited in size. The accumulated context information is used for prediction next symbol/bit. As a result, it can be beneficial to place similar articles nearby so context information that they share is reused as much as possible before eviction from the buffer.

STARLIT requires finding a new order of articles that minimizes the size of an archive outputted by an existing compressor (we are using cmix). Moreover, the part of RPA that searches for a new order of articles is not limited in complexity (code size) as it is not required to include it into the compressor: only the new order of articles should be included. Based on this observation, I implemented the STARLIT new-order-searching phase in pyspark. During that phase, firstly, each article is mapped to a feature vector using a Doc2Vec model. Secondly, considering each article feature vector to be a point in a Euclidean space, the Traveling Salesman Problem is solved resulting in the shortest path visiting each point. In other words, the shortest path represents the order of all articles where similar articles are placed nearby. 

The submission includes the new order under `./src/readalike_prepr/data/new_article_order`. The nth row of this file shows the index of an article in the original enwik9 file that should be placed as the nth article in the STARLIT-preprocessed file. 

# Changes in cmix
* disabling PAQ8 model
* disabling all layer1 mixers
* reducing the memory budget of PPMD models to 850MB 
* reducing the number of neurons in the LSTM mixer to 1 layer with 180 neurons
* limiting decay of learning rate of the LSTM mixer (learning rate is kept constant after input symbols are processed)
* replacing doubles with floats in few places 
* compiling with profiled gueded optimisations
* embedding a compressed english dictionary and a file with new article order as part of the compressor binary

# Instructions for building STARLIT compressor
Creating STARLIT compressor binary includes the following steps:
* building cmix + STARLIT by clang with profile guided optimisations
* compressing the resulting compressor binary with UPX
* compressing the english dictionary by the compressor
* compressing the file with the enew order of articles by the compressor
* merging the compressor binary with the compressed versions of the english dictionary and the new order file 

Constructing STARLIT compressor requires clang-12, upx-ucl and make packages. On Ubuntu 18, these packages can be installed by running the following scripts:
`./install_tools/install_upx.sh`
`./install_tools/install_clang-12.sh`

We provide a bash script for constructing STARLIT compressor on Ubuntu 18. It places the copmressor binary named `cmix` in `./run` directory. The script can be found under
`./build_and_construct_comp.sh`


# Instructions for running STARLIT compressor
To run the compressor use
```bash
cd ./run
cmix -e <PATH_TO_ENWIK9> enwik9.comp
```
NOTE: the current version of STARLIT compressor can only work when the compressor executable file is named `cmix` and when it is launched from the directory containing the executable file.

# Acknowelegments
The author thank Byron Knoll for making the source code of cmix compressor publicly available, and Alexander Rhatushnyak for sharing the source code of the phda9 enwik8 preprocessing stage. 



# Below is the original README from [the original cmix repo](https://github.com/byronknoll/cmix)
cmix version 18
Released by Byron Knoll on August 1, 2019.
http://www.byronknoll.com/cmix.html
https://github.com/byronknoll/cmix

cmix is a lossless data compression program aimed at optimizing compression ratio at the cost of high CPU/memory usage. cmix is free software distributed under the GNU General Public License.

cmix works in Linux, Windows, and Mac OS X. At least 32GB of RAM is recommended to run cmix. Feel free to contact me at byron@byronknoll.com if you have any questions.

Use "make" to compile cmix. In Windows, cmix can be compiled with MinGW (http://nuwen.net/mingw.html) or Cygwin (https://www.cygwin.com).

cmix can only compress/decompress single files. To compress multiple files or directories, create an archive file using "tar" (or some similar tool).

For some files, preprocessing using "precomp" may improve compression: https://github.com/schnaader/precomp-cpp

Compile with "-Ofast -march=native" for fastest performance. These compiler options might lead to incompatibility between different computers due to floating-point precision differences. Compile with "-O3" to fix compatibility issues (at the cost of slower performance).

Changes from version 17 to version 18:
- LSTM improvements

Changes from version 16 to version 17:
- New dictionary from phda 2017 November release
- Rewrote WRT dictionary preprocessor
- Improvements ported from paq8px
- Some other minor improvements, bug fixes, and refactoring

Changes from version 15 to version 16:
- Improvements ported from paq8px
- Context mixer improvements
- Removed some redundant models
- LSTM improvements

Changes from version 14 to version 15:
- Improvements ported from paq8px and paq8pxd
- Enabled pretraining
- LSTM tuning
- Context mixer improvements
- Memory tuning

Changes from version 13 to version 14:
- LSTM performance optimizations and tuning
- Changes from Márcio Pais: improved preprocessing and many PAQ8 model improvements
- Disabled pretraining
- New dictionary (from phda)

Changes from version 12 to version 13:
- Improved LSTM and SSE

Changes from version 11 to version 12:
- Replaced byte mixer with LSTM

Changes from version 10 to version 11:
- Added ppmd model
- Memory tuning and bug fixes

Changes from version 9 to version 10:
- Learning rate decay tuning
- Added "interval" context (adapted from MCM https://github.com/mathieuchartier/mcm)
- Added "only preprocessing" compression option
- Refactoring and memory tuning

Changes from version 8 to version 9:
- Improved Windows compatibility
- Learning rate decay
- New nonstationary state table
- Bracket matching models
- Minor bug fixes and memory tuning

Changes from version 7 to version 8:
- Added a recurrent backpropagation neural network to mix byte-level models
- Added some PPM models
- Memory tuning
- Minor refactoring

Changes from version 6 to version 7:
- Fixed bug that caused cmix to crash in Windows
- Tweaked word model
- Added some image models (JPEG/BMP/TIFF)
- Improved preprocessor (JPEG/BMP/text/EXE)

Changes from version 5 to version 6:
- Memory tuning
- Removed PPM model

Changes from version 4 to version 5:
- Added dictionary pretraining
- Removed dictionary from cmix binary
- Integrated some code from paq8pxd_v12 into paq8l
- Minor refactoring

Changes from version 3 to version 4:
- Added an additional mixer context (longest match length)
- Implemented an additional PPM model
- Integrated some code from paq8pxd_v11 into paq8l
- Integrated mixer contexts from paqar into paq8l
- Minor refactoring and tuning

Changes from version 2 to version 3:
- Merged paq8pxd code into paq8l
- Optimized speed of cmix and paq8 mixers
- Minor refactoring and tuning

Changes from version 1 to version 2:
- Memory tuning
- Increase PPM order to 7
- Replaced dictionary with the one used by decmprs8
- Added paq8pxd_v7 models
- Removed pic models
- Added "facade" models to copy the paq8 model predictions to the cmix mixer
