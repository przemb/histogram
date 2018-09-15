// Copyright 2015-2017 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_STORAGE_ADAPTIVE_HPP
#define BOOST_HISTOGRAM_STORAGE_ADAPTIVE_HPP

#include <algorithm>
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/cstdint.hpp>
#include <boost/histogram/detail/buffer.hpp>
#include <boost/histogram/detail/meta.hpp>
#include <boost/histogram/storage/weight_counter.hpp>
#include <boost/histogram/weight.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/mp11.hpp>
#include <iostream>
#include <limits>
#include <memory>
#include <type_traits>

// forward declaration for serialization
namespace boost {
namespace serialization {
class access;
}
} // namespace boost

namespace boost {
namespace histogram {

namespace detail {
template <typename T>
bool safe_increase(T& t) {
  if (t < std::numeric_limits<T>::max()) {
    ++t;
    return true;
  }
  return false;
}

template <typename T, typename U>
bool safe_assign(T& t, const U& u) {
  if (std::numeric_limits<T>::max() < std::numeric_limits<U>::max() &&
      std::numeric_limits<T>::max() < u)
    return false;
  t = static_cast<T>(u);
  return true;
}

template <typename T, typename U>
bool safe_radd(T& t, const U& u) {
  BOOST_ASSERT(t >= 0);
  BOOST_ASSERT(u >= 0);
  // static_cast converts back from signed to unsigned integer
  if (static_cast<T>(std::numeric_limits<T>::max() - t) < u) return false;
  t += static_cast<T>(u); // static_cast to suppress conversion warning
  return true;
}

}

template <class Allocator>
class adaptive_storage {
  static_assert(
      std::is_same<typename std::allocator_traits<Allocator>::pointer,
                   typename std::allocator_traits<Allocator>::value_type*>::value,
      "adaptive_storage requires allocator with trivial pointer type");

public:
  using allocator_type = Allocator;
  using element_type = weight_counter<double>;
  using const_reference = element_type;

  using wcount = weight_counter<double>;
  using mp_int = boost::multiprecision::number<
    boost::multiprecision::cpp_int_backend<
      0, 0, boost::multiprecision::signed_magnitude,
      boost::multiprecision::unchecked,
      typename std::allocator_traits<Allocator>::template rebind_alloc<
        boost::multiprecision::limb_type
      >
    >
  >;

  using types = mp11::mp_list<void, uint8_t, uint16_t, uint32_t,uint64_t,
                              mp_int, wcount>;

  template <typename T>
  static constexpr char type_index() {
    return static_cast<char>(mp11::mp_find<types, T>::value);
  }

  template <typename T>
  using next_type = mp11::mp_at_c<types, (adaptive_storage::type_index<T>() + 1)>;

private:
  struct buffer_type {
    allocator_type alloc;
    char type;
    std::size_t size;
    void* ptr;
    buffer_type(std::size_t s = 0, const allocator_type& a = allocator_type())
        : alloc(a), type(0), size(s), ptr(nullptr) {}

    template <typename T, typename U = T>
    void create(mp11::mp_identity<T>, const U* init = nullptr) {
      using alloc_type = typename std::allocator_traits<
        allocator_type>::template rebind_alloc<T>;
      alloc_type a(alloc); // rebind allocator
      ptr = init ? detail::create_buffer_from_iter(a, size, init) :
                   detail::create_buffer(a, size, 0);
      type = type_index<T>();
    }

    template <typename U = mp_int>
    void create(mp11::mp_identity<mp_int>, const U* init = nullptr) {
      using alloc_type = typename std::allocator_traits<
        allocator_type>::template rebind_alloc<mp_int>;
      alloc_type a(alloc); // rebound allocator for buffer
      // mp_int has no ctor with an allocator instance, cannot pass state :(
      // typename mp_int::backend_type::allocator_type a2(alloc);
      ptr = init ? detail::create_buffer_from_iter(a, size, init) :
                   detail::create_buffer(a, size, 0);
      type = type_index<mp_int>();
    }

