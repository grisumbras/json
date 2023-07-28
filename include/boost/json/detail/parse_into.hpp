//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2020 Krystian Stasiowski (sdkrystian@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_PARSE_INTO_HPP
#define BOOST_JSON_DETAIL_PARSE_INTO_HPP

#include <boost/json/detail/config.hpp>

#ifndef BOOST_JSON_NO_PARSE_INTO

#include <boost/json/error.hpp>
#include <boost/json/conversion.hpp>
#include <boost/describe/enum_from_string.hpp>

namespace boost {
namespace json {
namespace detail {

template< class Impl, class T, class Parent >
class converting_handler;

// get_handler
template< class V, class P >
using get_handler = converting_handler<
    select_conversion_implementation<basic_conversion_tag, V>, V, P>;

template<error E> class handler_error_base
{
public:

    handler_error_base() = default;

    handler_error_base( handler_error_base const& ) = delete;
    handler_error_base& operator=( handler_error_base const& ) = delete;

public:

    bool on_object_begin( error_code& ec ) { BOOST_JSON_FAIL( ec, E ); return false; }
    bool on_object_end( std::size_t, error_code& ec ) { BOOST_JSON_FAIL( ec, E ); return false; }
    bool on_array_begin( error_code& ec ) { BOOST_JSON_FAIL( ec, E ); return false; }
    bool on_array_end( std::size_t, error_code& ec ) { BOOST_JSON_FAIL( ec, E ); return false; }
    bool on_key_part( string_view, std::size_t, error_code& ec ) { BOOST_JSON_FAIL( ec, E ); return false; }
    bool on_key( string_view, std::size_t, error_code& ec ) { BOOST_JSON_FAIL( ec, E ); return false; }
    bool on_string_part( string_view, std::size_t, error_code& ec ) { BOOST_JSON_FAIL( ec, E ); return false; }
    bool on_string( string_view, std::size_t, error_code& ec ) { BOOST_JSON_FAIL( ec, E ); return false; }
    bool on_number_part( string_view, error_code& ec ) { BOOST_JSON_FAIL( ec, E ); return false; }
    bool on_int64( std::int64_t, string_view, error_code& ec ) { BOOST_JSON_FAIL( ec, E ); return false; }
    bool on_uint64( std::uint64_t, string_view, error_code& ec ) { BOOST_JSON_FAIL( ec, E ); return false; }
    bool on_double( double, string_view, error_code& ec ) { BOOST_JSON_FAIL( ec, E ); return false; }
    bool on_bool( bool, error_code& ec ) { BOOST_JSON_FAIL( ec, E ); return false; }
    bool on_null( error_code& ec ) { BOOST_JSON_FAIL( ec, E ); return false; }
};

template< class P, error E >
class scalar_handler
    : public handler_error_base<E>
{
protected:
    P* parent_;

public:
    scalar_handler(P* p): parent_( p )
    {}

    bool on_array_end( std::size_t, error_code& )
    {
        parent_->signal_end();
        return true;
    }
};

template< class D, class V, class P, error E >
class composite_handler
{
protected:
    using inner_handler_type = get_handler<V, D>;

    P* parent_;
    V next_value_ = {};
    inner_handler_type inner_;
    bool inner_active_ = false;

public:
    composite_handler( composite_handler const& ) = delete;
    composite_handler& operator=( composite_handler const& ) = delete;

    composite_handler( P* p )
        : parent_(p), inner_( &next_value_, static_cast<D*>(this) )
    {}

    void signal_end()
    {
        inner_active_ = false;
        parent_->signal_value();
    }

#define BOOST_JSON_INVOKE_INNER(f) \
    if( !inner_active_ ) { \
        BOOST_JSON_FAIL(ec, E); \
        return false; \
    } \
    else \
        return inner_.f

    bool on_object_begin( error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_object_begin(ec) );
    }

    bool on_object_end( std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_object_end(n, ec) );
    }

    bool on_array_begin( error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_array_begin(ec) );
    }

