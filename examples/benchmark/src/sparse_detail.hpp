#ifndef SPARSE_COLL_HPP
#define SPARSE_COLL_HPP

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <set>
#include <sstream>
#include <tuple>
#include <unordered_map>

namespace sparse_detail
{

  template <class IType>
  struct incremental_indices_distributor
  {
      incremental_indices_distributor(IType from_range, IType to_range, IType step = 1)
      {
          from = std::min(from_range, to_range);
          to = std::max(from_range, to_range);

          size_t unique_indices_count = (to - from) / step;
          if (unique_indices_count == 0)
          {
              throw std::runtime_error(std::string("Invalid range! from: ") + std::to_string(from) +
                                       ", to: " + std::to_string(to) +
                                       ", step: " + std::to_string(step));
          }

          generated_indices.resize(unique_indices_count);
          generated_indices[0] = from;
          for(size_t i = 1; i < unique_indices_count; i++)
          {
              generated_indices[i] = generated_indices[i - 1] + step;
          }

          cur_index = 0;
      }

      IType operator() ()
      {
          return generated_indices.at(cur_index ++ % generated_indices.size());
      }

  private:
      std::vector<IType> generated_indices;
      IType from;
      IType to;
      std::atomic<size_t> cur_index;
  };

  template<class ValueType, class IndexType, class IndicesDistributorType>
  void fill_sparse_data(const std::tuple<size_t, size_t>& expected_recv_counts,
                        IndicesDistributorType& generator,
                        size_t elem_count, IndexType* send_ibuf, ValueType* send_buf,
                        ValueType* recv_buf, size_t& recv_icount, size_t& recv_vcount,
                        size_t rank)
  {

      recv_icount = std::get<0>(expected_recv_counts);
      recv_vcount = std::get<1>(expected_recv_counts);
      size_t vdim_count = recv_vcount / recv_icount;
      for (size_t i_idx = 0; i_idx < recv_icount; i_idx++)
      {
          send_ibuf[i_idx] = generator();
          for (size_t e_idx = 0; e_idx < vdim_count; e_idx++)
          {
              if (i_idx * vdim_count + e_idx < elem_count)
              {
                  send_buf[i_idx * vdim_count + e_idx] = (rank + 1 + e_idx);
              }
          }
      }

      std::fill(recv_buf, recv_buf + elem_count, ValueType{0});
  }

  // override for ccl::bfp16
  template<class IndexType, class IndicesDistributorType>
  void fill_sparse_data(const std::tuple<size_t, size_t>& expected_recv_counts,
                        IndicesDistributorType& generator,
                        size_t elem_count, IndexType* send_ibuf, ccl::bfp16* send_buf,
                        ccl::bfp16* recv_buf, size_t& recv_icount, size_t& recv_vcount,
                        size_t rank)
  {

      recv_icount = std::get<0>(expected_recv_counts);
      recv_vcount = std::get<1>(expected_recv_counts);
      size_t vdim_count = recv_vcount / recv_icount;
      
      using from_type = float;
      std::vector<from_type> send_buf_from(elem_count);

      
      for (size_t i_idx = 0; i_idx < recv_icount; i_idx++)
      {
          send_ibuf[i_idx] = generator();
          for (size_t e_idx = 0; e_idx < vdim_count; e_idx++)
          {
              if (i_idx * vdim_count + e_idx < elem_count)
              {
                  send_buf_from[i_idx * vdim_count + e_idx] = 1.0f / (rank + 1 + e_idx);
              }
          }
      }

      std::fill(recv_buf, recv_buf + elem_count, ccl::bfp16{0});
      // convert send_buf from float to send_buf ib bfp16
      convert_fp32_to_bfp16_arrays(send_buf_from.data(), send_buf, elem_count);
  }

