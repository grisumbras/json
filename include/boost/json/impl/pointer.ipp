//
// Copyright (c) 2021 Dmitry Arkhipov (grisumbras@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_IMPL_POINTER_IPP
#define BOOST_JSON_IMPL_POINTER_IPP

#include <boost/json/pointer.hpp>

BOOST_JSON_NS_BEGIN

namespace detail {

std::size_t
parse_number_token(
    string_view::const_iterator& b,
    string_view::const_iterator e,
    error_code& ec) noexcept
{
    if ( ( b == e ) || ( *b == '0' && ( b + 1 ) != e  && b[1] != '/' ) )
    {
        ec = error::token_not_number;
        return {};
    }

    std::size_t result = 0;
    for (; b != e; ++b)
    {
        char const c = *b;

        if ( '/' == c)
        {
            break;
        }

        if ( c < '0' || c > '9' )
        {
            ec = error::token_not_number;
            return {};
        }

        constexpr std::size_t max_size =
            (std::numeric_limits<std::size_t>::max)();
        constexpr std::size_t max_div10 = max_size / 10;

        if ( result <= max_div10 )
        {
            result *= 10;
            std::size_t d = c - '0';
            if ( result <= max_size - d )
            {
                result += d;
                continue;
            }
        }

        ec = error::token_overflow;
        return {};
    }
    return result;
}


string_view
parse_string_token(
    string_view::const_iterator& b,
    string_view::const_iterator e,
    char* out,
    char const* out_end,
    error_code& ec) noexcept
{
    char* const start = out;

    for (; b < e; ++b)
    {
        char const c = *b;
        if ( '/' == c )
        {
            break;
        }

        if ( out == out_end )
        {
            ec = error::token_too_large;
            goto parse_done;
        }

        if ( '~' == c )
        {
            if ( ++b == e )
            {
                ec = error::invalid_escape;
                goto parse_done;
            }

            switch (*b)
            {
            case '0':
                *out++ = '~';
                break;
            case '1':
                *out++ = '/';
                break;
            default:
                ec = error::invalid_escape;
                goto parse_done;
            }
        }
        else
        {
            *out++ = c;
        }
    }

parse_done:
    return string_view(start, out - start);
}

template <class Value>
Value*
find_pointer(Value& root, pointer ptr, error_code& ec) noexcept
{
    ec.clear();
    string_view const ptr_str = ptr.string();
    Value* result = &root;
    auto cur = ptr_str.begin();
    while ( ptr_str.end() != cur )
    {
        if ( *cur++ != '/' )
        {
            ec = error::missing_slash;
            return nullptr;
        }

        if ( auto object = result->if_object() )
        {
            char buffer[256];
            string_view token = detail::parse_string_token(cur, ptr_str.end(),
                buffer, buffer + sizeof(buffer), ec);
            if ( ec )
            {
                return nullptr;
            }

            result = object->if_contains(token);
        }
        else if ( auto array = result->if_array() )
        {
            std::size_t index = detail::parse_number_token(cur, ptr_str.end(),
                    ec);
            if ( ec )
            {
                return nullptr;
            }

            result = array->if_contains(index);
        }
        else
        {
            ec = error::value_is_scalar;
            return nullptr;
        }

        if ( !result )
        {
            ec = error::not_found;
            return nullptr;
        }
    }
    return result;
}

template <class Value>
Value&
at_pointer(Value& root, pointer ptr)
{
    error_code ec;
    auto const found = find_pointer(root, ptr, ec);
    if ( !found )
    {
        detail::throw_system_error(ec, BOOST_JSON_SOURCE_POS);
    }
    return *found;
}

} // namespace detail

value const&
value::at(pointer ptr) const
{
    return detail::at_pointer(*this, ptr);
}

value&
value::at(pointer ptr)
{
    return detail::at_pointer(*this, ptr);
}

value const*
value::find(pointer ptr, error_code& ec) const noexcept
{
    return detail::find_pointer(*this, ptr, ec);
}

value*
value::find(pointer ptr, error_code& ec) noexcept
{
    return detail::find_pointer(*this, ptr, ec);
}

BOOST_JSON_NS_END

#endif // BOOST_JSON_IMPL_POINTER_IPP
