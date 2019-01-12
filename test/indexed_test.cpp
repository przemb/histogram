// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/core/lightweight_test.hpp>
#include <boost/histogram/axis/integer.hpp>
#include <boost/histogram/axis/variable.hpp>
#include <boost/histogram/histogram.hpp>
#include <boost/histogram/indexed.hpp>
#include <boost/histogram/literals.hpp>
#include <boost/mp11.hpp>
#include <iterator>
#include <vector>
#include "utility_histogram.hpp"

using namespace boost::histogram;
using namespace boost::histogram::literals;
using namespace boost::mp11;

template <class IsDynamic, class IncludeExtraBins>
void run_1d_tests(mp_list<IsDynamic, IncludeExtraBins>) {
  auto h = make(IsDynamic(), axis::integer<>(0, 3));
  h(-1, weight(1));
  h(0, weight(2));
  h(1, weight(3));
  h(2, weight(4));
  h(3, weight(5));

  auto ind = indexed(h, IncludeExtraBins());
  auto it = ind.begin();
  BOOST_TEST_EQ(it->size(), 1);

  if (IncludeExtraBins()) {
    BOOST_TEST_EQ(it->operator[](0), -1);
    BOOST_TEST_EQ(**it, 1);
    BOOST_TEST_EQ(it->bin(0), h.axis()[-1]);
    ++it;
  }
  BOOST_TEST_EQ(it->operator[](0), 0);
  BOOST_TEST_EQ(**it, 2);
  BOOST_TEST_EQ(it->bin(0), h.axis()[0]);
  ++it;
  BOOST_TEST_EQ(it->operator[](0), 1);
  BOOST_TEST_EQ(**it, 3);
  BOOST_TEST_EQ(it->bin(0), h.axis()[1]);
  ++it;
  BOOST_TEST_EQ(it->operator[](0), 2);
  BOOST_TEST_EQ(**it, 4);
  BOOST_TEST_EQ(it->bin(0), h.axis()[2]);
  ++it;
  if (IncludeExtraBins()) {
    BOOST_TEST_EQ(it->operator[](0), 3);
    BOOST_TEST_EQ(**it, 5);
    BOOST_TEST_EQ(it->bin(0), h.axis()[3]);
    ++it;
  }
  BOOST_TEST(it == ind.end());

  for (auto&& x : indexed(h, IncludeExtraBins())) *x = 0;

  for (auto&& x : indexed(static_cast<const decltype(h)&>(h), IncludeExtraBins()))
    BOOST_TEST_EQ(*x, 0);
}

template <class IsDynamic, class IncludeExtraBins>
void run_3d_tests(mp_list<IsDynamic, IncludeExtraBins>) {
  auto h = make_s(IsDynamic(), std::vector<int>(), axis::integer<>(0, 2),
                  axis::integer<int, axis::null_type, axis::option_type::none>(0, 3),
                  axis::integer<int, axis::null_type, axis::option_type::overflow>(0, 4));

  for (int i = -1; i < 3; ++i)
    for (int j = -1; j < 4; ++j)
      for (int k = -1; k < 5; ++k) h(i, j, k, weight(i * 100 + j * 10 + k));

  auto ind = indexed(h, IncludeExtraBins());
  auto it = ind.begin();
  BOOST_TEST_EQ(it->size(), 3);

  // imitate iteration order of indexed loop
  for (int k = 0; k < 4 + IncludeExtraBins(); ++k)
    for (int j = 0; j < 3; ++j)
      for (int i = -IncludeExtraBins(); i < 2 + IncludeExtraBins(); ++i) {
        BOOST_TEST_EQ(it->operator[](0), i);
        BOOST_TEST_EQ(it->operator[](1), j);
        BOOST_TEST_EQ(it->operator[](2), k);
        BOOST_TEST_EQ(it->bin(0_c), h.axis(0_c)[i]);
        BOOST_TEST_EQ(it->bin(1_c), h.axis(1_c)[j]);
        BOOST_TEST_EQ(it->bin(2_c), h.axis(2_c)[k]);
        BOOST_TEST_EQ(**it, i * 100 + j * 10 + k);
        ++it;
      }
  BOOST_TEST(it == ind.end());
}

template <class IsDynamic, class IncludeExtraBins>
void run_density_tests(mp_list<IsDynamic, IncludeExtraBins>) {
  auto ax = axis::variable<>({0.0, 0.1, 0.3, 0.6});
  auto ay = axis::integer<int>(0, 2);
  auto az = ax;
  auto h = make_s(IsDynamic(), std::vector<int>(), ax, ay, az);

  // fill uniformly
  for (auto&& x : h) x = 1;

  for (auto x : indexed(h, IncludeExtraBins())) {
    BOOST_TEST_EQ(x.density(), *x / (x.bin(0).width() * x.bin(2).width()));
  }
}

int main() {
  mp_for_each<
      mp_product<mp_list, mp_list<mp_false, mp_true>, mp_list<mp_false, mp_true>>>(
      [](auto&& x) {
        run_1d_tests(x);
        run_3d_tests(x);
        run_density_tests(x);
      });
  return boost::report_errors();
}