  template<class ValueType, class IndexType>
  void check_sparse_result(const std::tuple<size_t, size_t>& expected_recv_counts,
                           size_t elem_count,
                           const IndexType* send_ibuf, const ValueType* send_buf,
                           const IndexType* recv_ibuf, const ValueType* recv_buf,
                           size_t recv_icount, size_t recv_vcount,
                           size_t comm_size, size_t comm_rank)
  {
      size_t indices_count, vdim_count;
      std::tie(indices_count, vdim_count) = expected_recv_counts;
      vdim_count = vdim_count / indices_count;

      std::vector<IndexType> aggregated_indices;
      aggregated_indices.reserve(indices_count * comm_size);

      std::vector<ValueType> aggregated_values;
      aggregated_values.reserve(indices_count * vdim_count * comm_size);

      std::vector<ValueType> base_send_data(send_buf, send_buf + indices_count * vdim_count);
      std::transform(base_send_data.begin(), base_send_data.end(),
                     base_send_data.begin(),
                     std::bind(std::minus<ValueType>(),
                               std::placeholders::_1, comm_rank));

      for(size_t rank_index = 0; rank_index < comm_size; rank_index++)
      {
          std::copy(send_ibuf, send_ibuf + indices_count,
                    std::back_inserter(aggregated_indices));

          std::transform(base_send_data.begin(), base_send_data.end(),
                         std::back_inserter(aggregated_values),
                         std::bind(std::plus<ValueType>(),
                                   std::placeholders::_1, rank_index));
      }

      /*calculate expected values*/
      using values_array = std::vector<ValueType>;
      using values_array_for_indices = std::unordered_map<IndexType, values_array>;

      values_array_for_indices expected;
      for (size_t index_pos = 0; index_pos < aggregated_indices.size(); index_pos++)
      {
          IndexType index = aggregated_indices[index_pos];

          typename values_array::iterator from = aggregated_values.begin();
          typename values_array::iterator to = aggregated_values.begin();
          std::advance(from, index_pos * vdim_count);
          std::advance(to, (index_pos + 1) * vdim_count);

          auto candidate_it = expected.find(index);
          if (candidate_it == expected.end())
          {
              expected.emplace(std::piecewise_construct,
                               std::forward_as_tuple(index),
                               std::forward_as_tuple(from, to));
          }
          else
          {
              std::transform(candidate_it->second.begin(), candidate_it->second.end(),
                             from, candidate_it->second.begin(), 
                             std::plus<ValueType>());
          }
      }

      // check received values
      for (size_t index_pos = 0; index_pos < recv_icount; index_pos++)
      {
          IndexType recv_index_value = recv_ibuf[index_pos];
          auto expected_it = expected.find(recv_index_value);
          if (expected_it == expected.end())
          {
              throw std::runtime_error(std::string(__FUNCTION__) + " - incorrect index received: " +
                                       std::to_string(recv_index_value));
          }

          const values_array& expected_values = expected_it->second;

          const ValueType* from = recv_buf + index_pos * vdim_count;
          const ValueType* to = from + vdim_count;

          values_array got_values(from, to);
          if (got_values != expected_values)
          {
              std::stringstream ss;
              const char* val_ptr = getenv("CCL_ATL_TRANSPORT");
              if (!val_ptr or !strcmp(val_ptr, "mpi"))
              {
                  ss << "Environment value CCL_ATL_TRANSPORT=" << (val_ptr ? val_ptr : "<default>")
                     << ". MPI has a limited support of sparse allreduce. " 
                     << "Make sure, that the following conditions are satisfied: \n"
                     << "\tCCL_WORKER_OFFLOAD=0\n"
                     << "\tYou have only one non-repeated sparse collective in colls list.\n\n"
                     << "Failed scenario settings:\n";
              }
              ss << "elem_count: " << elem_count
                 << ", indices_count: " << indices_count
                 << ", vdim_count:" <<  vdim_count
                 << ", recv_icount: " << recv_icount
                 << ", recv_vcount: " << recv_vcount;

              ss << "\nValues got:\n";
              std::copy(got_values.begin(), got_values.end(),
                        std::ostream_iterator<ValueType>(ss, ","));
              ss << "\nValues expected:\n";
              std::copy(expected_values.begin(), expected_values.end(),
                        std::ostream_iterator<ValueType>(ss, ","));

              ss << "\nRank: " << comm_rank << ", send_bufs:\n";
              std::copy(send_buf, send_buf + indices_count * vdim_count,
                        std::ostream_iterator<ValueType>(ss, ","));
              ss << "\nRank: " << comm_rank << ", send_ibufs:\n";
              std::copy(send_ibuf, send_ibuf + indices_count,
                        std::ostream_iterator<IndexType>(ss, ","));

              ss << "\nRank: " << comm_rank << ", recv_bufs:\n";
              std::copy(recv_buf, recv_buf + indices_count * vdim_count,
                        std::ostream_iterator<ValueType>(ss, ","));
              ss << "\nRank: " << comm_rank << ", recv_ibufs:\n";
              std::copy(recv_ibuf, recv_ibuf + indices_count,
                        std::ostream_iterator<IndexType>(ss, ","));

              ss << "\nAggregated indices:\n";
              std::copy(aggregated_indices.begin(), aggregated_indices.end(),
                        std::ostream_iterator<IndexType>(ss, ","));
              ss << "\nAggregated values:\n";
              std::copy(aggregated_values.begin(), aggregated_values.end(),
                        std::ostream_iterator<ValueType>(ss, ","));

              throw std::runtime_error(std::string(__FUNCTION__) + " - incorrect values received!\n" + ss.str());
          }
      }
  }

