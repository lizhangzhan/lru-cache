/// The MIT License (MIT)
/// Copyright (c) 2016 Peter Goldsborough and Markus Engel
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.


#ifndef LRU_STATISTICS_HPP
#define LRU_STATISTICS_HPP

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "lru/error.hpp"
#include "lru/internal/utility.hpp"
#include "lru/key-statistics.hpp"

namespace LRU {
namespace Internal {
template <typename>
class StatisticsMutator;
}

template <typename Key>
class Statistics {
 public:
  using size_t = std::size_t;
  using InitializerList = std::initializer_list<Key>;

  Statistics() noexcept : _total_accesses(0), _total_hits(0) {
  }

  template <typename... Keys,
            typename = std::enable_if_t<Internal::all_of_type<Key, Keys...>>>
  explicit Statistics(Keys&&... keys) : Statistics() {
    Internal::for_each([this](auto&& key) { monitor(key); },
                       std::forward<Keys>(keys)...);
  }

  template <typename Range>
  explicit Statistics(const Range& range)
  : Statistics(std::begin(range), std::end(range)) {
  }

  template <typename Iterator>
  Statistics(Iterator begin, Iterator end) : Statistics() {
    for (; begin != end; ++begin) {
      monitor(*begin);
    }
  }

  Statistics(InitializerList list)  // NOLINT(runtime/explicit)
      : Statistics(list.begin(), list.end()) {
  }

  size_t total_accesses() const noexcept {
    return _total_accesses;
  }

  size_t total_hits() const noexcept {
    return _total_hits;
  }

  size_t total_misses() const noexcept {
    return total_accesses() - total_hits();
  }

  double hit_rate() const noexcept {
    return static_cast<double>(total_hits()) / total_accesses();
  }

  double miss_rate() const noexcept {
    return 1 - hit_rate();
  }

  size_t hits_for(const Key& key) const {
    return stats_for(key).hits;
  }

  size_t misses_for(const Key& key) const {
    return stats_for(key).misses;
  }

  size_t accesses_for(const Key& key) const {
    return stats_for(key).accesses();
  }

  const KeyStatistics& stats_for(const Key& key) const {
    auto iterator = _hit_map.find(key);
    if (iterator == _hit_map.end()) {
      throw LRU::Error::UnmonitoredKey();
    }

    return iterator->second;
  }

  const KeyStatistics& operator[](const Key& key) const {
    return stats_for(key);
  }

  void monitor(const Key& key) {
    // emplace does nothing if the key is already present
    _hit_map.emplace(key, KeyStatistics());
  }

  void unmonitor(const Key& key) {
    _hit_map.erase(key);
  }

  void unmonitor_all() {
    _hit_map.clear();
  }

  bool is_monitoring(const Key& key) const noexcept {
    return _hit_map.count(key);
  }

  size_t number_of_monitored_keys() const noexcept {
    return _hit_map.size();
  }

  size_t is_monitoring_keys() const noexcept {
    return !_hit_map.empty();
  }

 private:
  template <typename>
  friend class Internal::StatisticsMutator;

  using HitMap = std::unordered_map<Key, KeyStatistics>;

  size_t _total_accesses;
  size_t _total_hits;

  HitMap _hit_map;
};
}  // namespace LRU

#endif  // LRU_STATISTICS_HPP
