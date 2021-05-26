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


class pointer_token
{
public:
    class iterator;

    pointer_token(
        string_view::const_iterator b,
        string_view::const_iterator e) noexcept
        : b_(b), e_(e)
    {
    }

    iterator begin() const noexcept;
    iterator end() const noexcept;

private:
    string_view::const_iterator b_;
    string_view::const_iterator e_;
};

class pointer_token::iterator
{
public:
    explicit iterator(string_view::const_iterator base) noexcept
        : base_(base)
    {
    }

    char operator*() const noexcept
    {
        switch (char c = *base_)
        {
        case '~':
            c = base_[1];
            if ( '0' == c )
            {
                return '~';
            }
            else
            {
                BOOST_ASSERT('1' == c);
                return '/';
            }
        default:
            return c;
        }
    }

    iterator& operator++() noexcept
    {
        if ( '~' == *base_ )
        {
            base_ += 2;
        }
        else
        {
            ++base_;
        }
        return *this;
    }

    string_view::const_iterator base() const noexcept
    {
        return base_;
    }

private:
    string_view::const_iterator base_;
};

bool operator==(pointer_token::iterator l, pointer_token::iterator r) noexcept
{
    return l.base() == r.base();
}

bool operator!=(pointer_token::iterator l, pointer_token::iterator r) noexcept
{
    return l.base() != r.base();
}

pointer_token::iterator pointer_token::begin() const noexcept
{
    return iterator(b_);
}

pointer_token::iterator pointer_token::end() const noexcept
{
    return iterator(e_);
}

bool operator==(pointer_token token, string_view sv) noexcept
{
    auto t_b = token.begin();
    auto const t_e = token.end();
    auto s_b = sv.begin();
    auto const s_e = sv.end();
    for (; t_b != t_e && s_b != s_e; ++t_b, ++s_b)
    {
        if (*t_b != *s_b)
        {
            return false;
        }
    }

    return t_e == t_b && s_e == s_b;
}

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

pointer_token
get_token(
    string_view::const_iterator b,
    string_view::const_iterator e,
    error_code& ec) noexcept
{
    auto const start = b;
    for (; b < e; ++b)
    {
        char const c = *b;
        if ( '/' == c )
        {
            break;
        }

        if ( '~' == c )
        {
            if ( ++b == e )
            {
                ec = error::invalid_escape;
                break;
            }

            switch (*b)
            {
            case '0': // fall through
            case '1':
                // valid escape sequence
                continue;
            default:
                ec = error::invalid_escape;
                break;
            }
            break;
        }
    }

    return pointer_token(start, b);
}

value*
if_contains_token(object const& obj, pointer_token token)
{
    if(obj.empty())
    {
        return nullptr;
    }

    auto const it = detail::find_in_object(obj, token).first;
    if(!it)
    {
        return nullptr;
    }

    return &it->value();
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
            auto const token = get_token(cur, ptr_str.end(), ec);
            if ( ec )
            {
                return nullptr;
            }

            result = detail::if_contains_token(*object, token);
            cur = token.end().base();
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

    BOOST_ASSERT(result);
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
