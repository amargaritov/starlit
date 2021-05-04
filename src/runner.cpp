#include <fstream>
#include <ctime>
#include <stdio.h>
#include <cstdlib>
#include <vector>
#include <string.h>
#include <sys/stat.h>

#include "preprocess/preprocessor.h"
#include "coder/encoder.h"
#include "coder/decoder.h"
#include "predictor.h"

#include "readalike_prepr/article_reorder.h"
#include "readalike_prepr/self_extract.h"
#include "readalike_prepr/phda9_preprocess.h"
#include "readalike_prepr/misc.h"

//#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


namespace {
  const int kMinVocabFileSize = 10000;
}

int Help() {
  printf("cmix version 18\n");
  printf("Compress:\n");
  printf("    to compress enwik9: cmix -e enwik9 [output]\n");
  printf("    to create a header for hutter prize: cmix -h comp_dict_size comp_new_order_size decomp_input_size\n");
  printf("    with dictionary:    cmix -c [dictionary] [input] [output]\n");
  printf("    without dictionary: cmix -c [input] [output]\n");
  printf("    no preprocessing:   cmix -n [input] [output]\n");
  printf("    only preprocessing: cmix -s [dictionary] [input] [output]\n");
  printf("                        cmix -s [input] [output]\n");
  printf("Decompress:\n");
  printf("    with dictionary:    cmix -d [dictionary] [input] [output]\n");
  printf("    without dictionary: cmix -d [input] [output]\n");
  return -1;
}

void debugAllPrepare(std::string, std::string);

void WriteHeader(unsigned long long length, const std::vector<bool>& vocab,
    bool dictionary_used, std::ofstream* os) {
  for (int i = 4; i >= 0; --i) {
    char c = length >> (8*i);
    if (i == 4) {
      c &= 0x7F;
      if (dictionary_used) c |= 0x80;
    }
    os->put(c);
  }
  if (length < kMinVocabFileSize) return;
  for (int i = 0; i < 32; ++i) {
    unsigned char c = 0;
    for (int j = 0; j < 8; ++j) {
      if (vocab[i * 8 + j]) c += 1<<j;
    }
    os->put(c);
  }
}

void WriteStorageHeader(FILE* out, bool dictionary_used) {
  for (int i = 4; i >= 0; --i) {
    char c = 0;
    if (i == 4 && dictionary_used) c = 0x80;
    putc(c, out);
  }
}

void ReadHeader(std::ifstream* is, unsigned long long* length,
    bool* dictionary_used, std::vector<bool>* vocab) {
  *length = 0;
  for (int i = 0; i <= 4; ++i) {
    *length <<= 8;
    unsigned char c = is->get();
    if (i == 0) {
      if (c&0x80) *dictionary_used = true;
      else *dictionary_used = false;
      c &= 0x7F;
    }
    *length += c;
  }
  if (*length == 0) return;
  if (*length < kMinVocabFileSize) {
    std::fill(vocab->begin(), vocab->end(), true);
    return;
  }
  for (int i = 0; i < 32; ++i) {
    unsigned char c = is->get();
    for (int j = 0; j < 8; ++j) {
      if (c & (1<<j)) (*vocab)[i * 8 + j] = true;
    }
  }
}

void ExtractVocab(unsigned long long input_bytes, std::ifstream* is,
    std::vector<bool>* vocab) {
  for (unsigned long long pos = 0; pos < input_bytes; ++pos) {
    unsigned char c = is->get();
    (*vocab)[c] = true;
  }
}

void ClearOutput() {
  fprintf(stderr, "\r                     \r");
  fflush(stderr);
}

void Compress(unsigned long long input_bytes, std::ifstream* is,
    std::ofstream* os, unsigned long long* output_bytes, Predictor* p) {
  Encoder e(os, p);

  FILE* progress = fopen("./progress.log", "w");
  unsigned long long percent = 1 + (input_bytes / 10000);
  ClearOutput();
  for (unsigned long long pos = 0; pos < input_bytes; ++pos) {
    char c = is->get();
    for (int j = 7; j >= 0; --j) {
      e.Encode((c>>j)&1);
    }
    if (pos % percent == 0) {
      double frac = 100.0 * pos / input_bytes;
      fprintf(stderr, "\rprogress: %.2f%%", frac);
      fflush(stderr);

      fprintf(progress, "%.2f %llu\n", frac, os->tellp());
      fflush(progress);
    }
  }
  e.Flush();
  *output_bytes = os->tellp();
}

