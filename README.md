# Hutter Prize Submission 2021a: STARLIT + cmix
Released by Artemiy Margaritov on May 10, 2021.

This repository includes materials for a Hutter Prize Submission (submission-2021a). It contains:
* the source code of a new preprocessing algorithm that is tailored for enwik9 compression -- the _SorTing ARticLes by sImilariTy_ (STARLIT)
* the source code of a cmix-based compressor for enwik9 amended to meet the Hutter Prize restrictions on running time and usage of RAM
* the source code of the HP-2017 enwik8-specific preprocessor amended to work with enwik9
* a set of scripts for building and constructing the compressor combining the components mentioned above on Ubuntu 18/20
* a pre-built executable file of STARLIT compressor for an AMD's Zen 2 processor

The compressor that can be constructed using the sources/tools above can only work with enwik9. As per Hutter Prize Competition requirements, the compressor outputs an executable file -- a self-extracting archive (executable) that restores enwik9. 

The compressor/decompressor was tested on Ubuntu 18 and x86 CPU. 

# Submission description
STARLIT beats the current Hutter Prize result when combined with the cmix compressor and phda9 preprocessing. For brevity, further in this document, we use STARLIT to mean a compressor/decompressor that features 1) STARLIT preprocessing algorithm, 2) 
preprocessing algorithm, and 3) cmix compression routine.  

Below is the current STARLIT compressor result (Ubuntu 18 (Linux), x86 processor):
| Metric | Value |
| --- | ----------- |
| STARLIT compressor's executable file size (S1)| 403308 bytes |
| STARLIT self-extracting archive size (S2)| 115093300 bytes |
| Total size (S) | 115496608 bytes |
| Previous record | 116673681 bytes |
| Relaxation (as of May 10 2021)| 129 days * 5000 bytes = 645000 bytes |
| Previous record with relaxation (L) | 117318681 bytes |
| STARLIT improvement (1 - S/L) | 1.553% |

| Experiment platform |  |
| --- | ----------- |
| Operating system | Ubuntu 18 |
| Processor | Intel(R) Xeon(R) Silver 4114 CPU @ 2.20GHz ([Geekbenck score 640](https://browser.geekbench.com/processors/intel-xeon-silver-4114))
| Decompression running time | 76 hours |
| Decompression RAM max usage | 9910MB |
| Decompression disk usage | 20GB |

Compressor and decompressor perform similar stages. As a result, the compression time is approximately the same as the decomression time. Similarly, RAM and disk usages during compression is also approximately the same as one during decompression. 

# STARLIT algorithm description
The STARLIT algorithm is changing the order of articles in the initial enwik9. This algorithm is based on two insights. 
Firstly, enwik9 is a collection of articles whose titles are sorted by alphabet. As a result, if articles are reordered as part of a compressor, the initial order can be easily restored by a conventional sorting algorithm (e.g. Bubble Sort) that is simple and negligibly increases the size of the decompressor. 
Secondly, state-of-the-art compressors (cmix, phda9, etc) are based on accumulating context information in a memory buffer that is limited in size. The accumulated context information is used for predicting the next symbol/bit. We hypothesize that it is better to use the accumulated context information as soon as possible. Due to the limited size of the buffer, the longer the accumulated information stays in the buffer the higher the chances that it will be corrupted or removed. As a result, it can be beneficial to place _similar_ articles nearby so context information that they share is reused as much as possible before eviction from the buffer.
To sum up, the idea behind STARLIT is to reorder enwik articles in a way that similar articles are placed together. Such ordering makes prediction methods employed by cmix (or phda9, etc) more accurate and results in a higher degree of compression.


STARLIT requires finding a new order of articles that minimizes the size of an archive outputted by an existing compressor (we are using cmix). Moreover, the part of STARLIT that searches for a new order of articles is not limited in complexity (code size) as it is not required to include it into the compressor: only the new order of articles should be included. Based on this observation, we implemented the STARLIT new-order-searching phase in PySpark. During the searching phase, firstly, each article is mapped to a feature vector using a Doc2Vec model. Secondly, considering each feature vector to be a point in a Euclidean space, the Traveling Salesman Problem is solved resulting in the shortest path visiting each point. As a result, the found shortest path represents the order of all articles where similar articles are placed nearby. 

The submission includes the new article order file under `./src/readalike_prepr/data/new_article_order`. The nth row of this file shows the index of an article in the original enwik9 file that the STARLIT algorithm places as the nth article in its output. 

STARLIT reordering/sorting functions are implemented in `./src/readalike_prepr/article_reorder.h`. 

# Changes to cmix_v18
* disabling PAQ8 model
* disabling all layer1 mixers
* reducing the memory budget of PPMD models to 850MB 
* reducing the number of neurons in the LSTM mixer to 1 layer with 180 neurons
* limiting decay of learning rate of the LSTM mixer (learning rate is kept constant after input symbols are processed)
* replacing doubles with floats in several places 
* compiling with profiled guided optimizations
* embedding a compressed English dictionary and a file with new article order as part of the compressor's executable file

# Changes to HP-2017 enwik8-specific transforms 
* merging all (de)preprocessing functions to one header file under `src/readalike_prepr/phda9_preprocess.h`
* adding sed functions as part of preprocessing to "protect" some patterns from a wrong transformation 
* increasing the size of buffers used by (de)preprocessing functions 

# Instructions for compiling STARLIT compressor from sources
Creating the STARLIT compressor executable file includes the following steps:
1. building cmix + STARLIT by clang-12 with profile guided optimizations
2. Construction:
  * compressing the resulting compressor executable file with the Ultimate Packer for eXecutables (UPX)
  * compressing the cmix English dictionary by the resulting compressor
  * compressing the file with the new order of articles by the resulting compressor
  * merging the compressor executable file with the compressed versions of 1) the cmix English dictionary and 2) the new order file 

