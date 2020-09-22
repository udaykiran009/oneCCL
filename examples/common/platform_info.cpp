#include "base.hpp"

#ifdef MULTI_GPU_SUPPORT
#include "oneapi/ccl/native_device_api/export_api.hpp"
#endif

int main() {
    auto version = ccl::environment::instance().get_library_version();
    std::cout << "CCL library info:\nVersion:\n"
              << "major: " << version.major << "\nminor: " << version.minor
              << "\nupdate: " << version.update << "\nProduct: " << version.product_status
              << "\nBuild date: " << version.build_date << "\nFull: " << version.full << std::endl;
#ifdef MULTI_GPU_SUPPORT
    std::cout << "Compute runtime information:\n"
              << ::native::get_platform().to_string() << std::endl;
#endif
    std::cout << "Compute runtime information unavailable for current version" << std::endl;
}