void Decompress(unsigned long long output_length, std::ifstream* is,
                std::ofstream* os, Predictor* p) {
  Decoder d(is, p);
  unsigned long long percent = 1 + (output_length / 10000);
  ClearOutput();
  for(unsigned long long pos = 0; pos < output_length; ++pos) {
    int byte = 1;
    while (byte < 256) {
      byte += byte + d.Decode();
    }
    os->put(byte);
    if (pos % percent == 0) {
      double frac = 100.0 * pos / output_length;
      fprintf(stderr, "\rprogress: %.2f%%", frac);
      fflush(stderr);
    }
  }
}

bool Store(const std::string& input_path, const std::string& temp_path,
    const std::string& output_path, FILE* dictionary,
    unsigned long long* input_bytes, unsigned long long* output_bytes) {
  FILE* data_in = fopen(input_path.c_str(), "rb");
  if (!data_in) return false;
  FILE* data_out = fopen(output_path.c_str(), "wb");
  if (!data_out) return false;
  fseek(data_in, 0L, SEEK_END);
  *input_bytes = ftell(data_in);
  fseek(data_in, 0L, SEEK_SET);
  WriteStorageHeader(data_out, dictionary != NULL);
  fprintf(stderr, "\rpreprocessing...");
  fflush(stderr);
  preprocessor::Encode(data_in, data_out, *input_bytes, temp_path, dictionary);
  fseek(data_out, 0L, SEEK_END);
  *output_bytes = ftell(data_out);
  fclose(data_in);
  fclose(data_out);
  return true;
}

bool RunCompression(bool enable_preprocess, const std::string& input_path,
    const std::string& temp_path, const std::string& output_path,
    FILE* dictionary, unsigned long long* input_bytes,
    unsigned long long* output_bytes) {
  FILE* data_in = fopen(input_path.c_str(), "rb");
  if (!data_in) return false;
  FILE* temp_out = fopen(temp_path.c_str(), "wb");
  if (!temp_out) return false;

  fseek(data_in, 0L, SEEK_END);
  *input_bytes = ftell(data_in);
  fseek(data_in, 0L, SEEK_SET);

  if (enable_preprocess) {
    fprintf(stderr, "\rpreprocessing...");
    fflush(stderr);
    preprocessor::Encode(data_in, temp_out, *input_bytes, temp_path,
        dictionary);
  } else {
    preprocessor::NoPreprocess(data_in, temp_out, *input_bytes);
  }
  fclose(data_in);
  fclose(temp_out);

  std::ifstream temp_in(temp_path, std::ios::in | std::ios::binary);
  if (!temp_in.is_open()) return false;

  std::ofstream data_out(output_path, std::ios::out | std::ios::binary);
  if (!data_out.is_open()) return false;

  temp_in.seekg(0, std::ios::end);
  unsigned long long temp_bytes = temp_in.tellg();
  temp_in.seekg(0, std::ios::beg);

  std::vector<bool> vocab(256, false);
  if (temp_bytes < kMinVocabFileSize) {
    std::fill(vocab.begin(), vocab.end(), true);
  } else {
    ExtractVocab(temp_bytes, &temp_in, &vocab);
    temp_in.seekg(0, std::ios::beg);
  }

  WriteHeader(temp_bytes, vocab, dictionary != NULL, &data_out);
  Predictor p(vocab);
  if (enable_preprocess) preprocessor::Pretrain(&p, dictionary);
  Compress(temp_bytes, &temp_in, &data_out, output_bytes, &p);
  temp_in.close();
  data_out.close();
  remove(temp_path.c_str());
  return true;
}

