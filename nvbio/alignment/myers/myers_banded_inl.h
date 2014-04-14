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

namespace nvbio {
namespace aln {

namespace priv {

///@addtogroup private
///@{


/// A set of Myers bit-vectors for generic alphabets
///
template <uint32 ALPHABET_SIZE>
struct MyersBitVectors
{
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    MyersBitVectors()
    {
        #pragma unroll ALPHABET_SIZE
        for (uint32 c = 0; c < ALPHABET_SIZE; c++)
            B[c] = 0u;
    }

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    void load(const uint8 c, const uint32 v) { B[c] |= v; }

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint32 get(const uint8 c) const { return B[c]; }

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    void shift()
    {
        #pragma unroll ALPHABET_SIZE
        for(uint32 c = 0; c < ALPHABET_SIZE; c++)
            B[c] >>= 1;
    }

    uint32 B[ALPHABET_SIZE];
};

/// A set of Myers bit-vectors for 2-letter alphabets
///
template <>
struct MyersBitVectors<2>
{
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    MyersBitVectors()
    {
        #pragma unroll 2
        for (uint32 c = 0; c < 2; c++)
            B[c] = 0u;
    }

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    void load(const uint8 c, const uint32 v)
    {
        const uint8 cc = c & 1u;
        if (cc == 0) B[0] |= v;
        if (cc == 1) B[1] |= v;
    }

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint32 get(const uint8 c) const
    {
        const uint8 cc = c & 1u;
        return cc == 0 ? B[0] : B[1];
    }

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    void shift()
    {
        #pragma unroll 2
        for(uint32 c = 0; c < 2; c++)
            B[c] >>= 1;
    }

    uint32 B[4];
};

/// A set of Myers bit-vectors for 4-letter alphabets
///
template <>
struct MyersBitVectors<4>
{
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    MyersBitVectors()
    {
        #pragma unroll 4
        for (uint32 c = 0; c < 4; c++)
            B[c] = 0u;
    }

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    void load(const uint8 c, const uint32 v)
    {
        const uint8 cc = c & 3u;
        if (cc == 0) B[0] |= v;
        if (cc == 1) B[1] |= v;
        if (cc == 2) B[2] |= v;
        if (cc == 3) B[3] |= v;
    }

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint32 get(const uint8 c) const
    {
        const uint8 cc = c & 3u;
        return cc <= 1 ?
               cc == 0 ? B[0] : B[1] :
               cc == 2 ? B[2] : B[3];
    }

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    void shift()
    {
        #pragma unroll 4
        for(uint32 c = 0; c < 4; c++)
            B[c] >>= 1;
    }

    uint32 B[4];
};

/// A set of Myers bit-vectors for 5-letter alphabets
///
template <>
struct MyersBitVectors<5>
{
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    MyersBitVectors()
    {
        #pragma unroll 5
        for (uint32 c = 0; c < 4; c++)
            B[c] = 0u;
    }

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    void load(const uint8 c, const uint32 v)
    {
        if (c == 0) B[0] |= v;
        if (c == 1) B[1] |= v;
        if (c == 2) B[2] |= v;
        if (c == 3) B[3] |= v;
        if (c == 4) B[4] |= v;
    }

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint32 get(const uint8 c) const
    {
        return c <= 1 ? (c == 0 ? B[0] : B[1]) :
               c <= 3 ? (c == 2 ? B[2] : B[3]) :
                                  B[4];
    }

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    void shift()
    {
        #pragma unroll 5
        for(uint32 c = 0; c < 5; c++)
            B[c] >>= 1;
    }

