//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

// Test that header file is self-contained.
#include <boost/json/parse_into.hpp>

#ifndef BOOST_JSON_NO_PARSE_INTO

#include <boost/json/serialize.hpp>
#include <boost/json/value_from.hpp>
#include <boost/describe.hpp>

#include <climits>
#include <map>

#include "test.hpp"
#include "test_suite.hpp"

struct X
{
    int a;
    float b;
    std::string c;
};

BOOST_DESCRIBE_STRUCT(X, (), (a, b, c))

bool operator==( X const& x1, X const& x2 )
{
    return x1.a == x2.a && x1.b == x2.b && x1.c == x2.c;
}

struct Y
{
    std::vector<X> v;
    std::map<std::string, X> m;
};

BOOST_DESCRIBE_STRUCT(Y, (), (v, m))

bool operator==( Y const& y1, Y const& y2 )
{
    return y1.v == y2.v && y1.m == y2.m;
}

namespace boost {
namespace json {

class parse_into_test
{
public:

    template<class T> void testParseInto( T const& t )
    {
        T t1( t );

        std::string json = serialize( value_from( t1 ) );

        T t2{};

        error_code ec;
        parse_into( t2, json, ec );

        BOOST_TEST( !ec.failed() ) && BOOST_TEST( t1 == t2 );
    }

    void testNull()
    {
        testParseInto( nullptr );
    }

    void testBoolean()
    {
        testParseInto( false );
        testParseInto( true );
    }

    void testIntegral()
    {
        testParseInto<char>( 'A' ); // ?
        testParseInto<signed char>( -127 );
        testParseInto<unsigned char>( 255 );
        testParseInto<short>( -32767 );
        testParseInto<unsigned short>( 65535 );
        testParseInto<int>( -32767 );
        testParseInto<unsigned int>( 65535 );
        testParseInto<long>( LONG_MIN );
        testParseInto<unsigned long>( ULONG_MAX );
        testParseInto<long long>( LLONG_MIN );
        testParseInto<unsigned long long>( ULLONG_MAX );
    }

    void testFloatingPoint()
    {
        testParseInto( 0.25f );
        testParseInto( 1.125 );

        // value_from doesn't support long double
        // testParseInto( 2.25L );
    }

    void testString()
    {
        testParseInto<std::string>( "" );
        testParseInto<std::string>( "12345" );
    }

    void testSequence()
    {
        testParseInto<std::vector<std::nullptr_t>>( { nullptr, nullptr } );

        testParseInto< std::vector<bool> >( {} );
        testParseInto< std::vector<bool> >( { true, false } );

        testParseInto< std::vector<int> >( {} );
        testParseInto< std::vector<int> >( { 1, 2, 3 } );

        testParseInto< std::vector<float> >( {} );
        testParseInto< std::vector<float> >( { 1.02f, 2.11f, 3.14f } );

        testParseInto< std::vector<std::string> >( {} );
        testParseInto< std::vector<std::string> >( { "one", "two", "three" } );

        testParseInto< std::vector<std::vector<int>> >( {} );
        testParseInto< std::vector<std::vector<int>> >( { {}, { 1 }, { 2, 3 }, { 4, 5, 6 } } );
    }

    void testMap()
    {
        testParseInto< std::map<std::string, int> >( {} );
        testParseInto< std::map<std::string, int> >( { { "one", 1 }, { "two", 2 } } );

        testParseInto< std::map<std::string, int> >( {} );
        testParseInto< std::map<std::string, int> >( { { "one", 1 }, { "two", 2 } } );

        testParseInto< std::map<std::string, std::vector<int>> >( {} );
        testParseInto< std::map<std::string, std::vector<int>> >( { { "one", { 1 } }, { "two", { 2, 3 } } } );

        testParseInto< std::map<std::string, std::map<std::string, int>> >( {} );
        testParseInto< std::map<std::string, std::map<std::string, int>> >( { { "one", {} }, { "two", { { "1", 1 }, { "2", 2 } } } } );
    }

    void testTuple()
    {
        testParseInto<std::pair<int, float>>( {} );
        testParseInto<std::pair<int, float>>( { 1, 3.14f } );

        testParseInto<std::tuple<int, float, std::string>>( {} );
        testParseInto<std::tuple<int, float, std::string>>( std::tuple<int, float, std::string>( 1, 3.14f, "hello" ) );

        testParseInto<std::pair<std::vector<int>, std::map<std::string, std::pair<int, bool>>>>( {} );
        testParseInto<std::pair<std::vector<int>, std::map<std::string, std::pair<int, bool>>>>( { { 1, 2, 3 }, { { "one", { 7, true } } } } );
    }

    void testStruct()
    {
        testParseInto<X>( {} );
        testParseInto<X>( { 1, 3.14f, "hello" } );

        testParseInto<std::vector<std::pair<int, int>>>( {} );
        testParseInto<std::vector<std::pair<int, int>>>( { { 1, 2 }, { 3, 4 } } );

        testParseInto<std::vector<std::vector<std::pair<int, int>>>>( {} );
        testParseInto<std::vector<std::vector<std::pair<int, int>>>>( { { { 1, 2 }, { 3, 4 } } } );
        testParseInto<std::vector<std::vector<std::pair<int, int>>>>( { { { 1, 2 }, { 3, 4 } }, { { 5, 6 }, { 7, 8 } } } );

        testParseInto<std::map<std::string, std::vector<std::pair<int, int>>>>( {} );
        testParseInto<std::map<std::string, std::vector<std::pair<int, int>>>>( { { "one", {} } } );
        testParseInto<std::map<std::string, std::vector<std::pair<int, int>>>>( { { "one", { { 1, 2 }, { 3, 4 } } } } );

        testParseInto<Y>( {} );
        testParseInto<Y>( { { { 1, 1.0f, "one" }, { 2, 2.0f, "two" } }, { { "one", { 1, 1.1f, "1" } }, { "two", { 2, 2.2f, "2" } } } } );
    }

    void run()
    {
        testNull();
        testBoolean();
        testIntegral();
        testFloatingPoint();
        testString();
        testSequence();
        testMap();
        testTuple();
        testStruct();
    }
};

TEST_SUITE(parse_into_test, "boost.json.parse_into");

} // namespace boost
} // namespace json

#endif // BOOST_JSON_NO_PARSE_INTO