bool RunDecompression(const std::string& input_path,
    const std::string& temp_path, const std::string& output_path,
    FILE* dictionary, unsigned long long* input_bytes,
    unsigned long long* output_bytes) {
  std::ifstream data_in(input_path, std::ios::in | std::ios::binary);
  if (!data_in.is_open()) return false;

  data_in.seekg(0, std::ios::end);
  *input_bytes = data_in.tellg();
  data_in.seekg(0, std::ios::beg);
  std::vector<bool> vocab(256, false);
  bool dictionary_used;
  ReadHeader(&data_in, output_bytes, &dictionary_used, &vocab);
  if (!dictionary_used && dictionary != NULL) return false;
  if (dictionary_used && dictionary == NULL) return false;

  if (*output_bytes == 0) {  // undo store
    data_in.close();
    FILE* in = fopen(input_path.c_str(), "rb");
    if (!in) return false;
    FILE* data_out = fopen(output_path.c_str(), "wb");
    if (!data_out) return false;
    fseek(in, 5L, SEEK_SET);
    fprintf(stderr, "\rdecoding...");
    fflush(stderr);
    preprocessor::Decode(in, data_out, dictionary);
    fseek(data_out, 0L, SEEK_END);
    *output_bytes = ftell(data_out);
    fclose(in);
    fclose(data_out);
    return true;
  }
  Predictor p(vocab);
  if (dictionary_used) preprocessor::Pretrain(&p, dictionary);

  std::ofstream temp_out(temp_path, std::ios::out | std::ios::binary);
  if (!temp_out.is_open()) return false;

  Decompress(*output_bytes, &data_in, &temp_out, &p);
  data_in.close();
  temp_out.close();

  FILE* temp_in = fopen(temp_path.c_str(), "rb");
  if (!temp_in) return false;
  FILE* data_out = fopen(output_path.c_str(), "wb");
  if (!data_out) return false;

  preprocessor::Decode(temp_in, data_out, dictionary);
  fseek(data_out, 0L, SEEK_END);
  *output_bytes = ftell(data_out);
  fclose(temp_in);
  fclose(data_out);
  remove(temp_path.c_str());
  return true;
}

int main(int argc, char** argv) {
  if ((argc != 1) && (argv[1][1] != 'h') && (argc < 4 || argc > 5 || strlen(argv[1]) != 2 || argv[1][0] != '-' ||
      (argv[1][1] != 'c' && argv[1][1] != 'd' && argv[1][1] != 's' &&
      argv[1][1] != 'n' && argv[1][1] != 'e' ))) {
    return Help();
  }

  clock_t start = clock();

  bool enable_preprocess = true;
  std::string input_path ;
  std::string output_path;
  FILE* dictionary = NULL;

  
  if ((argc > 1) && (argv[1][1] != 'h'))  {
    if (argv[1][1] == 'n') enable_preprocess = false;
    input_path = argv[2];
    output_path = argv[3];
    if (argc == 5) {
      if (argv[1][1] == 'n') return Help();
      dictionary = fopen(argv[2], "rb");
      if (!dictionary) return Help();
      input_path = argv[3];
      output_path = argv[4];
    }
  }

  std::string temp_path = output_path + ".cmix.temp";

  unsigned long long input_bytes = 0, output_bytes = 0;

  if (argc == 1) {
    //Decompress enwik9
    // unpack a) header b) cmix dictionary, c) new order of articles, d) actual cmix binary
//    std::cout << "Uncompressing the dictionary and the input for cmix" << std::endl;
    selfextract_decomp();

    // run compression
//    std::cout << "Running cmix decompression..." << std::endl;
    input_path = ".ready4cmix_decomp";
    output_path = ".input_decomp" ;
    dictionary = fopen(".dict_decomp", "rb");
    if (!RunDecompression(input_path, temp_path, output_path, dictionary,
        &input_bytes, &output_bytes)) {
      return Help();
    }
//    std::cout << "Cmix decompression finished" << std::endl;

//    std::cout << "Prepared the decompressed file for restoring the order of articles" << std::endl;
    split4Decomp();

    // apply phda9 preprocessor
//    std::cout << "Revert phda9 preprocessing" << std::endl;
    phda9_resto();

    // change the order of articles in the input 
//    std::cout << "Restore article order" << std::endl;
    sort();

    // merge all input parts after preprocessing
//    std::cout << "Construct enwik9" << std::endl;
    cat(".intro_decomp", ".main_decomp_restored_sorted", "un1_d");
    cat("un1_d", ".coda_decomp", "enwik9_uncompressed");

    goto print_end_message;
  }

  if (argv[1][1] == 's') {
    if (!Store(input_path, temp_path, output_path, dictionary, &input_bytes,
        &output_bytes)) {
      return Help();
    }
  } else if (argv[1][1] == 'c' || argv[1][1] == 'n') {
    if (!RunCompression(enable_preprocess, input_path, temp_path, output_path,
        dictionary, &input_bytes, &output_bytes)) {
      return Help();
    }
  } else if (argv[1][1] == 'e') {
    // Compress enwik9
    input_path = argv[2];
    output_path = argv[3]; //name of a compressor output

    // DEBUG
//    debugAllPrepare(input_path, output_path);// debug
//    return 0;
    //DEBUG END

//    std::cout << "Uncompressing the dictionary and the file with the new order of articles" << std::endl;
    // unpack a) cmix dictionary, b) new order of articles, c) actual cmix binary
    selfextract_comp();

//    std::cout << "Preparing enwik9 for reordering" << std::endl;
    split4Comp(input_path.c_str());

    // change the order of articles in the input 
//    std::cout << "Reordering enwik9 articles" << std::endl;
    reorder();

    // apply phda9 preprocessor
//    std::cout << "Applying phda9 preprocessor to the reordered enwik9" << std::endl;
    phda9_prepr();

    // merge all input parts after preprocessing
//    std::cout << "Merging all parts into one input file for cmix" << std::endl;
    cat(".main_phda9prepr", ".intro", "un1");
    cat("un1", ".coda", ".ready4cmix");

    // run compression
//    std::cout << "Cmix compression..." << std::endl;
    input_path = ".ready4cmix";
    dictionary = fopen(".dict", "rb");
    if (!RunCompression(enable_preprocess, input_path, temp_path, output_path,
        dictionary, &input_bytes, &output_bytes)) {
      return Help();
    }
//    std::cout << "Cmix compression finished" << std::endl;

    // construct a selfextracting decompressor binary
    // archive9 = decomp_binary(upxed) + comp_dict + cmix_output + header.dat
//    std::cout << "Constructing the selfextracting archive9" << std::endl;
    cat(".decomp_bin", ".dict.comp", "dec1");

    // get the size of the output file
    FILE *f = fopen(output_path.c_str(), "rb");
    if (f == NULL) {
      printf("can't open output file for measuring its size"), exit(1);
    }
    fseek(f, 0, SEEK_END);
    size_t output_size = ftell(f);
    fclose(f);

    HeaderInfo header;
    read("test.dat", header);
    header.decomp_input_size = output_size;
    write("header4archive.dat", header);
    
    cat("dec1", output_path.c_str(), "dec2");
    cat("dec2", "header4archive.dat", "archive9");

    //make the decompressor binary executable
    char mode[] = "0777";
    char buf[100] = "archive9";
    int i = strtol(mode, 0, 8);
    chmod(buf, i);

  } else if (argv[1][1] == 'h') {
//    std::cout << argc << " " << sizeof(HeaderInfo) << std::endl << std::flush;
    if (argc < 5) return Help();
    HeaderInfo header;
    header.dict_size = atoi(argv[2]);
    header.new_article_order_size = atoi(argv[3]);
    header.decomp_input_size = atoi(argv[4]);
    write("header.dat", header);
//    std::cout << "Successfully created header.dat with dict_size=" << header.dict_size 
//              << " new_article_order_size=" << header.new_article_order_size 
//              << " decomp_input_size=" << header.decomp_input_size << std::endl;
    goto exit;
  } else {
    if (!RunDecompression(input_path, temp_path, output_path, dictionary,
        &input_bytes, &output_bytes)) {
      return Help();
    }
  }

print_end_message:
  printf("\r%lld bytes -> %lld bytes in %1.2f s.\n",
      input_bytes, output_bytes,
      ((double)clock() - start) / CLOCKS_PER_SEC);

  if (argc > 1) {
    if (argv[1][1] == 'c') {
      double cross_entropy = output_bytes;
      cross_entropy /= input_bytes;
      cross_entropy *= 8;
      printf("cross entropy: %.3f\n", cross_entropy);
    }
  }
exit:
  return 0;
}