    uint32 B[5];
};

template <uint32 BAND_WIDTH>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
int diagonal_column(const uint32 B_c_ref, uint32& VP, uint32& VN)
{
    uint32 X = B_c_ref | VN;
    const uint32 D0 = ((VP + (X & VP)) ^ VP) | X;
    const uint32 HN = VP & D0;
    const uint32 HP = VN | ~ (VP | D0);
    X = D0 >> 1;
    VN = X & HP;
    VP = HN | ~ (X | HP);

    return 1 - ((D0 >> (BAND_WIDTH - 1)) & 1u);
}

template <uint32 BAND_WIDTH>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
int horizontal_column(const uint32 B_c_ref, uint32& VP, uint32& VN, const int s)
{
    uint32 X = B_c_ref | VN;
    const uint32 D0 = ((VP + (X & VP)) ^ VP) | X;
    const uint32 HN = VP & D0;
    const uint32 HP = VN | ~ (VP | D0);
    X = D0 >> 1;
    VN = X & HP;
    VP = HN | ~ (X | HP);

    return ((HP >> s) & 1) - ((HN >> s) & 1);
}

template <
    uint32   BAND_WIDTH,
    uint32   C,
    uint32   ALPHABET_SIZE,
    typename pattern_string,
    typename text_string,
    typename sink_type>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
bool banded_myers(
    const pattern_string pattern,
    const text_string    text,
    const int16          min_score,
          sink_type&     sink)
{
    const uint32 pattern_len = nvbio::length( pattern );
    const uint32 text_len    = nvbio::length( text );

    // check whether the text is long enough
    if (text_len < pattern_len)
        return false;

    MyersBitVectors<ALPHABET_SIZE> B;

    uint32 VP = 0xFFFFFFFFu;
    uint32 VN = 0x0u;

    int dist = -int( C );

    // initialize the column with C bases from the pattern
    #pragma unroll 16
    for (int j = 0; j < C; j++)
        B.load( pattern[j], 1u << (BAND_WIDTH - C + j) );

    // phase 1 (diagonal)
    const uint32 last = nvbio::min( text_len - 1u, pattern_len - C );
    for(uint32 i = 0; i < last; ++i)
    {
        B.shift();

        B.load( pattern[ C + i ], 1u << (BAND_WIDTH - 1) );

        dist -= diagonal_column<BAND_WIDTH>( B.get( text[i] ), VP, VN );
    }

    // phase 2 (horizontal)
    int s = BAND_WIDTH - 1u + pattern_len - C - last;
    for (uint32 i = last; i < text_len && s >= 0; ++i)
    {
        B.shift();

        dist -= horizontal_column<BAND_WIDTH>( B.get( text[i] ), VP, VN, s );

        // report a potential hit
        if (dist >= min_score)
            sink.report( dist, make_uint2( i+1, pattern_len ) );

        --s;
    }
    return true;
}

///
/// Calculate the banded alignment score between a pattern and a text string
/// under the edit distance.
///
/// \param pattern      shorter string (horizontal)
/// \param quals        qualities string
/// \param text         longer string (vertical)
/// \param pos          offset in the reference string
/// \param sink         output alignment sink
///
/// \return             false if the minimum score was not reached, true otherwise
///
template <
    uint32 BAND_LEN,
    AlignmentType TYPE,
    uint32 ALPHABET_SIZE,
    typename pattern_type,
    typename qual_type,
    typename text_type,
    typename sink_type>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
bool banded_alignment_score(
    const EditDistanceAligner<TYPE, MyersTag<ALPHABET_SIZE> >&  aligner,
    pattern_type                                                pattern,
    qual_type                                                   quals,
    text_type                                                   text,
    const int32                                                 min_score,
    sink_type&                                                  sink)
{
    return banded_myers<BAND_LEN,0u,ALPHABET_SIZE>(
        pattern,
        text,
        min_score,
        sink );
}

///
/// Calculate a window of the banded edit distance Dynamic Programming alignment matrix
/// between a pattern and a text strings. A checkpoint is used to pass the initial row
/// and store the final one at the end of the window.
///
/// \param pattern      shorter string (horizontal)
/// \param quals        qualities string
/// \param text         longer string (vertical)
/// \param pos          offset in the reference string
/// \param sink         output alignment sink
///
/// \return             false if the minimum score was not reached, true otherwise
///
template <
    uint32 BAND_LEN,
    AlignmentType TYPE,
    uint32 ALPHABET_SIZE,
    typename pattern_type,
    typename qual_type,
    typename text_type,
    typename sink_type,
    typename checkpoint_type>
NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
bool banded_alignment_score(
    const EditDistanceAligner<TYPE, MyersTag<ALPHABET_SIZE> >&  aligner,
    pattern_type                                                pattern,
    qual_type                                                   quals,
    text_type                                                   text,
    const int32                                                 min_score,
    const uint32                                                window_begin,
    const uint32                                                window_end,
    sink_type&                                                  sink,
    checkpoint_type                                             checkpoint)
{
    return banded_myers<BAND_LEN,0u,ALPHABET_SIZE>(
        pattern,
        text,
        min_score,
        sink );
}

/// @} // end of private group

} // namespace priv

} // namespace aln
} // namespace nvbio