    bool on_array_end( std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_array_end(n, ec) );
    }

    bool on_key_part( string_view sv, std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_key_part(sv, n, ec) );
    }

    bool on_key( string_view sv, std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_key(sv, n, ec) );
    }

    bool on_string_part( string_view sv, std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_string_part(sv, n, ec) );
    }

    bool on_string( string_view sv, std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_string(sv, n, ec) );
    }

    bool on_number_part( string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_number_part(sv, ec) );
    }

    bool on_int64( std::int64_t v, string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_int64(v, sv, ec) );
    }

    bool on_uint64( std::uint64_t v, string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_uint64(v, sv, ec) );
    }

    bool on_double( double v, string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_double(v, sv, ec) );
    }

    bool on_bool( bool v, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_bool(v, ec) );
    }

    bool on_null( error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_null(ec) );
    }

#undef BOOST_JSON_INVOKE_INNER
};

// integral handler
template<class V,
typename std::enable_if<std::is_signed<V>::value, int>::type = 0>
bool integral_in_range( std::int64_t v )
{
    return v >= (std::numeric_limits<V>::min)() && v <= (std::numeric_limits<V>::max)();
}

template<class V,
typename std::enable_if<!std::is_signed<V>::value, int>::type = 0>
bool integral_in_range( std::int64_t v )
{
    return v >= 0 && static_cast<std::uint64_t>( v ) <= (std::numeric_limits<V>::max)();
}

template<class V>
bool integral_in_range( std::uint64_t v )
{
    return v <= static_cast<typename std::make_unsigned<V>::type>( (std::numeric_limits<V>::max)() );
}

template< class V, class P >
class converting_handler<integral_conversion_tag, V, P>
    : public scalar_handler<P, error::not_integer>
{
private:
    V* value_;

public:
    converting_handler( V* v, P* p )
        : converting_handler::scalar_handler(p)
        , value_(v)
    {}

    bool on_number_part( string_view, error_code& )
    {
        return true;
    }

    bool on_int64( std::int64_t v, string_view, error_code& ec )
    {
        if( !integral_in_range<V>( v ) )
        {
            BOOST_JSON_FAIL( ec, error::not_exact );
            return false;
        }

        *value_ = static_cast<V>( v );

        this->parent_->signal_value();
        return true;
    }

    bool on_uint64( std::uint64_t v, string_view, error_code& ec )
    {
        if( !integral_in_range<V>( v ) )
        {
            BOOST_JSON_FAIL( ec, error::not_exact );
            return false;
        }

        *value_ = static_cast<V>( v );

        this->parent_->signal_value();
        return true;
    }
};

// floating point handler
template< class V, class P>
class converting_handler<floating_point_conversion_tag, V, P>
    : public scalar_handler<P, error::not_double>
{
private:
    V* value_;

public:
    converting_handler( V* v, P* p )
        : converting_handler::scalar_handler(p)
        , value_(v)
    {}

    bool on_number_part( string_view, error_code& )
    {
        return true;
    }

    bool on_int64( std::int64_t v, string_view, error_code& )
    {
        *value_ = static_cast<V>( v );

        this->parent_->signal_value();
        return true;
    }

    bool on_uint64( std::uint64_t v, string_view, error_code& )
    {
        *value_ = static_cast<V>( v );

        this->parent_->signal_value();
        return true;
    }

    bool on_double( double v, string_view, error_code& )
    {
        *value_ = static_cast<V>( v );

        this->parent_->signal_value();
        return true;
    }
};

// string handler
template< class V, class P >
class converting_handler<string_like_conversion_tag, V, P>
    : public scalar_handler<P, error::not_string>
{
private:
    V* value_;

public:
    converting_handler( V* v, P* p )
        : converting_handler::scalar_handler(p)
        , value_(v)
    {}

    bool on_string_part( string_view sv, std::size_t, error_code& )
    {
        value_->append( sv.begin(), sv.end() );
        return true;
    }

    bool on_string( string_view sv, std::size_t, error_code& )
    {
        value_->append( sv.begin(), sv.end() );

        this->parent_->signal_value();
        return true;
    }
};

