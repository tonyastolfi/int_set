#ifndef _INT_SET_H_
#define _INT_SET_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace internal {

template <typename Int> inline constexpr int log2(std::uint8_t n) {
  return (n & (1 << 7))
             ? 8
             : (n & (1 << 6))
                   ? 7
                   : (n & (1 << 5))
                         ? 6
                         : (n & (1 << 4))
                               ? 5
                               : (n & (1 << 3))
                                     ? 4
                                     : (n & (1 << 2))
                                           ? 3
                                           : (n & (1 << 1))
                                                 ? 2
                                                 : (n & (1 << 0)) ? 1 : 0;
}

#ifdef __GNUC__

inline int count_all_bits(unsigned long long n) __attribute__((always_inline)) {
  return __builtin_popcountll(n);
}

inline int trailing_zero_bits(unsigned long long n)
    __attribute__((always_inline)) {
  return __builtin_ctzll(n);
}

inline int leading_zero_bits(unsigned long long n)
    __attribute__((always_inline)) {
  return __builtin_clzll(n);
}

#else

namespace {

const int ctzll_mod_67[67] = {
    0,  0,  1,  39, 2,  15, 40, 23, 3,  12, 16, 59, 41, 19, 24, 54, 4,
    0,  13, 10, 17, 62, 60, 28, 42, 30, 20, 51, 25, 44, 55, 47, 5,  32,
    0,  38, 14, 22, 11, 58, 18, 53, 63, 9,  61, 27, 29, 50, 43, 46, 31,
    37, 21, 57, 52, 8,  26, 49, 45, 36, 56, 7,  48, 35, 6,  34};

const uint8_t clz_4_bit[1 << 4] = {4, 3, 2, 2, 1, 1, 1, 1,
                                   0, 0, 0, 0, 0, 0, 0, 0};

} // namespace

// Use Kernighan's method.
inline int count_all_bits(unsigned long long n) {
  int count;
  for (count = 0; n; ++count) {
    n &= n - 1; // clear the least significant bit set
  }
  return count;
}

inline int trailing_zero_bits(unsigned long long n) {
  return ctzll_mode_67[(n & -n) % 67];
}

inline int leading_zero_bits(unsigned long long n) {
  int count;
  if ((n & 0xFFFFFFFF00000000ULL) == 0) {
    count = 32;
    n <<= 32;
  } else {
    count = 0;
  }
  if ((n & 0xFFFF000000000000ULL) == 0) {
    count += 16;
    n <<= 16;
  }
  if ((n & 0xFF00000000000000ULL) == 0) {
    count += 8;
    n <<= 8;
  }
  if ((n & 0xF000000000000000ULL) == 0) {
    count += 4;
    n <<= 4;
  }
  count += clz_4_bit[n >> (64 - 4)];
  return count;
}
#endif

template <typename Key, typename Value, int Height> struct child_container {
  using type = std::unordered_map<Key, Value>;
};

template <typename T, std::size_t Size> struct array_map {
  using value_type = T;

  auto &&operator[](auto i) { return items_[i]; }
};

template <typename Key, typename Value> struct child_container<Key, Value, 1> {
  using type = std::array<Value, 4096>;
};

template <typename Key, typename Value> struct child_container<0>;

} // namespace internal

template <typename Int,
          // this is the height of the tree
          int MaxSizeLog64Log2 = internal::log2(((sizeof(Int) * 8) + 5) / 6)>
class int_set {
public:
  using value_type = Int;
  using size_type = std::size_t;

  static const Int nval = std::numeric_limits<Int>::max();

  int_set() {}

  struct const_iterator {};

  bool empty() const { return size_ == 0; }

  size_type size() const { TODO }

  const_iterator find(Int n) {
    if (!count(n)) {
      return {};
    }
    return {*this, n};
  }

  const_iterator lower_bound(Int n) const { TODO }

  const_iterator upper_bound(Int n) const { TODO }

  Int front() const { return min_; }

  Int back() const { return max_; }

  Int min_greater_equal(Int n) const {
    if (empty() || n > max_) {
      return nval;
    }
    if (n <= min_) {
      return min_;
    }
    const Int page_index = page(n);
    const Int page_offset = offset(n);
    auto it = children_.find(page_index);
    if (it == children_.end() || page_offset > it->second.back()) {
      const Int next_page = nonempty_.min_greater_equal(page_index + 1);
      return from_page_offset(next_page, children_.find(next_page).front());
    }
    return from_page_offset(page_index,
                            it->second.min_greater_equal(page_offset));
  }

  Int max_less_than(Int n) const {
    if (empty() || n <= min_) {
      return nval;
    }
    if (n > max_) {
      return max_;
    }
    const Int page_index = page(n);
    const Int page_offset = offset(n);
    auto it = children_.find(page_index);
    if (it == children_.end() || page_offset <= it->second.front()) {
      const Int prev_page = nonempty_.max_less_than(page_index);
      return from_page_offset(prev_page, children_.find(prev_page).back());
    }
    return from_page_offset(page_index, it->second.max_less_than(page_offset));
  }

