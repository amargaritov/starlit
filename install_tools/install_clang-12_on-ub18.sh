#/bin/bash 

pushd /tmp
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
popd

sudo apt-get install software-properties-common
sudo apt-add-repository "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-12 main"
sudo apt-get update 


#To install just clang, lld and lldb (12 release):
sudo apt-get install clang-12 lldb-12 lld-12 -y 

# LLVM for PGO
sudo apt-get install libllvm-12-ocaml-dev libllvm12 llvm-12 llvm-12-dev llvm-12-doc llvm-12-examples llvm-12-runtime -y