// bool handler
template< class V, class P >
class converting_handler<bool_conversion_tag, V, P>
    : public scalar_handler<P, error::not_bool>
{
private:
    V* value_;

public:
    converting_handler( V* v, P* p )
        : converting_handler::scalar_handler(p)
        , value_(v)
    {}

    bool on_bool( bool v, error_code& )
    {
        *value_ = v;

        this->parent_->signal_value();
        return true;
    }
};

// null handler
template< class V, class P >
class converting_handler<null_like_conversion_tag, V, P>
    : public scalar_handler<P, error::not_null>
{
private:
    V* value_;

public:
    converting_handler( V* v, P* p )
        : converting_handler::scalar_handler(p)
        , value_(v)
    {}

    bool on_null( error_code& )
    {
        *value_ = {};

        this->parent_->signal_value();
        return true;
    }
};

// described enum handler
template< class V, class P >
class converting_handler<described_enum_conversion_tag, V, P>
    : public scalar_handler<P, error::not_string>
{
private:
    V* value_;
    std::string name_;

public:
    converting_handler( V* v, P* p )
        : converting_handler::scalar_handler(p)
        , value_(v)
    {}

    bool on_string_part( string_view sv, std::size_t, error_code& )
    {
        name_.append( sv.begin(), sv.end() );
        return true;
    }

    bool on_string( string_view sv, std::size_t, error_code& ec )
    {
        name_.append( sv.begin(), sv.end() );

        if( !describe::enum_from_string(name_.data(), *value_) )
        {
            BOOST_JSON_FAIL(ec, error::unknown_name);
            return false;
        }

        this->parent_->signal_value();
        return true;
    }
};

template< class V, class P >
struct converting_handler<no_conversion_tag, V, P>
{
    static_assert( sizeof(V) == 0, "This type is not supported" );
};

// sequence handler
template< class V, class P >
class converting_handler<sequence_conversion_tag, V, P>
    : public composite_handler<
        converting_handler<sequence_conversion_tag, V, P>,
        detail::value_type<V>,
        P,
        error::not_array>
{
private:
    V* value_;

public:
    converting_handler( V* v, P* p )
        : converting_handler::composite_handler(p), value_(v)
    {}

    void signal_value()
    {
        value_->push_back( std::move(this->next_value_) );
        this->next_value_ = {};
    }

    bool on_array_begin( error_code& ec )
    {
        if( this->inner_active_ )
        {
            return this->inner_.on_array_begin( ec );
        }
        else
        {
            this->inner_active_ = true;
            return true;
        }
    }

    bool on_array_end( std::size_t n, error_code& ec )
    {
        if( this->inner_active_ )
        {
            return this->inner_.on_array_end( n, ec );
        }
        else
        {
            this->parent_->signal_end();
            return true;
        }
    }
};

// map handler
template< class V, class P >
class converting_handler<map_like_conversion_tag, V, P>
    : public composite_handler<
        converting_handler<map_like_conversion_tag, V, P>,
        detail::mapped_type<V>,
        P,
        error::not_array>
{
private:
    V* value_;
    std::string key_;

public:
    converting_handler( V* v, P* p )
        : converting_handler::composite_handler(p), value_(v)
    {}

    void signal_value()
    {
        value_->emplace( std::move(key_), std::move(this->next_value_) );

        key_ = {};
        this->next_value_ = {};

        this->inner_active_ = false;
    }

    bool on_object_begin( error_code& ec )
    {
        if( this->inner_active_ )
            return this->inner_.on_object_begin(ec);

        return true;
    }

    bool on_object_end( std::size_t n, error_code& ec )
    {
        if( this->inner_active_ )
            return this->inner_.on_object_end(n, ec);

        this->parent_->signal_value();
        return true;
    }

    bool on_array_end( std::size_t n, error_code& ec )
    {
        if( this->inner_active_ )
            return this->inner_.on_array_end(n, ec);

        this->parent_->signal_end();
        return true;
    }

    bool on_key_part( string_view sv, std::size_t n, error_code& ec )
    {
        if( this->inner_active_ )
            return this->inner_.on_key_part(sv, n, ec);

        key_.append( sv.data(), sv.size() );
        return true;
    }

    bool on_key( string_view sv, std::size_t n, error_code& ec )
    {
        if( this->inner_active_ )
            return this->inner_.on_key(sv, n, ec);

        key_.append( sv.data(), sv.size() );

        this->inner_active_ = true;
        return true;
    }
};