    template <typename U = void>
    void create(mp11::mp_identity<void>, const U* init = nullptr) {
      boost::ignore_unused(init);
      BOOST_ASSERT(!init); // init is always a nullptr in this specialization
      ptr = nullptr;
      type = type_index<void>();
    }
  };

public:
  ~adaptive_storage() { apply(destroyer(), buffer_); }

  adaptive_storage(const adaptive_storage& o) {
    apply(replacer(), o.buffer_, buffer_);
  }

  adaptive_storage& operator=(const adaptive_storage& o) {
    if (this != &o) { apply(replacer(), o.buffer_, buffer_); }
    return *this;
  }

  adaptive_storage(adaptive_storage&& o) : buffer_(std::move(o.buffer_)) {
    o.buffer_.type = 0;
    o.buffer_.size = 0;
    o.buffer_.ptr = nullptr;
  }

  adaptive_storage& operator=(adaptive_storage&& o) {
    if (this != &o) { std::swap(buffer_, o.buffer_); }
    return *this;
  }

  template <typename S, typename = detail::requires_storage<S>>
  explicit adaptive_storage(const S& s) : buffer_(s.size(), s.get_allocator()) {
    buffer_.create(mp11::mp_identity<wcount>());
    auto it = reinterpret_cast<wcount*>(buffer_.ptr);
    const auto end = it + size();
    std::size_t i = 0;
    while (it != end) *it++ = s[i++];
  }

  template <typename S, typename = detail::requires_storage<S>>
  adaptive_storage& operator=(const S& s) {
    // no check for self-assign needed, since S is different type
    apply(destroyer(), buffer_);
    buffer_.alloc = s.get_allocator();
    buffer_.size = s.size();
    buffer_.create(mp11::mp_identity<void>());
    for (std::size_t i = 0; i < size(); ++i) { add(i, s[i]); }
    return *this;
  }

  explicit adaptive_storage(const allocator_type& a = allocator_type()) : buffer_(0, a) {
    buffer_.create(mp11::mp_identity<void>());
  }

  allocator_type get_allocator() const { return buffer_.alloc; }

  void reset(std::size_t s) {
    apply(destroyer(), buffer_);
    buffer_.size = s;
    buffer_.create(mp11::mp_identity<void>());
  }

  std::size_t size() const { return buffer_.size; }

  void increase(std::size_t i) {
    BOOST_ASSERT(i < size());
    apply(increaser(), buffer_, i);
  }

  template <typename T>
  void add(std::size_t i, const T& x) {
    BOOST_ASSERT(i < size());
    apply(adder(), buffer_, i, x);
  }

  const_reference operator[](std::size_t i) const {
    return apply(getter(), buffer_, i);
  }

  bool operator==(const adaptive_storage& o) const {
    if (size() != o.size()) return false;
    return apply(comparer(), buffer_, o.buffer_);
  }

  // precondition: storages have same size
  adaptive_storage& operator+=(const adaptive_storage& o) {
    BOOST_ASSERT(o.size() == size());
    if (this == &o) {
      /*
        Self-adding is a special-case, because the source buffer ptr may be
        invalided by growth. We avoid this by making a copy of the source.
        This is the simplest solution, but expensive. The cost is ok, because
        self-adding is only used by the unit-tests. It does not occur
        frequently in real applications.
      */
      const auto copy = o;
      apply(buffer_adder(), copy.buffer_, buffer_);
    } else {
      apply(buffer_adder(), o.buffer_, buffer_);
    }
    return *this;
  }

  // precondition: storages have same size
  template <typename S>
  adaptive_storage& operator+=(const S& o) {
    BOOST_ASSERT(o.size() == size());
    for (std::size_t i = 0; i < size(); ++i) add(i, o[i]);
    return *this;
  }

  adaptive_storage& operator*=(const double x) {
    apply(multiplier(), buffer_, x);
    return *this;
  }

