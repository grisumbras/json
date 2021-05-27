//
// Copyright (c) 2021 Dmitry Arkhipov (grisumbras@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

// Test that header file is self-contained.
#include <boost/json/pointer.hpp>
// Test header guards
#include <boost/json/pointer.hpp>

#include "test_suite.hpp"

BOOST_JSON_NS_BEGIN


class pointer_test
{
    value
    testValue() const
    {
        return value{
            {"foo", {"bar", "baz", "baf"}},
            {"", 0},
            {"a/b", 1},
            {"c%d", 2},
            {"e^f", 3},
            {"g|h", 4},
            {"i\\j", 5},
            {"k\"l", 6},
            {" ", 7},
            {"m~n", 8},
            {"x", object{{"y", "z"}}},
        };
    }

public:
    void
    testRootPointer()
    {
        auto const ptr = pointer();
        BOOST_TEST(ptr.string().empty());
        BOOST_TEST(ptr.string() == pointer("").string());

        value const jv = testValue();
        BOOST_TEST(&jv.at(ptr) == &jv);
    }

    void
    testChildPointer()
    {
        value const jv = testValue();
        BOOST_TEST(&jv.at(pointer("/foo"))== &jv.at("foo"));
        BOOST_TEST(&jv.at(pointer("/c%d"))== &jv.at("c%d"));
        BOOST_TEST(&jv.at(pointer("/e^f"))== &jv.at("e^f"));
        BOOST_TEST(&jv.at(pointer("/g|h"))== &jv.at("g|h"));
        BOOST_TEST(&jv.at(pointer("/"))== &jv.at(""));
        BOOST_TEST(&jv.at(pointer("/i\\j"))== &jv.at("i\\j"));
        BOOST_TEST(&jv.at(pointer("/k\"l"))== &jv.at("k\"l"));
        BOOST_TEST(&jv.at(pointer("/ "))== &jv.at(" "));
    }

    void
    testEscaped()
    {
        value const jv = testValue();
        BOOST_TEST(&jv.at(pointer("/a~1b"))== &jv.at("a/b"));
        BOOST_TEST(&jv.at(pointer("/m~0n"))== &jv.at("m~n"));
    }

    void
    testNested()
    {
        value const jv = testValue();
        BOOST_TEST(&jv.at(pointer("/foo/0")) == &jv.at("foo").at(0));
        BOOST_TEST(&jv.at(pointer("/foo/1")) == &jv.at("foo").at(1));
        BOOST_TEST(&jv.at(pointer("/x/y")) == &jv.at("x").at("y"));

        {
            value v1;
            object& o1 = v1.emplace_object();
            object& o2 = (o1["very"] = value()).emplace_object();
            object& o3 = (o2["deep"] = value()).emplace_object();

            array& a1 = (o3["path"] = value()).emplace_array();
            a1.emplace_back(value());

            array& a2 = a1.emplace_back(value()).emplace_array();
            a2.emplace_back(value());
            a2.emplace_back(value());

            array& a3 = a2.emplace_back(value()).emplace_array();
            object& o4 = a3.emplace_back(value()).emplace_object();
            object& o5 = (o4["0"] = value()).emplace_object();
            value& v2 = o5["fin"] = value();

            BOOST_TEST(&v1.at(pointer("/very/deep/path/1/2/0/0/fin")) == &v2);
        }
    }

    void
    testErrors()
    {
        value const jv = testValue();
        BOOST_TEST_THROWS(jv.at(pointer("foo")), system_error);
        BOOST_TEST_THROWS(jv.at(pointer("/fo")), system_error);
        BOOST_TEST_THROWS(jv.at(pointer("/m~")), system_error);
        BOOST_TEST_THROWS(jv.at(pointer("/m~n")), system_error);
        BOOST_TEST_THROWS(jv.at(pointer("/foo/bar")), system_error);
        BOOST_TEST_THROWS(jv.at(pointer("/foo/")), system_error);
        BOOST_TEST_THROWS(jv.at(pointer("/foo/01")), system_error);
        BOOST_TEST_THROWS(jv.at(pointer("/foo/2b")), system_error);
        BOOST_TEST_THROWS(jv.at(pointer("/x/y/z")), system_error);
    }

    void
    testNonThrowing()
    {
        value jv = testValue();
        error_code ec = error::syntax;
        BOOST_TEST(jv.find(pointer("/foo/0"), ec) == &jv.at("foo").at(0));
        BOOST_TEST(!ec);

        auto const& cjv = jv;
        ec = error::syntax;
        BOOST_TEST(cjv.find(pointer("/foo/1"), ec) == &jv.at("foo").at(1));
        BOOST_TEST(!ec);

        jv.find(pointer("foo"), ec);
        BOOST_TEST(ec == error::missing_slash);

        jv.find(pointer("/fo"), ec);
        BOOST_TEST(ec == error::not_found);

        jv.find(pointer("/foo/25"), ec);
        BOOST_TEST(ec == error::not_found);

        value(object()).find(pointer("/foo"), ec);
        BOOST_TEST(ec == error::not_found);

        jv.find(pointer("/m~"), ec);
        BOOST_TEST(ec == error::invalid_escape);

        jv.find(pointer("/m~n"), ec);
        BOOST_TEST(ec == error::invalid_escape);

        jv.find(pointer("/foo/bar"), ec);
        BOOST_TEST(ec == error::token_not_number);

        jv.find(pointer("/foo/"), ec);
        BOOST_TEST(ec == error::token_not_number);

        jv.find(pointer("/foo/01"), ec);
        BOOST_TEST(ec == error::token_not_number);

        jv.find(pointer("/foo/2b"), ec);
        BOOST_TEST(ec == error::token_not_number);

        jv.find(pointer("/foo/2."), ec);
        BOOST_TEST(ec == error::token_not_number);

        jv.find(pointer("/foo/-"), ec);
        BOOST_TEST(ec == error::past_the_end);

        jv.find(pointer("/foo/-/x"), ec);
        BOOST_TEST(ec == error::past_the_end);

        jv.find(pointer("/foo/-1"), ec);
        BOOST_TEST(ec == error::token_not_number);

        jv.find(pointer("/x/y/z"), ec);
        BOOST_TEST(ec == error::value_is_scalar);

        string s = "/foo/";
        s += std::to_string((std::numeric_limits<std::size_t>::max)());
        if ( '9' == s[s.size() - 1] )
        {
            for (std::size_t i = 6; i < s.size(); ++i)
            {
                s[i] = '0';
            }
            s[5] = '1';
            s += "0";
        }
        else
        {
            ++s[s.size() - 1];
        }
        jv.find(pointer(s), ec);
        BOOST_TEST(ec == error::token_overflow);
    }

    void
    run()
    {
        testRootPointer();
        testChildPointer();
        testEscaped();
        testNested();
        testErrors();
        testNonThrowing();
    }
};

TEST_SUITE(pointer_test, "boost.json.pointer");

BOOST_JSON_NS_END
