#ifndef ARTICLE_REORDER_H 
#define ARTICLE_REORDER_H 

#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

//#include <iostream>


struct Accumulator {
  int id;
  int start;
  int end;
};

enum ParserState {
  expect_page = 0,
  expect_id,
  expect_pageend
};

int line_count = 0;

int action_get_line_count(std::string s) {
  return line_count;
}

int action_get_id(std::string s) {
  std::string::iterator end_pos = std::remove(s.begin(), s.end(), ' ');
  s.erase(end_pos, s.end());

  std::string tok = "<id>";
  std::string::size_type i = s.find(tok);
  if (i != std::string::npos)
     s.erase(i, tok.length());

  tok = "</id>";
  i = s.find(tok);
  if (i != std::string::npos)
     s.erase(i, tok.length());

//  std::cout << s << std::endl << std::flush;
  return std::stoi(s);
}

void save_pagestart(int res, Accumulator* acc) {
  acc->start = res;
}

void save_id(int res, Accumulator* acc) {
  acc->id = res;
}

void save_pageend(int res, Accumulator* acc) {
  acc->end = res;
}

bool sortById(Accumulator &A, Accumulator &B) //function to sort fruits by names
{
  return (A.id < B.id);
}

int reorder() {
  line_count = 0;

  std::ifstream file(".main"); //file just has some sentences
//  if (!file) {
//    std::cout << "unable to open file";
//    return -1;
//  }

  std::ifstream order_file(".new_article_order"); //file just has some sentences
//  if (!order_file) {
//    std::cout << "unable to open file";
//    return -1;
//  }

  std::vector<std::string> lines;
  std::vector<int> positions;

  std::vector<std::string> patterns  = { "<page>", "<id>", "</page>" };
  std::vector<ParserState>  transitions    = { expect_id, expect_pageend, expect_page };
  int (*actions[3])(std::string)     = {action_get_line_count, action_get_id, action_get_line_count};
  void (*save[3])(int, Accumulator*) = {save_pagestart, save_id, save_pageend};

  ParserState state = expect_page;

  std::vector<Accumulator> vec;

  std::string s;
  std::string pattern;
  int res = 0;
  Accumulator acc;
  while (std::getline(file, s))
  {
    pattern = patterns[state];
    if (s.find(pattern) != std::string::npos) {
      res = actions[state](s);
      save[state](res, &acc);
      state = transitions[state];
      if (state == expect_page)
        vec.push_back(acc);
    } 
    line_count++;
    lines.push_back(s);
  }

//  std::cout << line_count  << std::endl;

//  for(std::vector<Accumulator>::const_iterator it = vec.begin();
//    it != vec.end(); ++it) {
//    std::cout << it->id << " " << it->start << " " << it->end << std::endl;
//  }

  while (std::getline(order_file, s)) {
//    std::cout << s << std::endl << std::flush;
    positions.push_back(std::stoi(s));
  }

  std::ofstream out(".main_reordered");
  for(int i = 0; i < positions.size(); i++) {
    int pos = positions[i];
    for(int j = vec[pos].start; j <= vec[pos].end; j++) {
      out << lines[j] << std::endl;
    }
  } 
  out.close();

  return 0;
}


int sort() {
  line_count = 0;

  std::ifstream file(".main_decomp_restored"); 
  if (!file) {
//    std::cout << "unable to open file";
    return -1;
  }

  std::vector<std::string> lines;

  std::vector<std::string> patterns  = { "<page>", "<id>", "</page>" };
  std::vector<ParserState>  transitions    = { expect_id, expect_pageend, expect_page };
  int (*actions[3])(std::string)     = {action_get_line_count, action_get_id, action_get_line_count};
  void (*save[3])(int, Accumulator*) = {save_pagestart, save_id, save_pageend};

  ParserState state = expect_page;

  std::vector<Accumulator> vec;

  std::string s;
  std::string pattern;
  int res = 0;
  Accumulator acc;
  while (std::getline(file, s))
  {
    pattern = patterns[state];
    if (s.find(pattern) != std::string::npos) {
      res = actions[state](s);
      save[state](res, &acc);
      state = transitions[state];
      if (state == expect_page)
        vec.push_back(acc);
    } 
    line_count++;
    lines.push_back(s);
  }

//  std::cout << line_count  << std::endl;

  std::sort(vec.begin(), vec.end(), sortById);

//  for(std::vector<Accumulator>::const_iterator it = vec.begin();
//    it != vec.end(); ++it) {
//    std::cout << it->id << " " << it->start << " " << it->end << std::endl;
//  }

  std::ofstream out(".main_decomp_restored_sorted");
  if (!out) {
//    std::cout << "unable to open file";
    return -1;
  }
  for(int i =0; i < vec.size(); i++) {
    for(int j = vec[i].start; j <= vec[i].end; j++) {
      out << lines[j] << std::endl;
    }
  } 
  out.close();

  return 0;
}

#endif // ARTICLE_REORDER_H 