void debugAllPrepare(std::string input_path, std::string output_path) {
    // Compress enwik9
    // unpack a) cmix dictionary, b) new order of articles, c) actual cmix binary
    selfextract_comp();
//    std::cout << __LINE__ << std::endl << std::flush;

    split4Comp(input_path.c_str());
//    std::cout << __LINE__ << std::endl << std::flush;

    // change the order of articles in the input 
    reorder();
 //   std::cout << __LINE__ << std::endl << std::flush;

    // apply phda9 preprocessor
    phda9_prepr();
//    std::cout << __LINE__ << std::endl << std::flush;

    // merge all input parts after preprocessing
    cat(".main_phda9prepr", ".intro", "un1");
    cat("un1", ".coda", ".input_decomp");
//    std::cout << __LINE__ << std::endl << std::flush;

//-->

    split4Decomp();
//    std::cout << __LINE__ << std::endl << std::flush;

    // apply phda9 preprocessor
    phda9_resto();
//    std::cout << __LINE__ << std::endl << std::flush;

    // change the order of articles in the input 
    sort();
//    std::cout << __LINE__ << std::endl << std::flush;

    // merge all input parts after preprocessing
    cat(".intro_decomp", ".main_decomp_restored_sorted", "un1_d");
    cat("un1_d", ".coda_decomp", "enwik9_restored");

//    std::cout << "Finished" << std::endl << std::flush;
}
