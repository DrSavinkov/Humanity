#ifndef MY_RAND
#define MY_RAND
#include <cstdint>
#include <omp.h>
#include <random>
#include <vector>
namespace my {
namespace Random {
const std::vector<std::mt19937_64 *> rg =
    []() -> std::vector<std::mt19937_64 *> {
  std::vector<std::mt19937_64 *> r;
  std::random_device rd{};
  for (int i = 0; i < omp_get_max_threads(); i++)
    r.push_back(new std::mt19937_64{rd()});
  return r;
}();
std::uniform_int_distribution<uint64_t> uuid{
    0, std::numeric_limits<uint64_t>::max()};
uint64_t next() {
  uint64_t r;
  r = uuid(*rg[omp_get_thread_num()]);
  return r;
}
double nextf() {
  return ((double)my::Random::next()) /
         ((double)std::numeric_limits<uint64_t>::max());
}
long double nextlf() {
  return ((long double)my::Random::next()) /
         ((long double)std::numeric_limits<uint64_t>::max());
}
}; // namespace Random
} // namespace my
#endif