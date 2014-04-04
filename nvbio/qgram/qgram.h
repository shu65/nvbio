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

#include <nvbio/basic/types.h>
#include <nvbio/basic/numbers.h>
#include <nvbio/basic/algorithms.h>
#include <nvbio/basic/cuda/primitives.h>
#include <nvbio/basic/thrust_view.h>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/sort.h>
#include <thrust/for_each.h>
#include <thrust/iterator/constant_iterator.h>
#include <thrust/iterator/counting_iterator.h>

///\page qgram_page Q-Gram Module
///\htmlonly
/// <img src="nvidia_cubes.png" style="position:relative; bottom:-10px; border:0px;"/>
///\endhtmlonly
///\par
/// This module contains a series of functions to operate on Q-Grams and build a compact Q-Gram Index over
/// a string T, with memory consumption and query time proportional to O(unique(T)) and O(log(unique(T))) respectively,
/// where unique(T) is the number of unique q-grams in T.
/// This is achieved by keeping a plain sorted list of the unique q-grams in T, together with an index of their occurrences
/// in the original string T.
/// This data-structure offers better construction speed and memory consumption than the \ref QGroupIndex, though the
/// query time is asymptotically higher.
///
///
/// \section TechnicalOverviewSection Technical Overview
///\par
/// A complete list of the classes and functions in this module is given in the \ref QGramIndex documentation.
///

namespace nvbio {

///
///@defgroup QGramIndex QGramIndex Module
/// This module contains a series of classes and functions to build a compact Q-Gram Index over
/// a string T, with memory consumption and query time proportional to O(unique(T)) and O(log(unique(T))) respectively,
/// where unique(T) is the number of unique q-grams in T.
/// This is achieved by keeping a plain sorted list of the unique q-grams in T, together with an index of their occurrences
/// in the original string T.
/// This data-structure offers better construction speed and memory consumption than the \ref QGroupIndex, though the
/// query time is asymptotically higher.
///
///@{
///

/// A plain view of a q-group index
///
struct QGramIndexView
{
    typedef uint64*                         qgram_vector_type;
    typedef uint32*                         index_vector_type;

    /// constructor
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    QGramIndexView(
        const uint32        _Q                 = 0,
        const uint32        _n_unique_qgrams   = 0,
        qgram_vector_type   _qgrams            = NULL,
        index_vector_type   _slots             = NULL,
        index_vector_type   _index             = NULL) :
        Q               (_Q),
        n_unique_qgrams (_n_unique_qgrams),
        qgrams          (_qgrams),
        slots           (_slots),
        index           (_index) {}

    /// return the slots of P corresponding to the given qgram g
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint2 range(const uint64 g)
    {
        // find the slot where stored our q-gram
        const uint32 i = uint32( nvbio::lower_bound(
            g,
            qgrams,
            n_unique_qgrams ) - qgrams );

        // check whether we find what we were looking for
        if (i >= n_unique_qgrams || g != qgrams[i])
            return make_uint2( 0u, 0u );

        // return the range
        return make_uint2( slots[i], slots[i+1] );
    }

    uint32              Q;
    uint32              n_unique_qgrams;
    qgram_vector_type   qgrams;
    index_vector_type   slots;
    index_vector_type   index;
};

/// A host-side q-group index
///
struct QGramIndexHost
{
    static const uint32 WORD_SIZE = 32;

    typedef thrust::host_vector<uint64>     qgram_vector_type;
    typedef thrust::host_vector<uint32>     index_vector_type;
    typedef QGramIndexView                  view_type;

    /// return the amount of device memory used
    ///
    uint64 used_host_memory() const
    {
        return qgrams.size() * sizeof(uint64) +
               slots.size()  * sizeof(uint32) +
               index.size()  * sizeof(uint32);
    }

    /// return the amount of device memory used
    ///
    uint64 used_device_memory() const { return 0u; }

    uint32              Q;
    uint32              n_unique_qgrams;
    qgram_vector_type   qgrams;
    index_vector_type   slots;
    index_vector_type   index;
};

/// A device-side q-group index
///
struct QGramIndexDevice
{
    typedef thrust::device_vector<uint64>   qgram_vector_type;
    typedef thrust::device_vector<uint32>   index_vector_type;
    typedef QGramIndexView                  view_type;

    /// build a q-group index from a given string T; the amount of storage required
    /// is basically O( A^q + |T|*32 ) bits, where A is the alphabet size.
    ///
    /// \tparam SYMBOL_SIZE     the size of the symbols, in bits
    /// \tparam string_type     the string iterator type
    ///
    /// \param q                the q parameter
    /// \param string_len       the size of the string
    /// \param string           the string iterator
    ///
    template <uint32 SYMBOL_SIZE, typename string_type>
    void build(
        const uint32        q,
        const uint32        string_len,
        const string_type   string);

    /// return the amount of device memory used
    ///
    uint64 used_host_memory() const { return 0u; }

    /// return the amount of device memory used
    ///
    uint64 used_device_memory() const
    {
        return qgrams.size() * sizeof(uint64) +
               slots.size()  * sizeof(uint32) +
               index.size()  * sizeof(uint32);
    }

    uint32              Q;
    uint32              n_unique_qgrams;
    qgram_vector_type   qgrams;
    index_vector_type   slots;
    index_vector_type   index;
};

/// return the plain view of a QGramIndex
///
QGramIndexView plain_view(QGramIndexDevice& qgram)
{
    return QGramIndexView(
        qgram.Q,
        qgram.n_unique_qgrams,
        nvbio::plain_view( qgram.qgrams ),
        nvbio::plain_view( qgram.slots ),
        nvbio::plain_view( qgram.index ) );
}

/// A utility functor to extract the i-th q-gram out of a string
///
/// \tparam SYMBOL_SIZE         the size of the symbols, in bits
/// \tparam string_type         the string iterator type
///
template <uint32 SYMBOL_SIZE, typename string_type>
struct string_qgram_functor
{
    static const uint32 WORD_SIZE        = 32;

    /// constructor
    ///
    /// \param _string_len       string length
    /// \param _string           string iterator
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    string_qgram_functor(const uint32 _Q, const uint32 _string_len, const string_type _string) :
        Q(_Q),
        string_len(_string_len),
        string(_string) {}

    /// functor operator
    ///
    /// \param i        position along the string
    ///
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint64 operator() (const uint32 i) const
    {
        const uint32 SYMBOL_MASK = (1u << SYMBOL_SIZE) - 1u;

        uint64 qgram = 0u;
        for (uint32 j = 0; j < Q; ++j)
            qgram |= uint64(i+j < string_len ? (string[i + j] & SYMBOL_MASK) : 0u) << (j*SYMBOL_SIZE);

        return qgram;
    }

    const uint32        Q;          ///< q-gram size
    const uint32        string_len; ///< string length
    const string_type   string;     ///< string iterator
};

///@} // end of the QGramIndex group

} // namespace nvbio

#include <nvbio/qgram/qgram_inl.h>