  std::pair<const_iterator, bool> insert(Int n) {
    assert(n < nval);
    if (empty()) {
      min_ = max_ = n;
      size_ = 1;
      return;
    }
    if (n < min_) {
      // min <= max, so n != max.
      std::swap(n, min_);
      ++size_;
    } else if (n > max_) {
      // min <= max, so n != min.
      std::swap(n, max_);
      ++size_;
    }
    TODO - update size_ in all cases below;
    const Int page_index = page(n);
    auto &child = children_[page_index];
    if (child.empty()) {
      // if the child node that stores this element is currently empty, then
      // the insert below will be O(1), so we can afford to recurse on the
      // nonempty set.
      nonempty_.insert(page_index);
    }
    child.insert(offset(n));
  }

  auto emplace(Int n) { return insert(n); }

  size_type erase(Int n) {
    if (!count(n)) {
      return 0;
    }
    TODO - implement, updating size_
  }

  const_iterator begin() const { return cbegin(); }

  const_iterator begin() const { return cend(); }

  const_iterator cbegin() const { return lower_bound(0); }

  const_iterator cend() const { return {}; }

  size_type count(Int n) const {
    if (empty()) {
      return 0;
    }
    if (n < min_ || n > max_) {
      return 0;
    }
    if (n == min_ || n == max_) {
      return 1;
    }
    const Int page_index = page(n);
    if (nonempty_.count(page_index)) {
      return children_.find(page_index)->second.count(offset(n));
    }
    return 0;
  }

  bool operator[](size_type i) const { return count(i); }

private:
  // Tree:
  // 0:  64 64 64 ... 64 64 64 ...
  // 1:  4096         4096        ...
  // 2:  16.7M                        ...
  // 3:  2^48
  // 4:  2^96 > 64
  //
  using child_type = int_set<Int, MaxSizeLog64Log2 - 1>;

  using child_container =
      internal::child_continer<Int, child_type, MaxSizeLog64Log2 - 1>::type;

  Int page(Int n) { TODO }
  Int offset(Int n) { TODO }

  Int size_ = 0;
  Int min_, max_;

  // The set of non-empty child sets by page index.  Size is n^0.5
  child_type nonempty_;

  // A collection of n^0.5 sets each of size n^0.5
  child_container children_;
};

// Non-recursive case; use builtins (clz, ctz) if available and 64-bit bit map.
template <typename Int> class int_set<Int, 0> {
public:
  using value_type = Int;
  using size_type = std::size_t;

  class const_iterator {
  public:
    using value_type = Int;

    const_iterator() : value_(64), set_(nullptr) {}

    const_iterator(const int_set &set, Int value) : value_(value), set_(&set) {}

    value_type operator*() const { return value_; }

    bool operator==(const const_iterator &that) const {
      return value_ == that.value_ && set_ == that.set_;
    }

    bool operator<(const const_iterator &that) const {
      return (value_ < that.value_ && (that.value_ == 64 || set_ == that.set_));
    }

    const_iterator &operator++() {
      assert(value_ < 64);
      value_ = set_->min_greater_equal(value_ + 1);
      return *this;
    }

    const_iterator &operator--() {
      assert(value_);
      value_ = set_->max_less_than(value_);
      return *this;
    }

    // TODO - add difference_type and operator-

  private:
    Int value_;
    const int_set *set_;
  };

  size_type size() const { return internal::count_all_bits(bitmap_); }

  const_iterator find(Int n) const {
    return (bitmap_ & bit_mask(n)) ? const_iterator(*this, n) : {};
  }

  const_iterator lower_bound(Int n) const {
    return const_iterator(*this, min_greater_equal(n));
  }

  const_iterator upper_bound(Int n) const {
    return const_iterator(*this, min_greater_equal(n + 1));
  }

  Int front() const { return min_greater_equal(0); }

  Int back() const { return 64 - internal::leading_zero_bits(bitmap_); }

  Int min_greater_equal(Int n) const {
    const Int c = internal::trailing_zero_bits(bitmap_ & greater_equal_mask(n));
    return c ? c : 64;
  }

  Int max_less_than(Int n) const {
    return 64 - internal::leading_zero_bits(bitmap_ & less_than_mask(n));
  }

  std::pair<const_iterator, bool> insert(Int n) {
    const std::uint64_t bit = bit_mask(n);
    const bool was_set = bitmap_ & bit;
    bitmap_ |= bit;
    return {const_iterator(&this, n), was_set};
  }

  auto emplace(Int n) { return insert(n); }

  size_type erase(Int n) {
    const size_type prior_count = count(n);
    bitmap &= ~bit_mask(n);
    return prior_count;
  }

  const_iterator begin() const { return cbegin(); }

  const_iterator begin() const { return cend(); }

  const_iterator cbegin() const { return lower_bound(0); }

  const_iterator cend() const { return {}; }

  size_type count(Int n) { return (bitmap_ & bit_mask(n)) >> n; }

  bool operator[](size_type i) const { return count(i); }

private:
  static Int bit_mask(const Int n) __attribute__((always_inline)) {
    return Int{1} << n;
  }

  static Int less_than_mask(const Int n) __attribute__((always_inline)) {
    return bit_mask(n) - 1;
  }

  static Int greater_equal_mask(const Int n) __attribute__((always_inline)) {
    return ~less_than_mask(n);
  }

  std::uint64_t bitmap_ = 0;
};

#endif // _INT_SET_H_
