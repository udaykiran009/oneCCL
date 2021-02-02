namespace native_api_test {

TEST(NATIVE_API, platform_info) {
    const native::ccl_device_platform& pl = native::get_platform();

    const char* native_api_args = getenv("NATIVE_API_ARGS");
    if (native_api_args != NULL && strcmp(native_api_args, "dump_table") == 0) {
        size_t counter = 0;
        auto drivers_list = pl.get_drivers();
        std::cout << "Table" << std::endl;
        for (auto driver : drivers_list) {
            auto devices_list = driver.second->get_devices();
            for (auto device : devices_list) {
                if (counter != 0)
                    std::cout << ",";
                counter++;
                std::cout << ccl::to_string(device.second->get_device_path());
                auto subdevices_list = device.second->get_subdevices();
                for (auto subdevice : subdevices_list) {
                    if (counter != 0)
                        std::cout << ",";
                    std::cout << ccl::to_string(subdevice.second->get_device_path());
                }
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }
        std::cout << "EndTable" << std::endl;
        unsetenv("NATIVE_API_ARGS");
    }
    else {
#ifndef STANDALONE_UT
        ASSERT_EQ(pl.get_drivers().empty(), false) << "No L0 drivers available";
#endif
        std::cout << pl.to_string() << std::endl;
    }
}
} // namespace native_api_test
