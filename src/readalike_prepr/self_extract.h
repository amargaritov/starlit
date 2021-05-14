#ifndef SELF_EXTRACT_H 
#define SELF_EXTRACT_H 

//#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include <string>
#include <fstream>

//#include <iostream>

struct HeaderInfo {
  int dict_size;
  int new_article_order_size;
  int decomp_input_size;
};

void write(const std::string& file_name, HeaderInfo& data) {
  std::ofstream out(file_name.c_str());
  out.write(reinterpret_cast<char*>(&data), sizeof(HeaderInfo));
}

void read(const std::string& file_name, HeaderInfo& data) {
  std::ifstream in(file_name.c_str());
  in.read(reinterpret_cast<char*>(&data), sizeof(HeaderInfo));
}


// This function splits the ./cmix binary file into 3 parts:
// 1) actual compressor/decompressor binary
// 2) dictionary (get's it in compressed form and decompresses it)
// 3) new order of articles (get's it in compressed form and decompresses it) 
int selfextract_comp() {
  HeaderInfo header;

// open itslef to read auxilary data (dictionary and neworder)
  FILE *f = NULL, *fo = NULL;
  f = fopen("cmix", "rb");
//  if (f == NULL) {
//    printf("can't open cmix file (itself)"), exit(1);
//  }

  // get the size of the whole binary
  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
//  std::cout << "fsize=" << fsize << std::endl;

  unsigned char *p1 = (unsigned char *)malloc(fsize);
  // read the whole binary to memory
  fread(p1, fsize, 1, f);
  fclose(f);

  // read header info
  fo = fopen("test.dat", "wb");
//  if (fo == NULL) {
//    printf("can't open test.dat file"), exit(1);
//  }
  fwrite(p1 + fsize - sizeof(HeaderInfo), sizeof(HeaderInfo), 1, fo);
  fclose(fo);
  read("test.dat", header);
//  std::cout << "Successfully read header file (test.dat) with dict_size=" << header.dict_size 
//            << " new_article_order_size=" << header.new_article_order_size 
//            << " decomp_input_size=" << header.decomp_input_size 
//            << std::endl;

  size_t decmpressor_binary_size = fsize - header.dict_size - header.new_article_order_size - sizeof(HeaderInfo);

// produce actual decompressor binary 
  fo = fopen(".decomp_bin", "wb");
//  if (fo == NULL) {
//    printf("can't open .decomp_bin file"), exit(1);
//  }
//  printf("Created .decomp_bin with size=%d\n", decmpressor_binary_size); //debug
  fwrite(p1, decmpressor_binary_size, 1, fo);
  fclose(fo);

// produce dictionary and decompress it
  fo = fopen(".dict.comp", "wb");
//  if (fo == NULL) {
//    printf("can't open .dict.comp file"), exit(1);
//  }
  fwrite(p1 + decmpressor_binary_size, header.dict_size, 1, fo);
  fclose(fo);
//  std::cout << "Decompressing dictionary..." << std::endl;
  system("./cmix -d .dict.comp .dict");

// produce article order and decompress it
  fo = fopen(".new_article_order.comp", "wb");
//  if (fo == NULL) {
//    printf("can't open .new_article_order.comp file"), exit(1);
//  }
  fwrite(p1 + decmpressor_binary_size + header.dict_size, header.new_article_order_size, 1, fo);
  fclose(fo);
//  std::cout << "Decompressing the file with the new article order..." << std::endl;
  system("./cmix -d .new_article_order.comp .new_article_order");

  free(p1);
  malloc_trim(0);
  return 0;
}

// Same as previous function, but used in decompressor
// This function splits the ./cmix binary file into 3 parts:
// 1) actual compressor/decompressor binary
// 2) dictionary (get's it in compressed form and decompresses it)
// 3) new order of articles (get's it in compressed form and decompresses it) 
int selfextract_decomp() {
  HeaderInfo header;
  FILE *f = NULL, *fo = NULL;
  f = fopen("archive9", "rb");
//  if (f == NULL) {
//    printf("can't open archive9 file (itself)"), exit(1);
//  }

  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  unsigned char *p1 = (unsigned char *)malloc(fsize);
  fread(p1, fsize, 1, f);
  fclose(f);

  // read header info
  fo = fopen("test.dat", "wb");
//  if (fo == NULL) {
//    printf("can't open test.dat file"), exit(1);
//  }
  fwrite(p1 + fsize - sizeof(HeaderInfo), sizeof(HeaderInfo), 1, fo);
  fclose(fo);
  read("test.dat", header);
//  std::cout << "Successfully read header file (test.dat) with dict_size=" << header.dict_size 
//            << " new_article_order_size=" << header.new_article_order_size 
//            << " decomp_input_size=" << header.decomp_input_size 
//            << std::endl;


  size_t decmpressor_binary_size = fsize - header.dict_size - header.decomp_input_size - sizeof(HeaderInfo);

  fo = fopen(".dict.comp_decomp", "wb");
//  if (fo == NULL) {
//    printf("can't open .dict.comp_decomp file"), exit(1);
//  }
  fwrite(p1 + decmpressor_binary_size, header.dict_size, 1, fo);
  fclose(fo);
//  std::cout << "Decompressing dictionary..." << std::endl;
  system("./archive9 -d .dict.comp_decomp .dict_decomp");

  fo = fopen(".ready4cmix_decomp", "wb");
//  if (fo == NULL) {
//    printf("can't open .ready4cmix_decomp file"), exit(1);
//  }
  fwrite(p1 + decmpressor_binary_size + header.dict_size, header.decomp_input_size, 1, fo);
  fclose(fo);

  free(p1);
  malloc_trim(0);
  return 0;
}

#endif // PREPR_H
