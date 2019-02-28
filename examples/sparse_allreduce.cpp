#include <initializer_list>
#include <iterator>
#include <unordered_map>
#include <vector>
#include "base.h"

#define COUNT_I 10
#define VDIM_SIZE 2

#define RUN_COLLECTIVE(start_cmd, name)                                    \
  do {                                                                     \
      t = 0;                                                               \
      for (iter_idx = 0; iter_idx < 1; iter_idx++)                         \
      {                                                                    \
          float* rcv_val = (float*)recv_vbuf;                              \
          for (idx = 0; idx < COUNT_I; idx++)                              \
          {                                                                \
            for (unsigned int jdx = 0; jdx < VDIM_SIZE; jdx++)             \
            {                                                              \
              send_vbuf[idx * VDIM_SIZE + jdx] = (float)(rank + 1 + jdx);  \
              rcv_val[idx * VDIM_SIZE + jdx] = 0.0;                        \
          }                                                                \
          }                                                                \
          t1 = when();                                                     \
          MLSL_CALL(start_cmd);                                            \
          MLSL_CALL(mlsl_wait(request));                                   \
          t2 = when();                                                     \
          t += (t2 - t1);                                                  \
          mlsl_barrier(NULL);                                              \
\
          int* rcv_idx = (int*)recv_ibuf;                                  \
          rcv_val = (float*)recv_vbuf;                                     \
\
          std::vector<float> vb(rcv_val, rcv_val + recv_vcount);           \
\
          std::unordered_map<int, std::vector<float> > expected = {{1,{9.0,14.0}},{2,{3.0,5.0}}, \
          {3,{2.0,4.0}},{4,{2.0,4.0}},{5,{5.0,8.0}},{6,{2.0,3.0}},{7,{1.0,2.0}}, \
          {8,{1.0,2.0}},{9,{5.0,8.0}}};                                    \
          for (idx = 0; idx < recv_icount; idx++)                          \
          {                                                                \
              auto key = expected.find(rcv_idx[idx]);                      \
              if (key == expected.end())                                   \
              {                                                            \
                  printf("iter %zu, idx %zu is not expected to be found\n",\
                          iter_idx, (size_t)rcv_idx[idx]);                 \
                  assert(0);                                               \
              }                                                            \
              else                                                         \
              {                                                            \
                  std::vector<float> v(vb.begin() + VDIM_SIZE * idx,       \
                  vb.begin() + VDIM_SIZE * (idx + 1));                     \
                  if (v != key->second)                                    \
                  {                                                        \
                    std::string str = "[";                                 \
                    for (auto x : key->second)                             \
                        str += std::to_string(x) + ",";                    \
                    str[str.length()-1] = ']';                             \
\
                    str += ", got [";                                      \
                    for (auto x : v)                                       \
                        str += std::to_string(x) + ",";                    \
                    str[str.length()-1] = ']';                             \
                    printf("iter %zu, idx %zu, expected %s\n",             \
                            iter_idx, (size_t)rcv_idx[idx], str.c_str());  \
                    assert(0);                                             \
                  }                                                        \
              }                                                            \
          }                                                                \
      }                                                                    \
      printf("[%zu] avg %s time: %8.2lf us\n", rank, name, t / 1);         \
      fflush(stdout);                                                      \
  } while (0)


int main()
{
    test_init();

    int send_ibuf[COUNT_I];
    float send_vbuf[COUNT_I * VDIM_SIZE];

    if (rank % 2)
    {
        auto init = { 1,5,2,9,6,1,5,9,1,1 };
        std::copy(init.begin(), init.end(), send_ibuf);
    }
    else
    {
        auto init = { 4,7,2,9,3,4,5,8,1,3 };
        std::copy(init.begin(), init.end(), send_ibuf);
    }

    void* recv_ibuf = malloc(COUNT_I * sizeof(int) + COUNT_I * VDIM_SIZE * sizeof(float));
    void* recv_vbuf = (char*)recv_ibuf + COUNT_I * sizeof(int);

    size_t recv_icount = 0;
    size_t recv_vcount = 0;

    coll_attr.to_cache = 0;
    RUN_COLLECTIVE(mlsl_sparse_allreduce(send_ibuf, COUNT_I, send_vbuf, COUNT_I * VDIM_SIZE, 
                                         &recv_ibuf, &recv_icount, &recv_vbuf, &recv_vcount, 
                                         mlsl_dtype_int, mlsl_dtype_float, mlsl_reduction_sum,
                                         &coll_attr, NULL, &request),
                   "basic_sparse_allreduce");

    free(recv_ibuf);

    test_finalize();
    return 0;
}
