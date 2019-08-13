#ifndef BASE_HPP
#define BASE_HPP

#include "ccl.hpp"

#include <functional>
#include <vector>
#include <chrono>

#define ITERS                (100)
#define MSG_SIZE_COUNT       (6)
#define START_MSG_SIZE_POWER (10)
#define COLL_ROOT            (0)

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

#define MSG_LOOP(per_msg_code)                                  \
  do                                                            \
  {                                                             \
      PRINT_BY_ROOT("iters=%d, msg_size_count=%d, "             \
                    "start_msg_size_power=%d, coll_root=%d",    \
                    ITERS, MSG_SIZE_COUNT,                      \
                    START_MSG_SIZE_POWER, COLL_ROOT);           \
      std::vector<size_t> msg_counts(MSG_SIZE_COUNT);           \
      std::vector<std::string> msg_match_ids(MSG_SIZE_COUNT);   \
      for (size_t idx = 0; idx < MSG_SIZE_COUNT; ++idx)         \
      {                                                         \
          msg_counts[idx] = 1u << (START_MSG_SIZE_POWER + idx); \
          msg_match_ids[idx] = std::to_string(msg_counts[idx]); \
      }                                                         \
      try                                                       \
      {                                                         \
          for (size_t idx = 0; idx < MSG_SIZE_COUNT; ++idx)     \
          {                                                     \
              size_t msg_count = msg_counts[idx];               \
              coll_attr.match_id = msg_match_ids[idx].c_str();  \
              PRINT_BY_ROOT("msg_count=%zu, match_id=%s",       \
                            msg_count, coll_attr.match_id);     \
              per_msg_code;                                     \
          }                                                     \
      }                                                         \
      catch (ccl::ccl_error& e)                                 \
      {                                                         \
          printf("FAILED\n");                                   \
          fprintf(stderr, "ccl exception:\n%s\n", e.what());    \
      }                                                         \
      catch (...)                                               \
      {                                                         \
          printf("FAILED\n");                                   \
          fprintf(stderr, "other exception\n");                 \
      }                                                         \
      PRINT_BY_ROOT("PASSED");                                  \
  } while (0)

#endif /* BASE_HPP */
