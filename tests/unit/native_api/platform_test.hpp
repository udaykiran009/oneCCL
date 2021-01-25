namespace native_api_test {

TEST(NATIVE_API, platform_info) {
    const native::ccl_device_platform& pl = native::get_platform();

#ifndef STANDALONE_UT
    ASSERT_EQ(pl.get_drivers().empty(), false) << "No L0 drivers available";
#endif
    std::cout << pl.to_string() << std::endl;
}
} // namespace native_api_test
