#ifndef BASE_HPP
#define BASE_HPP

#include "ccl.hpp"

#include <functional>
#include <vector>
#include <chrono>

#define COUNT     (1048576 / 256)
#define ITERS     10
#define COLL_ROOT 0

#define START_MSG_SIZE_POWER 10
#define MSG_SIZE_COUNT 11


#define PRINT_BY_ROOT(fmt, ...)             \
    if (comm.rank() == 0) {                 \
        printf(fmt"\n", ##__VA_ARGS__); }   \

#define ASSERT(cond, fmt, ...)                            \
  do                                                      \
  {                                                       \
      if (!(cond))                                        \
      {                                                   \
          printf("FAILED\n");                             \
          fprintf(stderr, "ASSERT '%s' FAILED " fmt "\n", \
              #cond, ##__VA_ARGS__);                      \
          test_finalize();                                \
          exit(1);                                        \
      }                                                   \
  } while (0)


#endif /* BASE_HPP */

ccl::environment env;
ccl::communicator comm;
ccl::stream stream;
