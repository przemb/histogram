// Copyright 2015-2016 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TEST_MODULE static_histogram_test
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/histogram/static_histogram.hpp>
#include <boost/histogram/storage/container_storage.hpp>
#include <boost/histogram/storage/adaptive_storage.hpp>
#include <boost/histogram/utility.hpp>
#include <boost/histogram/axis_ostream_operators.hpp>
#include <boost/histogram/serialization.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <limits>
#include <sstream>
#include <vector>

using namespace boost::histogram;
namespace mpl = boost::mpl;

BOOST_AUTO_TEST_CASE(init_0)
{
    auto h = static_histogram<
      mpl::vector<integer_axis>,
      adaptive_storage
    >();
    BOOST_CHECK_EQUAL(h.dim(), 1);
    BOOST_CHECK_EQUAL(h.size(), 0);
    auto h2 = static_histogram<
      mpl::vector<integer_axis>,
      container_storage<std::vector<unsigned>>
    >();
    BOOST_CHECK(h2 == h);
    auto h3 = static_histogram<
      mpl::vector<regular_axis>,
      adaptive_storage
    >();
    BOOST_CHECK(!(h3 == h));
}

BOOST_AUTO_TEST_CASE(init_1)
{
    auto h = make_static_histogram_with<adaptive_storage>(regular_axis{3, -1, 1});
    BOOST_CHECK_EQUAL(h.dim(), 1);
    BOOST_CHECK_EQUAL(h.size(), 5);
    BOOST_CHECK_EQUAL(shape(h.axis<0>()), 5);
    auto h2 = make_static_histogram_with<container_storage<std::vector<unsigned>>>(regular_axis{3, -1, 1});
    BOOST_CHECK(h2 == h);
}

BOOST_AUTO_TEST_CASE(init_2)
{
    auto h = make_static_histogram_with<adaptive_storage>(regular_axis{3, -1, 1},
                                                         integer_axis{-1, 1});
    BOOST_CHECK_EQUAL(h.dim(), 2);
    BOOST_CHECK_EQUAL(h.size(), 25);
    BOOST_CHECK_EQUAL(shape(h.axis<0>()), 5);
    BOOST_CHECK_EQUAL(shape(h.axis<1>()), 5);
    auto h2 = make_static_histogram_with<container_storage<std::vector<unsigned>>>(regular_axis{3, -1, 1},
                                                              integer_axis{-1, 1});
    BOOST_CHECK(h2 == h);
}

BOOST_AUTO_TEST_CASE(init_3)
{
    auto h = make_static_histogram_with<adaptive_storage>(regular_axis{3, -1, 1},
                                                         integer_axis{-1, 1},
                                                         polar_axis{3});
    BOOST_CHECK_EQUAL(h.dim(), 3);
    BOOST_CHECK_EQUAL(h.size(), 75);
    auto h2 = make_static_histogram_with<container_storage<std::vector<unsigned>>>(regular_axis{3, -1, 1},
                                                              integer_axis{-1, 1},
                                                              polar_axis{3});
    BOOST_CHECK(h2 == h);
}

BOOST_AUTO_TEST_CASE(init_4)
{
    auto h = make_static_histogram_with<adaptive_storage>(regular_axis{3, -1, 1},
                                                         integer_axis{-1, 1},
                                                         polar_axis{3},
                                                         variable_axis{-1, 0, 1});
    BOOST_CHECK_EQUAL(h.dim(), 4);
    BOOST_CHECK_EQUAL(h.size(), 300);
    auto h2 = make_static_histogram_with<container_storage<std::vector<unsigned>>>(regular_axis{3, -1, 1},
                                                              integer_axis{-1, 1},
                                                              polar_axis{3},
                                                              variable_axis{-1, 0, 1});
    BOOST_CHECK(h2 == h);
}