  // used by unit tests, not part of generic storage interface
  template <typename T>
  adaptive_storage(std::size_t s, const T* p, const allocator_type& a = allocator_type())
      : buffer_(s, a) {
    buffer_.create(mp11::mp_identity<T>(), p);
  }

private:
  struct destroyer {
    template <typename T, typename Buffer>
    void operator()(T* tp, Buffer& b) {
      using alloc_type = typename std::allocator_traits<
        allocator_type>::template rebind_alloc<T>;
      alloc_type a(b.alloc); // rebind allocator
      detail::destroy_buffer(a, tp, b.size);
    }

    template <typename Buffer>
    void operator()(void*, Buffer&) {}
  };

  template <typename F, typename B, typename... Ts>
  static typename std::result_of<F(void*, B&&, Ts&&...)>::type apply(F&& f, B&& b, Ts&&... ts) {
    // this is intentionally not a switch, the if-chain is faster in benchmarks
    if (b.type == type_index<uint8_t>())
      return f(reinterpret_cast<uint8_t*>(b.ptr), std::forward<B>(b),
               std::forward<Ts>(ts)...);
    if (b.type == type_index<uint16_t>())
      return f(reinterpret_cast<uint16_t*>(b.ptr), std::forward<B>(b),
               std::forward<Ts>(ts)...);
    if (b.type == type_index<uint32_t>())
      return f(reinterpret_cast<uint32_t*>(b.ptr), std::forward<B>(b),
               std::forward<Ts>(ts)...);
    if (b.type == type_index<uint64_t>())
      return f(reinterpret_cast<uint64_t*>(b.ptr), std::forward<B>(b),
               std::forward<Ts>(ts)...);
    if (b.type == type_index<mp_int>())
      return f(reinterpret_cast<mp_int*>(b.ptr), std::forward<B>(b),
               std::forward<Ts>(ts)...);
    if (b.type == type_index<wcount>())
      return f(reinterpret_cast<wcount*>(b.ptr), std::forward<B>(b),
               std::forward<Ts>(ts)...);
    // b.type == 0 is intentionally the last in the chain, because it is rarely
    // triggered
    return f(b.ptr, std::forward<B>(b), std::forward<Ts>(ts)...);
  }

  struct replacer {
    template <typename T, typename OBuffer, typename Buffer>
    void operator()(T* optr, const OBuffer& ob, Buffer& b) {
      if (b.size == ob.size && b.type == ob.type) {
        std::copy(optr, optr + ob.size, reinterpret_cast<T*>(b.ptr));
      } else {
        apply(destroyer(), b);
        b.alloc = ob.alloc;
        b.size = ob.size;
        b.create(mp11::mp_identity<T>(), optr);
      }
    }

    template <typename OBuffer, typename Buffer>
    void operator()(void*, const OBuffer& ob, Buffer& b) {
      apply(destroyer(), b);
      b.type = 0;
      b.size = ob.size;
    }
  };

  struct increaser {
    template <typename T, typename Buffer>
    void operator()(T* tp, Buffer& b, std::size_t i) {
      if (!detail::safe_increase(tp[i])) {
        using U = next_type<T>;
        b.create(mp11::mp_identity<U>(), tp);
        destroyer()(tp, b);
        ++reinterpret_cast<U*>(b.ptr)[i];
      }
    }

    template <typename Buffer>
    void operator()(void*, Buffer& b, std::size_t i) {
      using U = next_type<void>;
      b.create(mp11::mp_identity<U>());
      ++reinterpret_cast<U*>(b.ptr)[i];
    }

    template <typename Buffer>
    void operator()(mp_int* tp, Buffer&, std::size_t i) {
      ++tp[i];
    }

    template <typename Buffer>
    void operator()(wcount* tp, Buffer&, std::size_t i) {
      ++tp[i];
    }
  };

  struct adder {
    template <typename Buffer, typename U>
    void if_U_is_integral(std::true_type, mp_int* tp, Buffer&, std::size_t i, const U& x) {
      tp[i] += static_cast<mp_int>(x);
    }

    template <typename T, typename Buffer, typename U>
    void if_U_is_integral(std::true_type, T* tp, Buffer& b, std::size_t i, const U& x) {
      if (!detail::safe_radd(tp[i], x)) {
        using V = next_type<T>;
        b.create(mp11::mp_identity<V>(), tp);
        destroyer()(tp, b);
        if_U_is_integral(std::true_type(), reinterpret_cast<V*>(b.ptr), b, i, x);
      }
    }