// tuple handler
template<std::size_t I, class T> struct handler_tuple_element
{
    T t_;
};

template<class S, class... T> struct handler_tuple_impl;

template<std::size_t... I, class... T>
struct handler_tuple_impl<mp11::index_sequence<I...>, T...>
    : handler_tuple_element<I, T>...
{
    template<class... A>
    handler_tuple_impl( A... a )
        : handler_tuple_element<I, T>{{ a.first, a.second }}...
    { }
};

template<class P, class... V>
struct handler_tuple
    : public handler_tuple_impl<
        mp11::index_sequence_for<V...>, get_handler<V, P>...>
{
    using base_type = handler_tuple_impl<
        mp11::index_sequence_for<V...>, get_handler<V, P>...>;

    template<class... A>
    handler_tuple( A... a )
        : base_type( a... )
    { }

    handler_tuple( handler_tuple const& ) = delete;
    handler_tuple& operator=( handler_tuple const& ) = delete;
};

template<std::size_t I, class T>
T&
get( handler_tuple_element<I, T>& e )
{
    return e.t_;
}

template< class P, class T >
struct tuple_inner_handlers;

template< class P, template<class...> class L, class... V >
struct tuple_inner_handlers<P, L<V...>>
{
    handler_tuple<P, V...> handlers_;

    template<std::size_t... I>
    tuple_inner_handlers(
        L<V...>* pv, P* pp, mp11::index_sequence<I...> )
        : handlers_( std::make_pair( &get<I>(*pv), pp )... )
    {}
};

template< class T, class P >
class converting_handler<tuple_conversion_tag, T, P>
{
private:
    T* value_;
    P* parent_;

    std::string key_;

    tuple_inner_handlers<converting_handler, T> inner_;
    int inner_active_ = -1;

public:
    converting_handler( converting_handler const& ) = delete;
    converting_handler& operator=( converting_handler const& ) = delete;

    converting_handler( T* v, P* p )
        : value_( v )
        , parent_( p )
        , inner_(
            v,
            this,
            mp11::make_index_sequence< std::tuple_size<T>::value >())
    {}

    void signal_value()
    {
        ++inner_active_;
    }

    void signal_end()
    {
        inner_active_ = -1;
        parent_->signal_value();
    }

#define BOOST_JSON_INVOKE_INNER(fn) \
    if( inner_active_ < 0 ) \
    { \
        BOOST_JSON_FAIL( ec, error::not_array ); \
        return false; \
    } \
    constexpr int N = std::tuple_size<T>::value; \
    if( inner_active_ >= N ) \
    { \
        BOOST_JSON_FAIL( ec, error::size_mismatch ); \
        return false; \
    } \
    return mp11::mp_with_index<N>( inner_active_, [&](auto I){ \
        return get<I>( inner_.handlers_ ).fn; \
    });

