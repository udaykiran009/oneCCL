#include "common/global/global.hpp"

#include <new>

mlsl_global_data global_data{};
thread_local bool mlsl_global_data::is_worker_thread = false;