We refer to the stage 1 as _building_, and to the stage 2 as _constructing_. 

# Installing packages required for compiling STARLIT compressor from sources on Ubuntu 18/20
Building STARLIT compressor from sources requires clang-12, upx-ucl, and make packages. On Ubuntu 18/20, these packages can be installed by running the following scripts:
```bash
./install_tools/install_upx.sh
./install_tools/install_clang-12.sh
```

# Compiling STARLIT compressor from sources
We provide a bash script for compiling STARLIT compressor from sources on Ubuntu 18. This script places the STARLIT executalbe file named as `cmix` in `./run` directory. The script can be run as
```bash
./build_and_construct_comp.sh
```

# Constructing STARLIT compressor using a pre-built executable file
If clang-12, upx-ucl, or make packages are not available, it is possible to construct a STARLIT executable file using a pre-built version (tested only on Ubuntu 18). We provide a script for constructing the STARLIT compressor from a prebuild executable file. This script places the STARLIT compressor file under `./run`. The script can be run as
```bash
./construct_from_prebuilt.sh ./prebuild_binary/cmix_amdzen2_ub18
```

_NOTE: currently, we provide a prebuilt STARLIT executable optimized for an AMD's Zen 2 processor and Ubuntu 20. Please contact the author if you need a prebuild executable file optimized for another CPU and/or OS._

# Running STARLIT compressor
To run the STARLIT compressor use
```bash
cd ./run
cmix -e <PATH_TO_ENWIK9> enwik9.comp
```
_NOTE: the current version of the STARLIT compressor can only work when the STARLIT executable file is named `cmix` and when it is launched from the directory containing it._

# Expected STARLIT compressor/decompressor output
When launched as described above, the STARLIT compressor would 
1. decompress the cmix English dictionary
2. decompress the new article order file 
3. apply the STARLIT algoritm (reorder articles) 
4. apply the HP-2017 preprcessor (enwik-specific transforms)
5. run normal cmix compressing routine

For stages 1, 2, and 5, the STARLIT compressor would print progress (similarly as the original cmix does it). The stages 3 and 4 are expected to run less than 15 minutes in total. After finishing stages 3 and 4, the stage 5 would print progress. After finishing stages 1 and 2, the output is expected to be 
```bash
***
77992 bytes -> 415377 bytes in 112.10 s.
210080 bytes -> 1592865 bytes in 383.34 s.
***
```

# Running STARLIT decompressor
The compressor is expected to output an executable file named `archive9` in the same directory (`./run`). The file `archive9` when executed is expected to reproduce the original enwi9 as a file named `enwik9_restored`. The executable file `archive9` should be launched wihtout argments from the directory containing it. 
```bash
cd ./run
./archive9
```
_NOTE: both STARLIT compressor (executalbe `cmix`) and decompressor (exectable `archive9`) require about 20GB of disk space._ 

# Acknowelegments
The author thanks Byron Knoll for making the source code of the cmix compressor publicly available. The author also thanks Alexander Rhatushnyak for open-sourcing the set of enwik8 specific transforms that are part of the HP-2017 submission. The author is also grateful to Marcus Hutter and Matt Mahoney for assistance with the submission process. 

# Open-source projects used in this submission
* [This is a link to the original cmix repo](https://github.com/byronknoll/cmix)
* [This is a link to the code of HP-2017 enwik8 specific transforms](https://encode.su/attachment.php?s=849482f9bef20cb68ebc7aaba0f5ff49&attachmentid=6353&d=1546482845)
