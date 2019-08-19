// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_SPAN_HPP
#define BOOST_HISTOGRAM_DETAIL_SPAN_HPP

#if __has_include(<span>)
#include <span>

namespace boost {
namespace histogram {
namespace detail {
using std::span;
} // namespace detail
} // namespace histogram
} // namespace boost

#else

#include <array>
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/histogram/detail/non_member_container_access.hpp>
#include <initializer_list>
#include <iterator>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

static constexpr std::size_t dynamic_extent = ~static_cast<std::size_t>(0);

template <class T, std::size_t N>
class span_base {
public:
  constexpr T* data() noexcept { return begin_; }
  constexpr const T* data() const noexcept { return begin_; }
  constexpr std::size_t size() const noexcept { return N; }

protected:
  constexpr span_base(T* b, std::size_t s) noexcept : begin_(b) {
    static_assert(N == s, "sizes do not match");
  }
  constexpr void set(T* b, std::size_t s) noexcept {
    begin_ = b;
    static_assert(N == s, "sizes do not match");
  }

private:
  T* begin_;
};

template <class T>
class span_base<T, dynamic_extent> {
public:
  constexpr T* data() noexcept { return begin_; }
  constexpr const T* data() const noexcept { return begin_; }
  constexpr std::size_t size() const noexcept { return size_; }

protected:
  constexpr span_base(T* b, std::size_t s) noexcept : begin_(b), size_(s) {}

  constexpr void set(T* b, std::size_t s) noexcept {
    begin_ = b;
    size_ = s;
  }

private:
  T* begin_;
  std::size_t size_;
};

template <class T, std::size_t Extent = dynamic_extent>
class span : public span_base<T, Extent> {
  using base = span_base<T, Extent>;

public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using index_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  static constexpr std::size_t extent = Extent;

  template <std::size_t _ = extent,
            class = std::enable_if_t<(_ == 0 || _ == dynamic_extent)> >
  constexpr span() noexcept : base(nullptr, 0) {}

  constexpr span(pointer first, pointer last)
      : span(first, static_cast<std::size_t>(last - first)) {
    BOOST_ASSERT(extent == dynamic_extent ||
                 static_cast<difference_type>(extent) == (last - first));
  }

  constexpr span(pointer ptr, index_type count) : base(ptr, count) {}

  template <std::size_t N>
  constexpr span(element_type (&arr)[N]) noexcept : span(data(arr), N) {
    static_assert(extent == dynamic_extent || extent == N, "static sizes do not match");
  }

  template <std::size_t N,
            class = std::enable_if_t<(extent == dynamic_extent || extent == N)> >
  constexpr span(std::array<value_type, N>& arr) noexcept : span(data(arr), N) {}

  template <std::size_t N,
            class = std::enable_if_t<(extent == dynamic_extent || extent == N)> >
  constexpr span(const std::array<value_type, N>& arr) noexcept : span(data(arr), N) {}

  template <
      class Container,
      class = std::enable_if_t<std::is_convertible<
          decltype(size(std::declval<Container>()), data(std::declval<Container>())),
          pointer>::value> >
  constexpr span(const Container& cont) : span(data(cont), size(cont)) {}

  template <
      class Container,
      class = std::enable_if_t<std::is_convertible<
          decltype(size(std::declval<Container>()), data(std::declval<Container>())),
          pointer>::value> >
  constexpr span(Container& cont) : span(data(cont), size(cont)) {}

  template <class U, std::size_t N,
            class = std::enable_if_t<((extent == dynamic_extent || extent == N) &&
                                      std::is_convertible<U, element_type>::value)> >
  constexpr span(const span<U, N>& s) noexcept : span(s.data(), s.size()) {}

  constexpr span(const span& other) noexcept = default;

  constexpr iterator begin() { return base::data(); }
  constexpr const_iterator begin() const { return base::data(); }
  constexpr const_iterator cbegin() const { return base::data(); }

  constexpr iterator end() { return base::data() + base::size(); }
  constexpr const_iterator end() const { return base::data() + base::size(); }
  constexpr const_iterator cend() const { return base::data() + base::size(); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return reverse_iterator(end()); }
  const_reverse_iterator crbegin() { return reverse_iterator(end()); }

  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const { return reverse_iterator(begin()); }
  const_reverse_iterator crend() { return reverse_iterator(begin()); }

  constexpr reference front() { *base::data(); }
  constexpr reference back() { *(base::data() + base::size() - 1); }

  constexpr reference operator[](index_type idx) const { return base::data()[idx]; }

  // constexpr pointer data() const noexcept { return base::data(); }
  // constexpr std::size_t size() const noexcept { return base::size(); }

  constexpr std::size_t size_bytes() const noexcept {
    return base::size() * sizeof(element_type);
  }

#if BOOST_WORKAROUND(BOOST_CLANG, >= 0)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++17-extensions"
#endif

  BOOST_ATTRIBUTE_NODISCARD constexpr bool empty() const noexcept {
    return base::size() == 0;
  }

#if BOOST_WORKAROUND(BOOST_CLANG, >= 0)
#pragma GCC diagnostic pop
#endif

  template <std::size_t Count>
  constexpr span<element_type, Count> first() const {
    BOOST_ASSERT(Count <= base::size());
    return span<element_type, Count>(base::data(), Count);
  }

  constexpr span<element_type, dynamic_extent> first(std::size_t count) const {
    BOOST_ASSERT(count <= base::size());
    return span<element_type, dynamic_extent>(base::data(), count);
  }

  template <std::size_t Count>
  constexpr span<element_type, Count> last() const {
    BOOST_ASSERT(Count <= base::size());
    return span<element_type, Count>(base::data() + base::size() - Count, Count);
  }

  constexpr span<element_type, dynamic_extent> last(std::size_t count) const {
    BOOST_ASSERT(count <= base::size());
    return span<element_type, dynamic_extent>(base::data() + base::size() - count, count);
  }

  template <std::size_t Offset, std::size_t Count = dynamic_extent>
  constexpr span<element_type,
                 (Count != dynamic_extent
                      ? Count
                      : (extent != dynamic_extent ? extent - Offset : dynamic_extent))>
  subspan() const {
    BOOST_ASSERT(Offset <= base::size());
    constexpr std::size_t E =
        (Count != dynamic_extent
             ? Count
             : (extent != dynamic_extent ? extent - Offset : dynamic_extent));
    BOOST_ASSERT(E == dynamic_extent || E <= base::size());
    return span<element_type, E>(base::data() + Offset,
                                 Count == dynamic_extent ? base::size() - Offset : Count);
  }

  constexpr span<element_type, dynamic_extent> subspan(
      std::size_t offset, std::size_t count = dynamic_extent) const {
    BOOST_ASSERT(offset <= base::size());
    const std::size_t s = count == dynamic_extent ? base::size() - offset : count;
    BOOST_ASSERT(s <= base::size());
    return span<element_type, dynamic_extent>(base::data() + offset, s);
  }

private:
};

#endif

template <class T>
span<T> make_span(T* begin, T* end) {
  return span<T>{begin, end};
}

template <class T>
span<T> make_span(T* begin, std::size_t size) {
  return span<T>{begin, size};
}

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
