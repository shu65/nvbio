/*
 * nvbio
 * Copyright (C) 2011-2014, NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <nvbio/strings/string_set.h>


namespace nvbio {

///@addtogroup Strings
///@{

///@addtogroup StringSetsModule
///@{

typedef uint32      string_prefix_coord_type;
typedef uint64      long_string_prefix_coord_type;

typedef uint32_2    string_set_prefix_coord_type;
typedef uint64_2    long_string_set_prefix_coord_type;

///@addtogroup Private
///@{

/// A class to represent a string prefix, i.e. an arbitrarily placed substring
///
/// \tparam StringType          the underlying string type
/// \tparam CoordType           the type of prefix coordinates, string_prefix_coord_type for strings, string_set_prefix_coord_type for string-sets
/// \tparam CoordDim            the number of coordinates, 1 for strings, 2 for string-sets
///
template <
    typename StringType,
    typename CoordType,
    uint32   CoordDim>
struct PrefixCore {};

/// A class to represent a string prefix, i.e. an arbitrarily placed substring
///
/// \tparam StringType          the underlying string type
/// \tparam CoordType           the type of prefix coordinates, uint32|uint64
///
template <
    typename StringType,
    typename CoordType>
struct PrefixCore<StringType,CoordType,1u>
{
    typedef StringType                                              string_type;
    typedef CoordType                                               coord_type;

    typedef typename std::iterator_traits<string_type>::value_type  symbol_type;
    typedef typename std::iterator_traits<string_type>::value_type  value_type;
    typedef typename std::iterator_traits<string_type>::reference   reference;

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    PrefixCore() {}

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    PrefixCore(
        const string_type   string,
        const coord_type    prefix) :
        m_string( string ),
        m_coords( prefix ) {}

    /// prefix size
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint32 size() const { return nvbio::length( m_string ) - m_coords; }

    /// prefix length
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint32 length() const { return size(); }

    /// indexing operator
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    symbol_type operator[] (const uint32 i) const { return m_string[ m_coords + i ]; }

    /// indexing operator
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    reference operator[] (const uint32 i) { return m_string[ m_coords + i ]; }

    /// return the prefix coordinates
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    coord_type coords() const { return m_coords; }

    string_type     m_string;       ///< the underlying string set
    coord_type      m_coords;       ///< the prefix coordinates
};

/// A class to represent a string prefix, i.e. an arbitrarily placed substring
///
/// \tparam StringType          the underlying string type
/// \tparam CoordType           the type of prefix coordinates, uint32|uint64
///
template <
    typename StringType,
    typename CoordType>
struct PrefixCore<StringType,CoordType,2u>
{
    typedef StringType                                              string_type;
    typedef CoordType                                               coord_type;

    typedef typename std::iterator_traits<string_type>::value_type  symbol_type;
    typedef typename std::iterator_traits<string_type>::value_type  value_type;
    typedef typename std::iterator_traits<string_type>::reference   reference;

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    PrefixCore() {}

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    PrefixCore(
        const string_type   string,
        const coord_type    prefix) :
        m_string( string ),
        m_coords( prefix ) {}

    /// prefix size
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint32 size() const { return nvbio::length( m_string ) - m_coords.y; }

    /// prefix length
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint32 length() const { return size(); }

    /// indexing operator
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    symbol_type operator[] (const uint32 i) const { return m_string[ m_coords.y + i ]; }

    /// indexing operator
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    reference operator[] (const uint32 i) { return m_string[ m_coords.y + i ]; }

    /// return the prefix coordinates
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    coord_type coords() const { return m_coords; }

    string_type     m_string;       ///< the underlying string set
    coord_type      m_coords;       ///< the prefix coordinates
};

///@} Private

/// A class to represent a string prefix, i.e. an arbitrarily placed substring
///
/// \tparam StringType          the underlying string type
/// \tparam CoordType           the type of prefix coordinates, string_prefix_coord_type for strings, string_set_prefix_coord_type for string-sets
/// \tparam CoordDim            the number of coordinates, 1 for strings, 2 for string-sets
///
template <
    typename StringType,
    typename CoordType>
struct Prefix : PrefixCore< StringType, CoordType, vector_traits<CoordType>::DIM >
{
    typedef PrefixCore< StringType, CoordType, vector_traits<CoordType>::DIM >  core_type;
    typedef StringType                                                          string_type;
    typedef CoordType                                                           coord_type;

    typedef typename std::iterator_traits<string_type>::value_type              symbol_type;
    typedef typename std::iterator_traits<string_type>::value_type              value_type;
    typedef typename std::iterator_traits<string_type>::reference               reference;

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    Prefix() {}

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    Prefix(
        const string_type   string,
        const coord_type    infix) : core_type( string, infix ) {}
}

/// make a prefix, i.e. a substring of a given string
///
/// \tparam StringType  the underlying string type
/// \tparam CoordType   the coordinates type, either string_prefix_coord_type or string_set_prefix_coord_type
///
/// \param string       the underlying string object
/// \param coords       the prefix coordinates
///
template <typename StringType, typename CoordType>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
Prefix<StringType,CoordType> make_prefix(const StringType string, const CoordType coords)
{
    return Prefix<StringType,CoordType>( string, coords );
}

///@addtogroup Private
///@{

/// Represent a set of prefixes of a string or string-set
///
/// \tparam SequenceType            the string or string-set type
/// \tparam PrefixIterator          the prefix iterator type - value_type can be string_prefix_coord_type for strings, string_set_prefix_coord_type for string-sets
/// \tparam CoordDim                the number of coordinates representing a prefix, 1 for strings, 2 for string-sets
///
template <
    typename SequenceType,
    typename PrefixIterator,
    uint32   CoordDim>
struct PrefixSetCore {};

/// Represent a set of prefixes of a string
///
/// \tparam SequenceType            the string or string-set container
/// \tparam PrefixIterator          the prefix iterator type - value_type can be uint32 or uint64
///
template <
    typename SequenceType,
    typename PrefixIterator>
struct PrefixSetCore<SequenceType,PrefixIterator,1u>
{
    typedef SequenceType                                                sequence_type;
    typedef PrefixIterator                                              prefix_iterator;

    typedef typename std::iterator_traits<PrefixIterator>::value_type   coord_type;
    typedef Prefix<sequence_type, coord_type>                           string_type;

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    PrefixSetCore() {}

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    PrefixSetCore(
        const uint32            size,
        const sequence_type     sequence,
        const prefix_iterator    prefixes) :
        m_size( size ),
        m_sequence( sequence ),
        m_prefixes( prefixes ) {}

    /// set size
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint32 size() const { return m_size; }

    /// indexing operator: access the i-th string
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    string_type operator[] (const uint32 i) const
    {
        const coord_type coords = m_prefixes[i];
        return string_type( m_sequence, coords );
    }

    uint32              m_size;
    sequence_type       m_sequence;
    prefix_iterator     m_prefixes;
};

/// Represent a set of prefixes of a string-set
///
/// \tparam SequenceType            the string or string-set type
/// \tparam PrefixIterator          the prefix iterator type - value_type can be string_set_prefix_coord_type or long_string_set_prefix_coord_type
///
template <
    typename SequenceType,
    typename PrefixIterator>
struct PrefixSetCore<SequenceType,PrefixIterator,2u>
{
    typedef SequenceType                                                sequence_type;
    typedef PrefixIterator                                              prefix_iterator;

    typedef typename sequence_type::string_type                         base_string_type;
    typedef typename std::iterator_traits<PrefixIterator>::value_type   coord_type;
    typedef Prefix<base_string_type, coord_type>                        string_type;

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    PrefixSetCore() {}

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    PrefixSetCore(
        const uint32            size,
        const sequence_type     sequence,
        const prefix_iterator   prefixes) :
        m_size( size ),
        m_sequence( sequence ),
        m_prefixes( prefixes ) {}

    /// set size
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint32 size() const { return m_size; }

    /// indexing operator: access the i-th string
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    string_type operator[] (const uint32 i) const
    {
        const coord_type coords = m_prefixes[i];
        return string_type( m_sequence[ coords.x ], coords );
    }

    uint32              m_size;
    sequence_type       m_sequence;
    prefix_iterator     m_prefixes;
};

///@} Private

/// return the string index of a given prefix
///
template <typename StringType, typename CoordType>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
uint32 string_id(const PrefixCore<StringType,CoordType,2u>& prefix) { return prefix.m_coords.x; }

/// return the length of a given prefix
///
template <typename StringType, typename CoordType, uint32 CoordDim>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
uint32 length(const PrefixCore<StringType,CoordType,CoordDim>& prefix) { return prefix.length(); }

/// Represent a set of prefixes of a string or string-set. An PrefixSet is a \ref StringSetAnchor "String Set".
///
/// \tparam SequenceType        the string or string-set type
/// \tparam PrefixIterator      the prefix iterator type - value_type can be string_prefix_coord_type for strings, string_set_prefix_coord_type for string-sets
///
template <
    typename SequenceType,
    typename PrefixIterator>
struct PrefixSet : public PrefixSetCore<
                            SequenceType,
                            PrefixIterator,
                            vector_traits<typename std::iterator_traits<PrefixIterator>::value_type>::DIM>
{
    typedef PrefixSetCore<
        SequenceType,
        PrefixIterator,
        vector_traits<typename std::iterator_traits<PrefixIterator>::value_type>::DIM>   base_type;

    typedef SequenceType                                                sequence_type;      ///< the underlying sequence type
    typedef PrefixIterator                                              prefix_iterator;    ///< the underlingy prefix iterator type
    typedef typename iterator_system<PrefixIterator>::type              system_tag;         ///< the system tag

    typedef typename base_type::coord_type                              coord_type;         ///< the prefix coordinates type
    typedef typename base_type::string_type                             string_type;        ///< the prefix string type

    typedef StringSetIterator< PrefixSet<SequenceType,InfixIterator> >        iterator;     ///< the iterator type
    typedef StringSetIterator< PrefixSet<SequenceType,InfixIterator> >  const_iterator;     ///< the const_iterator type

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    PrefixSet() {}

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    PrefixSet(
        const uint32            size,
        const sequence_type     sequence,
        const prefix_iterator    prefixes) :
        base_type( size, sequence, prefixes ) {}

    /// begin iterator
    ///
    const_iterator begin() const { return const_iterator(*this,0u); }

    /// begin iterator
    ///
    const_iterator end() const { return const_iterator(*this,base_type::size()); }

    /// begin iterator
    ///
    iterator begin() { return iterator(*this,0u); }

    /// begin iterator
    ///
    iterator end() { return iterator(*this,base_type::size()); }
};

///@} StringSetsModule
///@} Strings

} // namespace nvbio

//#include <nvbio/basic/prefix_inl.h>
