namespace native {
template struct memory<char, ccl_device, ccl_context>;
template struct memory<int, ccl_device, ccl_context>;
template struct memory<int64_t, ccl_device, ccl_context>;
template struct memory<uint64_t, ccl_device, ccl_context>;
template struct memory<float, ccl_device, ccl_context>;
template struct memory<double, ccl_device, ccl_context>;
} // namespace native