    template <typename T, typename Buffer, typename U>
    void if_U_is_integral(std::false_type, T* tp, Buffer& b, std::size_t i, const U& x) {
      b.create(mp11::mp_identity<wcount>(), tp);
      destroyer()(tp, b);
      operator()(reinterpret_cast<wcount*>(b.ptr), b, i, x);
    }

    template <typename T, typename Buffer, typename U>
    void operator()(T* tp, Buffer& b, std::size_t i, const U& x) {
      if_U_is_integral(std::integral_constant<bool,
                       (std::is_integral<U>::value ||
                        std::is_same<U, mp_int>::value)>(), tp, b, i, x);
    }

    template <typename Buffer, typename U>
    void operator()(void*, Buffer& b, std::size_t i, const U& x) {
      using V = next_type<void>;
      b.create(mp11::mp_identity<V>());
      operator()(reinterpret_cast<V*>(b.ptr), b, i, x);
    }

    template <typename Buffer, typename U>
    void operator()(wcount* tp, Buffer&, std::size_t i, const U& x) {
      tp[i] += x;
    }

    template <typename Buffer>
    void operator()(wcount* tp, Buffer&, std::size_t i, const mp_int& x) {
      tp[i] += static_cast<double>(x);
    }
  };

  struct buffer_adder {
    template <typename T, typename OBuffer, typename Buffer>
    void operator()(T* tp, const OBuffer&, Buffer& b) {
      for (std::size_t i = 0; i < b.size; ++i) { apply(adder(), b, i, tp[i]); }
    }

    template <typename OBuffer, typename Buffer>
    void operator()(void*, const OBuffer&, Buffer&) {}
  };

  struct getter {
    template <typename T, typename Buffer>
    wcount operator()(T* tp, Buffer&, std::size_t i) {
      return static_cast<wcount>(tp[i]);
    }

    template <typename Buffer>
    wcount operator()(void*, Buffer&, std::size_t) {
      return static_cast<wcount>(0);
    }
  };

  // precondition: buffers already have same size
  struct comparer {
    struct inner {
      template <typename U, typename OBuffer, typename T>
      bool operator()(const U* optr, const OBuffer& ob, const T* tp) {
        return std::equal(optr, optr + ob.size, tp);
      }

      template <typename U, typename OBuffer>
      bool operator()(const U* optr, const OBuffer& ob, const void*) {
        return std::all_of(optr, optr + ob.size, [](const U& x) { return x == 0; });
      }

      template <typename OBuffer, typename T>
      bool operator()(const void*, const OBuffer& ob, const T* tp) {
        return std::all_of(tp, tp + ob.size, [](const T& x) { return x == 0; });
      }

      template <typename OBuffer>
      bool operator()(const void*, const OBuffer&, const void*) {
        return true;
      }
    };

    template <typename T, typename Buffer, typename OBuffer>
    bool operator()(const T* tp, const Buffer& b, const OBuffer& ob) {
      BOOST_ASSERT(b.size == ob.size);
      return apply(inner(), ob, tp);
    }
  };

  struct multiplier {
    template <typename T, typename Buffer>
    void operator()(T* tp, Buffer& b, const double x) {
      b.create(mp11::mp_identity<wcount>(), tp);
      operator()(reinterpret_cast<wcount*>(b.ptr), b, x);
    }

    template <typename Buffer>
    void operator()(void*, Buffer&, const double) {}

    template <typename Buffer>
    void operator()(wcount* tp, Buffer& b, const double x) {
      for (auto end = tp + b.size; tp != end; ++tp) *tp *= x;
    }
  };

  buffer_type buffer_;

  template <typename UAllocator>
  friend class adaptive_storage;
  friend class python_access;
  friend class ::boost::serialization::access;
  template <class Archive>
  void serialize(Archive&, unsigned);
};

} // namespace histogram
} // namespace boost

#endif