BOOST_AUTO_TEST_CASE(init_5)
{
    auto h = make_static_histogram_with<adaptive_storage>(regular_axis{3, -1, 1},
                                                         integer_axis{-1, 1},
                                                         polar_axis{3},
                                                         variable_axis{-1, 0, 1},
                                                         category_axis{"A", "B", "C"});
    BOOST_CHECK_EQUAL(h.dim(), 5);
    BOOST_CHECK_EQUAL(h.size(), 900);
    auto h2 = make_static_histogram_with<container_storage<std::vector<unsigned>>>(regular_axis{3, -1, 1},
                                                              integer_axis{-1, 1},
                                                              polar_axis{3},
                                                              variable_axis{-1, 0, 1},
                                                              category_axis{"A", "B", "C"});
    BOOST_CHECK(h2 == h);
}

BOOST_AUTO_TEST_CASE(copy_ctor)
{
    auto h = make_static_histogram(integer_axis(0, 1),
                                   integer_axis(0, 2));
    h.fill(0, 0);
    auto h2 = decltype(h)(h);
    BOOST_CHECK(h2 == h);
    auto h3 = static_histogram<
      mpl::vector<integer_axis, integer_axis>,
      container_storage<std::vector<unsigned>>
    >(h);
    BOOST_CHECK(h3 == h);
}

BOOST_AUTO_TEST_CASE(copy_assign)
{
    auto h = make_static_histogram(integer_axis(0, 1),
                                   integer_axis(0, 2));
    h.fill(0, 0);
    auto h2 = decltype(h)();
    BOOST_CHECK(!(h == h2));
    h2 = h;
    BOOST_CHECK(h == h2);
    // test self-assign
    h2 = h2;
    BOOST_CHECK(h == h2);
    auto h3 = static_histogram<
      mpl::vector<integer_axis, integer_axis>,
      container_storage<std::vector<unsigned>>
    >();
    h3 = h;
    BOOST_CHECK(h == h3);
}

BOOST_AUTO_TEST_CASE(move)
{
    auto h = make_static_histogram(integer_axis(0, 1),
                                   integer_axis(0, 2));
    h.fill(0, 0);
    const auto href = h;
    decltype(h) h2(std::move(h));
    BOOST_CHECK_EQUAL(h.dim(), 2);
    BOOST_CHECK_EQUAL(h.sum(), 0);
    BOOST_CHECK_EQUAL(h.size(), 0);
    BOOST_CHECK(h2 == href);
    decltype(h) h3;
    h3 = std::move(h2);
    BOOST_CHECK_EQUAL(h2.dim(), 2);
    BOOST_CHECK_EQUAL(h2.sum(), 0);
    BOOST_CHECK_EQUAL(h2.size(), 0);
    BOOST_CHECK(h3 == href);
}

BOOST_AUTO_TEST_CASE(equal_compare)
{
    auto a = make_static_histogram(integer_axis(0, 1));
    auto b = make_static_histogram(integer_axis(0, 1), integer_axis(0, 2));
    BOOST_CHECK(!(a == b));
    BOOST_CHECK(!(b == a));
    auto c = make_static_histogram(integer_axis(0, 1));
    BOOST_CHECK(!(b == c));
    BOOST_CHECK(!(c == b));
    BOOST_CHECK(a == c);
    BOOST_CHECK(c == a);
    auto d = make_static_histogram(regular_axis(2, 0, 1));
    BOOST_CHECK(!(c == d));
    BOOST_CHECK(!(d == c));
    c.fill(0);
    BOOST_CHECK(!(a == c));
    BOOST_CHECK(!(c == a));
    a.fill(0);
    BOOST_CHECK(a == c);
    BOOST_CHECK(c == a);
    a.fill(0);
    BOOST_CHECK(!(a == c));
    BOOST_CHECK(!(c == a));
}

