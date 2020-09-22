#include <iterator>
#include <sstream>
#include <unistd.h>

#include "common/env/env.hpp"
#include "common/global/global.hpp"
#include "common/log/log.hpp"

namespace ccl {

env_data::env_data() : log_level(static_cast<int>(ccl_log_level::DEBUG)) {}

} /* namespace ccl */
