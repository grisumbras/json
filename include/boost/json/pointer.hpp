//
// Copyright (c) 2021 Dmitry Arkhipov (grisumbras@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_POINTER_HPP
#define BOOST_JSON_POINTER_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/value.hpp>

BOOST_JSON_NS_BEGIN

/** Path to JSON value nested element
 */
class pointer
{
    string_view str_;

public:
    /** Default constructor
    */
    pointer() noexcept
        : pointer("")
    {}

    /** String constructor
    */
    explicit pointer(string_view s) noexcept
        : str_(s)
    {}

    string_view
    string() const noexcept
    {
        return str_;
    }
};

BOOST_JSON_NS_END

#endif // BOOST_JSON_POINTER_HPP