BOOST_AUTO_TEST_CASE(d1)
{
    auto h = make_static_histogram(integer_axis(0, 1));
    h.fill(0);
    h.fill(0);
    h.fill(-1);
    h.fill(10);

    BOOST_CHECK_EQUAL(h.dim(), 1);
    BOOST_CHECK_EQUAL(bins(h.axis<0>()), 2);
    BOOST_CHECK_EQUAL(shape(h.axis<0>()), 4);
    BOOST_CHECK_EQUAL(h.sum(), 4);

    BOOST_CHECK_THROW(h.value(-2), std::out_of_range);
    BOOST_CHECK_EQUAL(h.value(-1), 1.0);
    BOOST_CHECK_EQUAL(h.value(0), 2.0);
    BOOST_CHECK_EQUAL(h.value(1), 0.0);
    BOOST_CHECK_EQUAL(h.value(2), 1.0);
    BOOST_CHECK_THROW(h.value(3), std::out_of_range);

    BOOST_CHECK_THROW(h.variance(-2), std::out_of_range);
    BOOST_CHECK_EQUAL(h.variance(-1), 1.0);
    BOOST_CHECK_EQUAL(h.variance(0), 2.0);
    BOOST_CHECK_EQUAL(h.variance(1), 0.0);
    BOOST_CHECK_EQUAL(h.variance(2), 1.0);
    BOOST_CHECK_THROW(h.variance(3), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(d1_2)
{
    auto h = make_static_histogram(integer_axis(0, 1, "", false));
    h.fill(0);
    h.fill(-0);
    h.fill(-1);
    h.fill(10);

    BOOST_CHECK_EQUAL(h.dim(), 1);
    BOOST_CHECK_EQUAL(bins(h.axis<0>()), 2);
    BOOST_CHECK_EQUAL(shape(h.axis<0>()), 2);
    BOOST_CHECK_EQUAL(h.sum(), 2);

    BOOST_CHECK_THROW(h.value(-1), std::out_of_range);
    BOOST_CHECK_EQUAL(h.value(0), 2.0);
    BOOST_CHECK_EQUAL(h.value(1), 0.0);
    BOOST_CHECK_THROW(h.value(2), std::out_of_range);

    BOOST_CHECK_THROW(h.variance(-1), std::out_of_range);
    BOOST_CHECK_EQUAL(h.variance(0), 2.0);
    BOOST_CHECK_EQUAL(h.variance(1), 0.0);
    BOOST_CHECK_THROW(h.variance(2), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(d1w)
{
    auto h = make_static_histogram(regular_axis(2, -1, 1));
    h.fill(0);
    h.wfill(2.0, -1.0);
    h.fill(-1.0);
    h.fill(-2.0);
    h.wfill(5.0, 10.0);

    BOOST_CHECK_EQUAL(h.sum(), 10);

    BOOST_CHECK_EQUAL(h.value(-1), 1.0);
    BOOST_CHECK_EQUAL(h.value(0), 3.0);
    BOOST_CHECK_EQUAL(h.value(1), 1.0);
    BOOST_CHECK_EQUAL(h.value(2), 5.0);

    BOOST_CHECK_EQUAL(h.variance(-1), 1.0);
    BOOST_CHECK_EQUAL(h.variance(0), 5.0);
    BOOST_CHECK_EQUAL(h.variance(1), 1.0);
    BOOST_CHECK_EQUAL(h.variance(2), 25.0);
}

BOOST_AUTO_TEST_CASE(d2)
{
    auto h = make_static_histogram(regular_axis(2, -1, 1),
                                   integer_axis(-1, 1, nullptr, false));
    h.fill(-1, -1);
    h.fill(-1, 0);
    std::array<double, 2> ai = {{-1., -10.}};
    h.fill(ai);
    double in[2] = {-10., 0.};
    h.fill(in, in+2);

    BOOST_CHECK_EQUAL(h.dim(), 2);
    BOOST_CHECK_EQUAL(bins(h.axis<0>()), 2);
    BOOST_CHECK_EQUAL(shape(h.axis<0>()), 4);
    BOOST_CHECK_EQUAL(bins(h.axis<1>()), 3);
    BOOST_CHECK_EQUAL(shape(h.axis<1>()), 3);
    BOOST_CHECK_EQUAL(h.sum(), 3);

    BOOST_CHECK_EQUAL(h.value(-1, 0), 0.0);
    BOOST_CHECK_EQUAL(h.value(-1, 1), 1.0);
    BOOST_CHECK_EQUAL(h.value(-1, 2), 0.0);

    BOOST_CHECK_EQUAL(h.value(0, 0), 1.0);
    BOOST_CHECK_EQUAL(h.value(0, 1), 1.0);
    BOOST_CHECK_EQUAL(h.value(0, 2), 0.0);

    BOOST_CHECK_EQUAL(h.value(1, 0), 0.0);
    BOOST_CHECK_EQUAL(h.value(1, 1), 0.0);
    BOOST_CHECK_EQUAL(h.value(1, 2), 0.0);

    BOOST_CHECK_EQUAL(h.value(2, 0), 0.0);
    BOOST_CHECK_EQUAL(h.value(2, 1), 0.0);
    BOOST_CHECK_EQUAL(h.value(2, 2), 0.0);

    BOOST_CHECK_EQUAL(h.variance(-1, 0), 0.0);
    BOOST_CHECK_EQUAL(h.variance(-1, 1), 1.0);
    BOOST_CHECK_EQUAL(h.variance(-1, 2), 0.0);

    BOOST_CHECK_EQUAL(h.variance(0, 0), 1.0);
    BOOST_CHECK_EQUAL(h.variance(0, 1), 1.0);
    BOOST_CHECK_EQUAL(h.variance(0, 2), 0.0);

    BOOST_CHECK_EQUAL(h.variance(1, 0), 0.0);
    BOOST_CHECK_EQUAL(h.variance(1, 1), 0.0);
    BOOST_CHECK_EQUAL(h.variance(1, 2), 0.0);

    BOOST_CHECK_EQUAL(h.variance(2, 0), 0.0);
    BOOST_CHECK_EQUAL(h.variance(2, 1), 0.0);
    BOOST_CHECK_EQUAL(h.variance(2, 2), 0.0);
}

BOOST_AUTO_TEST_CASE(d2w)
{
    auto h = make_static_histogram(regular_axis(2, -1, 1),
                                   integer_axis(-1, 1, nullptr, false));
    h.fill(-1, 0);       // -> 0, 1
    h.wfill(10, -1, -1); // -> 0, 0
    h.wfill(5, -1, -10); // is ignored
    h.wfill(7, -10, 0);  // -> -1, 1

    BOOST_CHECK_EQUAL(h.sum(), 18);

    BOOST_CHECK_EQUAL(h.value(-1, 0), 0.0);
    BOOST_CHECK_EQUAL(h.value(-1, 1), 7.0);
    BOOST_CHECK_EQUAL(h.value(-1, 2), 0.0);

    BOOST_CHECK_EQUAL(h.value(0, 0), 10.0);
    BOOST_CHECK_EQUAL(h.value(0, 1), 1.0);
    BOOST_CHECK_EQUAL(h.value(0, 2), 0.0);

    BOOST_CHECK_EQUAL(h.value(1, 0), 0.0);
    BOOST_CHECK_EQUAL(h.value(1, 1), 0.0);
    BOOST_CHECK_EQUAL(h.value(1, 2), 0.0);

    BOOST_CHECK_EQUAL(h.value(2, 0), 0.0);
    BOOST_CHECK_EQUAL(h.value(2, 1), 0.0);
    BOOST_CHECK_EQUAL(h.value(2, 2), 0.0);

    BOOST_CHECK_EQUAL(h.variance(-1, 0), 0.0);
    BOOST_CHECK_EQUAL(h.variance(-1, 1), 49.0);
    BOOST_CHECK_EQUAL(h.variance(-1, 2), 0.0);

    BOOST_CHECK_EQUAL(h.variance(0, 0), 100.0);
    BOOST_CHECK_EQUAL(h.variance(0, 1), 1.0);
    BOOST_CHECK_EQUAL(h.variance(0, 2), 0.0);

    BOOST_CHECK_EQUAL(h.variance(1, 0), 0.0);
    BOOST_CHECK_EQUAL(h.variance(1, 1), 0.0);
    BOOST_CHECK_EQUAL(h.variance(1, 2), 0.0);

    BOOST_CHECK_EQUAL(h.variance(2, 0), 0.0);
    BOOST_CHECK_EQUAL(h.variance(2, 1), 0.0);
    BOOST_CHECK_EQUAL(h.variance(2, 2), 0.0);
}

BOOST_AUTO_TEST_CASE(d3w)
{
    auto h = make_static_histogram(integer_axis(0, 3),
                                   integer_axis(0, 4),
                                   integer_axis(0, 5));
    for (auto i = 0; i < bins(h.axis<0>()); ++i)
        for (auto j = 0; j < bins(h.axis<1>()); ++j)
            for (auto k = 0; k < bins(h.axis<2>()); ++k)
    {
        h.wfill(i+j+k, i, j, k);
    }

    for (auto i = 0; i < bins(h.axis<0>()); ++i)
        for (auto j = 0; j < bins(h.axis<1>()); ++j)
            for (auto k = 0; k < bins(h.axis<2>()); ++k)
        BOOST_CHECK_EQUAL(h.value(i, j, k), i+j+k);
}

BOOST_AUTO_TEST_CASE(add_1)
{
    auto a = make_static_histogram_with<
      adaptive_storage
    >(integer_axis(-1, 1));
    auto b = make_static_histogram_with<
      container_storage<std::vector<unsigned>>
    >(integer_axis(-1, 1));
    a.fill(-1);
    b.fill(1);
    auto c = a;
    c += b;
    BOOST_CHECK_EQUAL(c.value(-1), 0);
    BOOST_CHECK_EQUAL(c.value(0), 1);
    BOOST_CHECK_EQUAL(c.value(1), 0);
    BOOST_CHECK_EQUAL(c.value(2), 1);
    BOOST_CHECK_EQUAL(c.value(3), 0);
    auto d = a;
    d += b;
    BOOST_CHECK_EQUAL(d.value(-1), 0);
    BOOST_CHECK_EQUAL(d.value(0), 1);
    BOOST_CHECK_EQUAL(d.value(1), 0);
    BOOST_CHECK_EQUAL(d.value(2), 1);
    BOOST_CHECK_EQUAL(d.value(3), 0);
}

BOOST_AUTO_TEST_CASE(add_2)
{
    auto a = make_static_histogram_with<
      adaptive_storage
    >(integer_axis(-1, 1));
    auto b = make_static_histogram_with<
      adaptive_storage
    >(integer_axis(-1, 1));

    a.fill(0);
    b.wfill(3, -1);
    auto c = a;
    c += b;
    BOOST_CHECK_EQUAL(c.value(-1), 0);
    BOOST_CHECK_EQUAL(c.value(0), 3);
    BOOST_CHECK_EQUAL(c.value(1), 1);
    BOOST_CHECK_EQUAL(c.value(2), 0);
    BOOST_CHECK_EQUAL(c.value(3), 0);    
    auto d = a;
    d += b;
    BOOST_CHECK_EQUAL(d.value(-1), 0);
    BOOST_CHECK_EQUAL(d.value(0), 3);
    BOOST_CHECK_EQUAL(d.value(1), 1);
    BOOST_CHECK_EQUAL(d.value(2), 0);
    BOOST_CHECK_EQUAL(d.value(3), 0);
}

BOOST_AUTO_TEST_CASE(add_3)
{
    auto a = make_static_histogram_with<
      container_storage<std::vector<char>>
    >(integer_axis(-1, 1));
    auto b = make_static_histogram_with<
      container_storage<std::vector<unsigned>>
    >(integer_axis(-1, 1));
    a.fill(-1);
    b.fill(1);
    auto c = a;
    c += b;
    BOOST_CHECK_EQUAL(c.value(-1), 0);
    BOOST_CHECK_EQUAL(c.value(0), 1);
    BOOST_CHECK_EQUAL(c.value(1), 0);
    BOOST_CHECK_EQUAL(c.value(2), 1);
    BOOST_CHECK_EQUAL(c.value(3), 0);
    auto d = a;
    d += b;
    BOOST_CHECK_EQUAL(d.value(-1), 0);
    BOOST_CHECK_EQUAL(d.value(0), 1);
    BOOST_CHECK_EQUAL(d.value(1), 0);
    BOOST_CHECK_EQUAL(d.value(2), 1);
    BOOST_CHECK_EQUAL(d.value(3), 0);
}

BOOST_AUTO_TEST_CASE(bad_add)
{
    auto a = make_static_histogram(integer_axis(0, 1));
    auto b = make_static_histogram(integer_axis(0, 2));
    BOOST_CHECK_THROW(a += b, std::logic_error);
}

BOOST_AUTO_TEST_CASE(bad_index)
{
    auto a = make_static_histogram(integer_axis(0, 1));
    BOOST_CHECK_THROW(a.value(5), std::out_of_range);
    BOOST_CHECK_THROW(a.value(-5), std::out_of_range);
    BOOST_CHECK_THROW(a.variance(5), std::out_of_range);
    BOOST_CHECK_THROW(a.variance(-5), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(doc_example_0)
{
    namespace bh = boost::histogram;
    // create 1d-histogram with 10 equidistant bins from -1.0 to 2.0,
    // with axis of histogram labeled as "x"
    auto h = bh::make_static_histogram(bh::regular_axis(10, -1.0, 2.0, "x"));

    // fill histogram with data
    h.fill(-1.5); // put in underflow bin
    h.fill(-1.0); // included in first bin, bin interval is semi-open
    h.fill(-0.5);
    h.fill(1.1);
    h.fill(0.3);
    h.fill(1.7);
    h.fill(2.0);  // put in overflow bin, bin interval is semi-open
    h.fill(20.0); // put in overflow bin
    h.wfill(5.0, 0.1); // fill with a weighted entry, weight is 5.0

    std::ostringstream os1;
    // access histogram counts
    const auto& a = h.axis<0>();
    for (int i = -1, n = bins(a) + 1; i < n; ++i) {
        os1 << "bin " << i
            << " x in [" << left(a, i) << ", " << right(a ,i) << "): "
            << h.value(i) << " +/- " << std::sqrt(h.variance(i))
            << "\n";
    }

    std::ostringstream os2;
    os2 << "bin -1 x in [-inf, -1): 1 +/- 1\n"
           "bin 0 x in [-1, -0.7): 1 +/- 1\n"
           "bin 1 x in [-0.7, -0.4): 1 +/- 1\n"
           "bin 2 x in [-0.4, -0.1): 0 +/- 0\n"
           "bin 3 x in [-0.1, 0.2): 5 +/- 5\n"
           "bin 4 x in [0.2, 0.5): 1 +/- 1\n"
           "bin 5 x in [0.5, 0.8): 0 +/- 0\n"
           "bin 6 x in [0.8, 1.1): 0 +/- 0\n"
           "bin 7 x in [1.1, 1.4): 1 +/- 1\n"
           "bin 8 x in [1.4, 1.7): 0 +/- 0\n"
           "bin 9 x in [1.7, 2): 1 +/- 1\n"
           "bin 10 x in [2, inf): 2 +/- 1.41421\n";

    BOOST_CHECK_EQUAL(os1.str(), os2.str());
}

BOOST_AUTO_TEST_CASE(histogram_serialization)
{
    auto a = make_static_histogram(regular_axis(3, -1, 1, "r"),
                                   polar_axis(4, 0.0, "p"),
                                   variable_axis({0.1, 0.2, 0.3, 0.4, 0.5}, "v"),
                                   category_axis{"A", "B", "C"},
                                   integer_axis(0, 1, "i"));
    a.fill(0.5, 0.1, 0.25, 1, 0);
    std::string buf;
    {
        std::ostringstream os;
        boost::archive::text_oarchive oa(os);
        oa << a;
        buf = os.str();
    }
    auto b = decltype(a)();
    BOOST_CHECK(!(a == b));
    {
        std::istringstream is(buf);
        boost::archive::text_iarchive ia(is);
        ia >> b;
    }
    BOOST_CHECK(a == b);
}
