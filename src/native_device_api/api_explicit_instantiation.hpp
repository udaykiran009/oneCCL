namespace native
{
template struct memory<char, ccl_device>;
template struct memory<int, ccl_device>;
template struct memory<int64_t, ccl_device>;
template struct memory<uint64_t, ccl_device>;
template struct memory<float, ccl_device>;
template struct memory<double, ccl_device>;
}
