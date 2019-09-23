// Copyright 2015-2017 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_OSTREAM_HPP
#define BOOST_HISTOGRAM_OSTREAM_HPP

#include <boost/histogram/accumulators/ostream.hpp>
#include <boost/histogram/axis/ostream.hpp>
#include <boost/histogram/fwd.hpp>
#include <iosfwd>
#include <type_traits>
//#include <boost/histogram/display.hpp>

/**
  \file boost/histogram/ostream.hpp

  A simple streaming operator for the histogram type. The text representation is
  rudimentary and not guaranteed to be stable between versions of Boost.Histogram. This
  header is not included by any other header and must be explicitly included to use the
  streaming operator.

  To you use your own, simply include your own implementation instead of this header.
 */

#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED

namespace boost {
namespace histogram {
namespace detail {

// template <class T>
// std::false_type f_alternative_exist_f (std::ostream &, T const &, long);

// template <class T>
// auto f_alternative_exist_f (std::ostream & os, T const & t, int)
//    -> decltype( ostream_prototype(os, t), std::true_type{} );

// template <class T>
// using f_alternative_exist
//    = decltype(f_alternative_exist_f(std::declval<std::ostream &>(),
//                                     std::declval<T>(), 0));


// template<class T>
// static constexpr bool f_alternative_exist_v = 0; // f_alternative_exist<T>::value;




// template<class T, typename CharT, typename Traits, typename A, typename S, 
// typename std::enable_if <((std::rank<T>::value > 1) || (false == alternative_ostream_v<T>)),
// int>::type = 0>
// void ostream_prototype(std::basic_ostream<CharT, Traits>& os,
//                        const histogram<A, S>& h) {
//   os << "histogram(";
//   h.for_each_axis([&](const auto& a) { os << "\n  " << a << ","; });
//   std::size_t i = 0;
//   for (auto&& x : h) os << "\n  " << i++ << ": " << x;
//   os << (h.rank() ? "\n)" : ")");
// }

} // namespace detail

template<class T, 
typename CharT, typename Traits, typename A, typename S,
typename std::enable_if <((std::rank<T>::value != 1)),
int>::type = 0>
void ostream_prototype(std::basic_ostream<CharT, Traits>& os,
                       const histogram<A, S>& h) {
  os << "histogram(";
  h.for_each_axis([&](const auto& a) { os << "\n  " << a << ","; });
  std::size_t i = 0;
  for (auto&& x : h) os << "\n  " << i++ << ": " << x;
  os << (h.rank() ? "\n)" : ")");
}

template <typename CharT, typename Traits, typename A, typename S>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              const histogram<A, S>& h) {
  ostream_prototype<decltype(h)>(os, h);
  return os;
}

} // namespace histogram
} // namespace boost

#endif // BOOST_HISTOGRAM_DOXYGEN_INVOKED

#endif
