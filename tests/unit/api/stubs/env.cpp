#include <iterator>
#include <sstream>
#include <unistd.h>

#include "common/env/env.hpp"
#include "common/global/global.hpp"
#include "common/log/log.hpp"

namespace ccl {

env_data::env_data() : log_level(ccl_log_level::debug) {}
void env_data::print(int rank) {}
} /* namespace ccl */