  // override for ccl::bfp16
  template<class IndexType>
  void check_sparse_result(const std::tuple<size_t, size_t>& expected_recv_counts,
                           size_t elem_count,
                           const IndexType* send_ibuf, const ccl::bfp16* send_buf,
                           const IndexType* recv_ibuf, const ccl::bfp16* recv_buf,
                           size_t recv_icount, size_t recv_vcount,
                           size_t comm_size, size_t comm_rank)
  {
      size_t indices_count, vdim_count;
      std::tie(indices_count, vdim_count) = expected_recv_counts;
      vdim_count = vdim_count / indices_count;

      std::vector<IndexType> aggregated_indices;
      aggregated_indices.reserve(indices_count * comm_size);

      std::vector<float> aggregated_values;
      aggregated_values.reserve(indices_count * vdim_count * comm_size);

      for(size_t rank_index = 0; rank_index < comm_size; rank_index++)
      {
          std::copy(send_ibuf, send_ibuf + indices_count,
                    std::back_inserter(aggregated_indices));

          for (size_t i_idx = 0; i_idx < indices_count; i_idx++)
          {
              for (size_t e_idx = 0; e_idx < vdim_count; e_idx++)
              {
                  if (i_idx * vdim_count + e_idx < elem_count)
                  {
                      std::back_inserter(aggregated_values) = 1.0f / (rank_index + 1 + e_idx);
                  }
              }
          }
      }

      /*calculate expected values*/
      using values_array = std::vector<float>;
      using values_array_for_indices = std::unordered_map<IndexType, values_array>;

      values_array_for_indices expected;
      for (size_t index_pos = 0; index_pos < aggregated_indices.size(); index_pos++)
      {
          IndexType index = aggregated_indices[index_pos];

          typename values_array::iterator from = aggregated_values.begin();
          typename values_array::iterator to = aggregated_values.begin();
          std::advance(from, index_pos * vdim_count);
          std::advance(to, (index_pos + 1) * vdim_count);

          auto candidate_it = expected.find(index);
          if (candidate_it == expected.end())
          {
              expected.emplace(std::piecewise_construct,
                               std::forward_as_tuple(index),
                               std::forward_as_tuple(from, to));
          }
          else
          {
              std::transform(candidate_it->second.begin(), candidate_it->second.end(),
                             from, candidate_it->second.begin(), std::plus<float>());
          }
      }

      // check received values
      std::vector<float> recv_buf_float(recv_vcount, float{0});
      convert_bfp16_to_fp32_arrays(reinterpret_cast<void*>(const_cast<ccl::bfp16*>(recv_buf)), recv_buf_float.data(), recv_vcount);
      
      /* https://www.mcs.anl.gov/papers/P4093-0713_1.pdf */
      /* added conversion error float->bfp16 for comm_size == 1*/
      double log_base2 = log(comm_size != 1 ? comm_size : 2 ) / log(2);
      double g = (log_base2 * BFP16_PRECISION)/(1 - (log_base2 * BFP16_PRECISION));
    
      for (size_t index_pos = 0; index_pos < recv_icount; index_pos++)
      {
          IndexType recv_index_value = recv_ibuf[index_pos];
          auto expected_it = expected.find(recv_index_value);
          if (expected_it == expected.end())
          {
              throw std::runtime_error(std::string(__FUNCTION__) + "_bfp16 - incorrect index received: " +
                                       std::to_string(recv_index_value));
          }

          const float* from = recv_buf_float.data() + index_pos * vdim_count;
          const float* to = from + vdim_count;
          const values_array& expected_values = expected_it->second;
          if (vdim_count != expected_values.size())
          {
              throw std::runtime_error(std::string(__FUNCTION__) + "_bfp16 - incorrect recv_buf count, got: " + 
                                       std::to_string(std::distance(from, to)) + ", expected: " +
                                       std::to_string(expected_values.size()));
          }
          
          for(size_t i = 0; i < expected_values.size(); i++)
          {
              double compare_max_error = g * expected_values[i] * 2;
              if (fabs(compare_max_error) < fabs(expected_values[i] - from[i]))
              {
                  std::stringstream ss;
                  const char* val_ptr = getenv("CCL_ATL_TRANSPORT");
                  if (!val_ptr or !strcmp(val_ptr, "mpi"))
                  {
                      ss << "Environment value CCL_ATL_TRANSPORT=" << (val_ptr ? val_ptr : "<default>")
                         << ". MPI has a limited support of sparse allreduce. "
                         << "Make sure, that the following conditions are satisfied: \n\n"
                         << "\tCCL_WORKER_OFFLOAD=0\n"
                         << "\tYou have only one non-repeated sparse collective in colls list.\n\n"
                         << "Failed scenario settings:\n";
                  }
                  
                  ss << "elem_count: " << elem_count
                     << ", indices_count: " << indices_count
                     << ", vdim_count:" <<  vdim_count
                     << ", recv_icount: " << recv_icount
                     << ", recv_vcount: " << recv_vcount
                     << ", absolute_max_error: " << compare_max_error;

                  ss << "\nValues got:\n";
                  std::copy(from, to,
                            std::ostream_iterator<float>(ss, ","));
                  ss << "\nValues expected:\n";
                  std::copy(expected_values.begin(), expected_values.end(),
                            std::ostream_iterator<float>(ss, ","));

                  ss << "\nRank: " << comm_rank << ", send_ibufs:\n";
                  std::copy(send_ibuf, send_ibuf + indices_count,
                            std::ostream_iterator<IndexType>(ss, ","));

                  ss << "\nRank: " << comm_rank << ", recv_bufs:\n";
                  std::copy(recv_buf_float.begin(), recv_buf_float.end(),
                            std::ostream_iterator<float>(ss, ","));
                  ss << "\nRank: " << comm_rank << ", recv_ibufs:\n";
                  std::copy(recv_ibuf, recv_ibuf + indices_count,
                            std::ostream_iterator<IndexType>(ss, ","));

                  ss << "\nAggregated indices:\n";
                  std::copy(aggregated_indices.begin(), aggregated_indices.end(),
                            std::ostream_iterator<IndexType>(ss, ","));
                  ss << "\nAggregated values:\n";
                  std::copy(aggregated_values.begin(), aggregated_values.end(),
                            std::ostream_iterator<float>(ss, ","));

                  throw std::runtime_error(std::string(__FUNCTION__) + "_bfp16 - incorrect values received!\n" + ss.str());
              }
          }
      }
  }
} /* End sparse detail content */



#endif /* SPARSE_COLL_HPP */
