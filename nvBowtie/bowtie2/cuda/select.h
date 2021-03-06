/*
 * nvbio
 * Copyright (C) 2012-2014, NVIDIA Corporation
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

///
///\file select.h
///

#pragma once

#include <nvBowtie/bowtie2/cuda/defs.h>
#include <nvBowtie/bowtie2/cuda/seed_hit.h>
#include <nvBowtie/bowtie2/cuda/seed_hit_deque_array.h>
#include <nvBowtie/bowtie2/cuda/scoring_queues.h>
#include <nvBowtie/bowtie2/cuda/params.h>
#include <nvbio/io/alignments.h>
#include <nvbio/basic/cuda/pingpong_queues.h>
#include <nvbio/basic/priority_deque.h>
#include <nvbio/basic/vector_view.h>
#include <nvbio/basic/strided_iterator.h>
#include <nvbio/basic/sum_tree.h>

namespace nvbio {
namespace bowtie2 {
namespace cuda {

template <typename ScoringScheme> struct BaseScoringPipelineState;
template <typename ScoringScheme> struct BestApproxScoringPipelineState;

///@addtogroup nvBowtie
///@{

/// \defgroup Select
///
/// The functions in this module implement a pipeline stage in which, for all the active reads in the
/// \ref ScoringQueues, a group of seed hits is selected for a new round of \ref Scoring.
/// In terms of inputs and outputs, this stage takes the set of input ScoringQueues::active_reads, and
/// fills the ScoringQueues::hits with a new set of hits, writing the HitQueues::read_id, HitQueues::seed,
/// and HitQueues::loc fields. The hits' location specified by the HitQueues::loc field is expressed in
/// Suffix Array coordinates.
/// Finally, this stage is also responsible for producing a new set of ScoringQueues::active_reads.
///
/// \b inputs:
///  - SeedHitDequeArray
///  - ScoringQueues::active_reads
///
/// \b outputs:
///  - SeedHitDequeArray
///  - ScoringQueues::active_reads
///  - HitQueues::read_id
///  - HitQueues::seed
///  - HitQueues::loc
///

///@addtogroup Select
///@{

///
/// Initialize the hit-selection pipeline
///
void select_init(
    const uint32                        count,
    const SeedHitDequeArrayDeviceView   hits,
    uint32*                             trys,
    const ParamsPOD                     params);

///
/// Initialize the hit-selection pipeline
///
template <typename ScoringScheme>
void select_init(BestApproxScoringPipelineState<ScoringScheme>& pipeline, const ParamsPOD& params);

///
/// a context class for the select_kernel
///
struct SelectBestApproxContext
{
    // constructor
    //
    // \param trys      the per-read vector of extension trys
    //
    SelectBestApproxContext(uint32* trys) : m_trys( trys ) {}

    // stopping function
    //
    // \return          true iff we can stop the hit selection process
    //
    NVBIO_FORCEINLINE NVBIO_DEVICE
    bool stop(const uint32 read_id) const { return m_trys[ read_id ] == 0; }

private:
    uint32* m_trys;
};

///
/// select next hit extensions from the top seed ranges
///
__global__ 
void select_n_from_top_range_kernel(
    const uint32        begin,
    const uint32        count,
    const uint32        n_reads,
    const SeedHit*      hit_data,
    const uint32*       hit_range_scan,
          uint32*       loc_queue,
          uint32*       seed_queue,
          uint32*       read_info);

///
/// Prepare for a round of seed extension by selecting the next SA row from each
/// of the seed-hit deque arrays (SeedHitDequeArray) bound to the active-reads in
/// the scoring queues (ScoringQueues::active_reads).
///
template <typename BatchType, typename ContextType>
void select(
    const BatchType                         read_batch,
    SeedHitDequeArrayDeviceView             hits,
    const ContextType                       context,
          ScoringQueuesDeviceView           scoring_queues,
    const ParamsPOD                         params);

///
/// Prepare for a round of seed extension by selecting the next SA row from each
/// of the seed-hit deque arrays (SeedHitDequeArray) bound to the active-reads in
/// the scoring queues (ScoringQueues::active_reads).
///
template <typename BatchType, typename ContextType>
void rand_select(
    const BatchType                         read_batch,
    SeedHitDequeArrayDeviceView             hits,
    uint32*                                 rseeds,
    const ContextType                       context,
          ScoringQueuesDeviceView           scoring_queues,
    const ParamsPOD                         params);

///
/// Prepare for a round of seed extension by selecting a set of up
/// to 'n_multi' SA rows from each of the seed-hit deque arrays (SeedHitDequeArray)
/// bound to the active-reads in the scoring queues (ScoringQueues::active_reads).
///
/// For each read in the input queue, this kernel generates:
///     1. one or zero output reads, in the main output read queue,
///     2. zero to 'n_multi' SA rows. These are made of three entries,
///        one in 'loc_queue', identifying the corresponding SA index,
///        one in 'seed_queue', storing information about the seed hit,
///        and one in 'parent_queue', storing the index of the "parent"
///        read in the output queue (i.e. the slot where the read is
///        is being stored)
///
template <typename BatchType, typename ContextType>
void select_multi(
    const BatchType                         read_batch,
    SeedHitDequeArrayDeviceView             hits,
    const ContextType                       context,
          ScoringQueuesDeviceView           scoring_queues,
    const uint32                            n_multi,
    const ParamsPOD                         params);

///
/// Prepare for a round of seed extension by selecting the next SA row from each
/// of the seed-hit deque arrays (SeedHitDequeArray) bound to the active-reads in
/// the scoring queues (ScoringQueues::active_reads).
///
template <typename BatchType, typename ContextType>
void select(
    const BatchType                         read_batch,
    SeedHitDequeArrayDeviceView             hits,
    uint32*                                 rseeds,
    const ContextType                       context,
          ScoringQueuesDeviceView           scoring_queues,
    const uint32                            n_multi,
    const ParamsPOD                         params);

///
/// Prepare for a round of seed extension by selecting the next SA row from each
/// of the seed-hit deque arrays (SeedHitDequeArray) bound to the active-reads in
/// the scoring queues (ScoringQueues::active_reads).
///
template <typename ScoringScheme, typename ContextType>
void select(
    const ContextType                                       context,
    const BestApproxScoringPipelineState<ScoringScheme>&    pipeline,
    const ParamsPOD                                         params);

///
/// Select next hit extensions for all-mapping
///
void select_all(
    const uint64                        begin,
    const uint32                        count,
    const uint32                        n_reads,
    const uint32                        n_hit_ranges,
    const uint64                        n_hits,
    const SeedHitDequeArrayDeviceView   hits,
    const uint32*                       hit_count_scan,
    const uint64*                       hit_range_scan,
          HitQueuesDeviceView           scoring_queues);

///
/// Prune the set of active reads based on whether we found the best alignments
///
void prune_search(
    const uint32                                    max_dist,
    const nvbio::cuda::PingPongQueuesView<uint32>   queues,
    const io::BestAlignments*                       best_data);

///@}  // group Select
///@}  // group nvBowtie

} // namespace cuda
} // namespace bowtie2
} // namespace nvbio

#include <nvBowtie/bowtie2/cuda/select_inl.h>
