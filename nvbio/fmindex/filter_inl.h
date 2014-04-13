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

namespace nvbio {

namespace fmindex {

// return the size of a given range
struct range_size
{
    typedef uint2  argument_type;
    typedef uint64 result_type;

    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    uint64 operator() (const uint2 range) const { return 1u + range.y - range.x; }
};

template <typename index_type, typename string_set_type>
struct rank_functor
{
    typedef uint32  argument_type;
    typedef uint2   result_type;

    // constructor
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    rank_functor(
        const index_type        _index,
        const string_set_type   _string_set) :
    index       ( _index ),
    string_set  ( _string_set ) {}

    // functor operator
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const argument_type string_id) const
    {
        typedef typename string_set_type::string_type   string_type;

        // fetch the given string
        const string_type string = string_set[ string_id ];

        // and match it in the FM-index
        return match( index, string, length( string ) );
    }

    const index_type        index;
    const string_set_type   string_set;
};

template <typename index_type>
struct filter_results
{
    typedef uint64  argument_type;
    typedef uint2   result_type;

    // constructor
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    filter_results(
        const index_type        _index,
        const uint32            _n_queries,
        const uint64*           _slots,
        const uint2*            _ranges) :
    index       ( _index ),
    n_queries   ( _n_queries ),
    slots       ( _slots ),
    ranges      ( _ranges ) {}

    // functor operator
    NVBIO_FORCEINLINE NVBIO_HOST_DEVICE
    result_type operator() (const uint64 output_index) const
    {
        // find the text q-gram slot corresponding to this output index
        const uint32 slot = uint32( upper_bound(
            output_index,
            slots,
            n_queries ) - slots );

        // fetch the corresponding text position
        const uint32 string_id   = slot;

        // locate the hit position
        const uint2  range       = ranges[ slot ];
        const uint64 base_slot   = slot ? slots[ slot-1 ] : 0u;
        const uint32 local_index = output_index - base_slot;

        const uint32 index_pos = locate( index, range.x + local_index );

        // and write out the pair (qgram_pos,text_pos)
        return make_uint2( index_pos, string_id );
    }

    const index_type        index;
    const uint32            n_queries;
    const uint64*           slots;
    const uint2*            ranges;
};

} // namespace fmindex


// enact the filter on an FM-index and a string-set
//
// \param fm_index         the FM-index
// \param string-set       the query string-set
//
// \return the total number of hits
//
template <typename fm_index_type>
template <typename string_set_type>
uint64 FMIndexFilter<host_tag, fm_index_type>::rank(
    const fm_index_type&    index,
    const string_set_type&  string_set)
{
    // save the query
    m_n_queries   = string_set.size();
    m_index       = index;

    // alloc enough storage for the results
    m_ranges.resize( m_n_queries );
    m_slots.resize( m_n_queries );

    // search the strings in the index, obtaining a set of ranges
    thrust::transform(
        thrust::make_counting_iterator<uint32>(0u),
        thrust::make_counting_iterator<uint32>(0u) + m_n_queries,
        m_ranges.begin(),
        fmindex::rank_functor<fm_index_type,string_set_type>( m_index, string_set ) );

    // scan their size to determine the slots
    thrust::inclusive_scan(
        thrust::make_transform_iterator( m_ranges.begin(), fmindex::range_size() ),
        thrust::make_transform_iterator( m_ranges.begin(), fmindex::range_size() ) + m_n_queries,
        m_slots.begin() );

    // determine the total number of occurrences
    m_n_occurrences = m_slots[ m_n_queries-1 ];
    return m_n_occurrences;
}

// enumerate all hits in a given range
//
// \tparam hits_iterator         a hit_type iterator
//
template <typename fm_index_type>
template <typename hits_iterator>
void FMIndexFilter<host_tag,fm_index_type>::locate(
    const uint64    begin,
    const uint64    end,
    hits_iterator   hits)
{
    // and fill it
    thrust::transform(
        thrust::make_counting_iterator<uint64>(0u) + begin,
        thrust::make_counting_iterator<uint64>(0u) + end,
        hits,
        fmindex::filter_results<fm_index_type>(
            m_index,
            m_n_queries,
            nvbio::plain_view( m_slots ),
            nvbio::plain_view( m_ranges ) ) );
}

// enact the filter on an FM-index and a string-set
//
// \param fm_index         the FM-index
// \param string-set       the query string-set
//
// \return the total number of hits
//
template <typename fm_index_type>
template <typename string_set_type>
uint64 FMIndexFilter<device_tag,fm_index_type>::rank(
    const fm_index_type&    index,
    const string_set_type&  string_set)
{
    // save the query
    m_n_queries   = string_set.size();
    m_index       = index;

    // alloc enough storage for the results
    m_ranges.resize( m_n_queries );
    m_slots.resize( m_n_queries );

    // search the strings in the index, obtaining a set of ranges
    thrust::transform(
        thrust::make_counting_iterator<uint32>(0u),
        thrust::make_counting_iterator<uint32>(0u) + m_n_queries,
        m_ranges.begin(),
        fmindex::rank_functor<fm_index_type,string_set_type>( m_index, string_set ) );

    // scan their size to determine the slots
    cuda::inclusive_scan(
        m_n_queries,
        thrust::make_transform_iterator( m_ranges.begin(), fmindex::range_size() ),
        m_slots.begin(),
        thrust::plus<uint64>(),
        d_temp_storage );

    // determine the total number of occurrences
    m_n_occurrences = m_slots[ m_n_queries-1 ];
    return m_n_occurrences;
}

// enumerate all hits in a given range
//
// \tparam hits_iterator         a hit_type iterator
//
template <typename fm_index_type>
template <typename hits_iterator>
void FMIndexFilter<device_tag,fm_index_type>::locate(
    const uint64    begin,
    const uint64    end,
    hits_iterator   hits)
{
    // and fill it
    thrust::transform(
        thrust::make_counting_iterator<uint64>(0u) + begin,
        thrust::make_counting_iterator<uint64>(0u) + end,
        device_iterator( hits ),
        fmindex::filter_results<fm_index_type>(
            m_index,
            m_n_queries,
            nvbio::plain_view( m_slots ),
            nvbio::plain_view( m_ranges ) ) );
}

} // namespace nvbio