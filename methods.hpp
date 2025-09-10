#ifndef MY_METHODS
#define MY_METHODS
#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
const bool DEBUG = false;
class Tick {
  static uint64_t _tick;

public:
  static uint64_t get() { return _tick; }
  static void tick() { _tick++; }
};
class HumanStorage {
  static std::map<void *, size_t> links;

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
std::map<void *, size_t> HumanStorage::links = {};
bool is_natural(const std::string &s) {
  try {
    int value = stoi(s);
    return value - value == 0;
  } catch (std::invalid_argument &ia) {
    if (DEBUG)
      std::cerr << "\"" << s << "\" is not a number.\n";
  } catch (std::out_of_range &oor) {
    if (DEBUG)
      std::cerr << "\"" << s << "\" is too large.\n";
  }
  return false;
}
uint64_t Tick::_tick = 0;
std::vector<std::string> split(std::string s, const std::string &key,
                               const bool trim = true) {
  std::vector<std::string> r;
  size_t pos;
  while ((pos = s.find(key)) != s.npos) {
    r.push_back(s.substr(0, pos));
    s = s.substr(pos + key.length());
  }
  auto trim_proc = [](std::string &s) -> void {
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
template <typename C, typename T> std::vector<T> shuffle(const C &container) {
  std::vector<T> r(container.begin(), container.end());
  std::random_device rd;
  std::mt19937 generator(rd());
  shuffle(r.begin(), r.end(), generator);
  return r;
}
template <typename T> std::vector<T> shuffle(const std::set<T> &container) {
  return shuffle<std::set<T>, T>(container);
}
std::set<std::string> intersect(const std::set<std::string> &a,
                                const std::set<std::string> &b) {
  std::set<std::string> r;
  for (const auto &i : a)
    if (b.count(i))
      r.insert(i);
  return r;
}
#endif