    bool on_object_begin( error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_object_begin(ec) );
    }

    bool on_object_end( std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_object_end(n, ec) );
    }

    bool on_array_begin( error_code& ec )
    {
        if( inner_active_ < 0 )
        {
            inner_active_ = 0;
            return true;
        }

        constexpr int N = std::tuple_size<T>::value;

        if( inner_active_ >= N )
        {
            inner_active_ = 0;
            return true;
        }

        return mp11::mp_with_index<N>( inner_active_, [&](auto I){
            return get<I>( inner_.handlers_ ).on_array_begin(ec);
        });
    }

    bool on_array_end( std::size_t n, error_code& ec )
    {
        if( inner_active_ < 0 )
        {
            parent_->signal_end();
            return true;
        }

        constexpr int N = std::tuple_size<T>::value;

        if( inner_active_ >= N )
        {
            parent_->signal_value();

            inner_active_ = -1;
            return true;
        }

        return mp11::mp_with_index<N>( inner_active_, [&](auto I){
            return get<I>( inner_.handlers_ ).on_array_end( n, ec );
        });
    }

    bool on_key_part( string_view sv, std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_key_part(sv, n, ec) );
    }

    bool on_key( string_view sv, std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_key(sv, n, ec) );
    }

    bool on_string_part( string_view sv, std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_string_part(sv, n, ec) );
    }

    bool on_string( string_view sv, std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_string(sv, n, ec) );
    }

    bool on_number_part( string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_number_part(sv, ec) );
    }

    bool on_int64( std::int64_t v, string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_int64(v, sv, ec) );
    }

    bool on_uint64( std::uint64_t v, string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_uint64(v, sv, ec) );
    }

    bool on_double( double v, string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_double(v, sv, ec) );
    }

    bool on_bool( bool v, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_bool(v, ec) );
    }

    bool on_null( error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_null(ec) );
    }

#undef BOOST_JSON_INVOKE_INNER
};

// described struct handler
template<class T, class D>
using struct_member_type = std::remove_reference_t< decltype( std::declval<T&>().*D::pointer ) >;

template<class P, class T, class L>
struct struct_inner_handlers;

template<class P, class T, template<class...> class L, class... D>
struct struct_inner_handlers<P, T, L<D...>>
{
    handler_tuple<P, struct_member_type<T, D>...> handlers_;

    struct_inner_handlers( T* pv, P* pp )
        : handlers_( std::make_pair( &(pv->*D::pointer), pp )... )
    {}
};

template<class V, class P>
class converting_handler<described_class_conversion_tag, V, P>
{
private:
    V* value_;
    P* parent_;

    std::string key_;

    using Dm = describe::describe_members<V, describe::mod_public>;

    struct_inner_handlers<converting_handler, V, Dm> inner_;
    int inner_active_ = -1;

public:
    converting_handler( converting_handler const& ) = delete;
    converting_handler& operator=( converting_handler const& ) = delete;

public:
    converting_handler( V* v, P* p )
        : value_(v), parent_(p), inner_(v, this)
    {}

    void signal_value()
    {
        key_ = {};
        inner_active_ = -1;
    }

    void signal_end()
    {
        key_ = {};
        inner_active_ = -1;

        parent_->signal_value();
    }

#define BOOST_JSON_INVOKE_INNER(fn) \
    if( inner_active_ < 0 ) \
    { \
        BOOST_JSON_FAIL( ec, error::not_object ); \
        return false; \
    } \
    return mp11::mp_with_index<mp11::mp_size<Dm>>( inner_active_, [&](auto I) { \
        return get<I>( inner_.handlers_ ).fn; \
    });

    bool on_object_begin( error_code& ec )
    {
        if( inner_active_ < 0 )
            return true;

        BOOST_JSON_INVOKE_INNER( on_object_begin(ec) );
    }

    bool on_object_end( std::size_t n, error_code& ec )
    {
        if( inner_active_ < 0 )
        {
            parent_->signal_value();
            return true;
        }

        BOOST_JSON_INVOKE_INNER( on_object_end(n, ec) );
    }

