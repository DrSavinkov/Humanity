#ifndef MY_METHODS
#define MY_METHODS
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;
ofstream fcerr("logs_2.txt");
ofstream fcout("logs_1.txt");
#define cerr fcerr
#define cout fcout
const bool DEBUG = false;
class Tick {
  static uint64_t _tick;

public:
  static uint64_t get() { return _tick; }
  static void tick() { _tick++; }
};
class HumanStorage {
  static map<void *, size_t> links;

public:
  static void append(void *link) {
    if (links.count(link) == 0)
      links[link] = links.size();
  }
  static size_t get(void *link) {
    auto f = links.find(link);
    size_t rz = -1;
    if (f == links.end())
      return rz;
    return f->second;
  }
};
map<void *, size_t> HumanStorage::links = {};
bool is_natural(const string &s) {
  try {
    int value = stoi(s);
    return value - value == 0;
  } catch (invalid_argument &ia) {
    if (DEBUG)
      cerr << "\"" << s << "\" is not a number.\n";
  } catch (out_of_range &oor) {
    if (DEBUG)
      cerr << "\"" << s << "\" is too large.\n";
  }
  return false;
}
uint64_t Tick::_tick = 0;
vector<string> split(string s, const string &key, const bool trim = true) {
  vector<string> r;
  size_t pos;
  while ((pos = s.find(key)) != s.npos) {
    r.push_back(s.substr(0, pos));
    s = s.substr(pos + key.length());
  }
  auto trim_proc = [](string &s) -> void {
    while (s.length()) {
      if (s[0] == ' ')
        s = s.substr(1);
      else if (s.back() == ' ')
        s = s.substr(0, s.length() - 1);
      else
        break;
    }
  };
  r.push_back(s);
  for (uint64_t i = 0; i < r.size(); i++)
    trim_proc(r[i]);
  for (uint64_t i = r.size() - 1; i < r.size(); i--) {
    if (r[i].length() == 0)
      r.erase(r.begin() + i);
  }
  return r;
}
template <typename C, typename T> vector<T> shuffle(const C &container) {
  vector<T> r(container.begin(), container.end());
  random_device rd;
  mt19937 generator(rd());
  shuffle(r.begin(), r.end(), generator);
  return r;
}
template <typename T> vector<T> shuffle(const set<T> &container) {
  return shuffle<set<T>, T>(container);
}
set<string> intersect(const set<string> &a, const set<string> &b) {
  set<string> r;
  for (const auto &i : a)
    if (b.count(i))
      r.insert(i);
  return r;
}
#endif