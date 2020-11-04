namespace native {
template struct memory<int8_t, ccl_device, ccl_context>;
template struct memory<uint8_t, ccl_device, ccl_context>;
template struct memory<int16_t, ccl_device, ccl_context>;
template struct memory<uint16_t, ccl_device, ccl_context>;
template struct memory<int32_t, ccl_device, ccl_context>;
template struct memory<uint32_t, ccl_device, ccl_context>;
template struct memory<int64_t, ccl_device, ccl_context>;
template struct memory<uint64_t, ccl_device, ccl_context>;
template struct memory<float, ccl_device, ccl_context>;
template struct memory<double, ccl_device, ccl_context>;
} // namespace native