    bool on_array_begin( error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_array_begin(ec) );
    }

    bool on_array_end( std::size_t n, error_code& ec )
    {
        if( inner_active_ < 0 )
        {
            parent_->signal_end();
            return true;
        }

        BOOST_JSON_INVOKE_INNER( on_array_end(n, ec) );
    }

    bool on_key_part( string_view sv, std::size_t n, error_code& ec )
    {
        if( inner_active_ < 0 )
        {
            key_.append( sv.data(), sv.size() );
            return true;
        }

        BOOST_JSON_INVOKE_INNER( on_key_part(sv, n, ec) );
    }

    bool on_key( string_view sv, std::size_t n, error_code& ec )
    {
        if( inner_active_ >= 0 )
        {
            BOOST_JSON_INVOKE_INNER( on_key(sv, n, ec) );
        }

        key_.append( sv.data(), sv.size() );

        int i = 0;

        mp11::mp_for_each<Dm>([&](auto D) {
            if( key_ == D.name )
                inner_active_ = i;
            ++i;
        });

        if( inner_active_ < 0 )
        {
            BOOST_JSON_FAIL(ec, error::unknown_name);
            return false;
        }

        return true;
    }

    bool on_string_part( string_view sv, std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_string_part(sv, n, ec) );
    }

    bool on_string( string_view sv, std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_string(sv, n, ec) );
    }

    bool on_number_part( string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_number_part(sv, ec) );
    }

    bool on_int64( std::int64_t v, string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_int64(v, sv, ec) );
    }

    bool on_uint64( std::uint64_t v, string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_uint64(v, sv, ec) );
    }

    bool on_double( double v, string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_double(v, sv, ec) );
    }

    bool on_bool( bool v, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_bool(v, ec) );
    }

    bool on_null( error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_null(ec) );
    }

#undef BOOST_JSON_INVOKE_INNER
};

// into_handler
template< class V >
class into_handler
{
private:

    using inner_handler_type = get_handler<V, into_handler>;

    inner_handler_type inner_;
    bool inner_active_ = true;

public:

    into_handler( into_handler const& ) = delete;
    into_handler& operator=( into_handler const& ) = delete;

public:

    static constexpr std::size_t max_object_size = object::max_size();
    static constexpr std::size_t max_array_size = array::max_size();
    static constexpr std::size_t max_key_size = string::max_size();
    static constexpr std::size_t max_string_size = string::max_size();

public:

    explicit into_handler( V* v ): inner_( v, this )
    {
    }

    void signal_value()
    {
    }

    void signal_end()
    {
    }

    bool on_document_begin( error_code& )
    {
        return true;
    }

    bool on_document_end( error_code& )
    {
        inner_active_ = false;
        return true;
    }

#define BOOST_JSON_INVOKE_INNER(f) \
    if( !inner_active_ ) \
    { \
        BOOST_JSON_FAIL( ec, error::extra_data ); \
        return false;\
    } \
    else \
        return inner_.f

    bool on_object_begin( error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_object_begin( ec ) );
    }

    bool on_object_end( std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_object_end( n, ec ) );
    }

    bool on_array_begin( error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_array_begin( ec ) );
    }

    bool on_array_end( std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_array_end( n, ec ) );
    }

    bool on_key_part( string_view sv, std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_key_part( sv, n, ec ) );
    }

    bool on_key( string_view sv, std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_key( sv, n, ec ) );
    }

    bool on_string_part( string_view sv, std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_string_part( sv, n, ec ) );
    }

    bool on_string( string_view sv, std::size_t n, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_string( sv, n, ec ) );
    }

    bool on_number_part( string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_number_part( sv, ec ) );
    }

    bool on_int64( std::int64_t v, string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_int64( v, sv, ec ) );
    }

    bool on_uint64( std::uint64_t v, string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_uint64( v, sv, ec ) );
    }

    bool on_double( double v, string_view sv, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_double( v, sv, ec ) );
    }

    bool on_bool( bool v, error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_bool( v, ec ) );
    }

    bool on_null( error_code& ec )
    {
        BOOST_JSON_INVOKE_INNER( on_null( ec ) );
    }

    bool on_comment_part(string_view, error_code&)
    {
        return true;
    }

    bool on_comment(string_view, error_code&)
    {
        return true;
    }

#undef BOOST_JSON_INVOKE_INNER
};

} // namespace detail
} // namespace boost
} // namespace json

#endif // BOOST_JSON_NO_PARSE_INTO

#endif
