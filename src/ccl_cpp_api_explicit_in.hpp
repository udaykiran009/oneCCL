#ifndef COMMA
#define COMMA ,
#endif

//TODO
#if 0
/**
 * Attributes
 */
HOST_ATTRIBUTE_INSTANTIATION(ccl_host_color,
                               typename ccl::comm_split_attr_id_traits<ccl_host_color>::type);
HOST_ATTRIBUTE_INSTANTIATION(ccl_host_version,
                               typename ccl::comm_split_attr_id_traits<ccl_host_version>::type);

API_COLL_EXPLICIT_INSTANTIATION(char);
API_COLL_EXPLICIT_INSTANTIATION(int);
API_COLL_EXPLICIT_INSTANTIATION(int64_t);
API_COLL_EXPLICIT_INSTANTIATION(uint64_t);
API_COLL_EXPLICIT_INSTANTIATION(float);
API_COLL_EXPLICIT_INSTANTIATION(double);

#ifdef CCL_ENABLE_SYCL
    API_COLL_EXPLICIT_CLASS_INSTANTIATION(cl::sycl::buffer<char COMMA 1>);
    API_COLL_EXPLICIT_CLASS_INSTANTIATION(cl::sycl::buffer<int COMMA 1>);
    API_COLL_EXPLICIT_CLASS_INSTANTIATION(cl::sycl::buffer<int64_t COMMA 1>);
    API_COLL_EXPLICIT_CLASS_INSTANTIATION(cl::sycl::buffer<uint64_t COMMA 1>);
    API_COLL_EXPLICIT_CLASS_INSTANTIATION(cl::sycl::buffer<float COMMA 1>);
    API_COLL_EXPLICIT_CLASS_INSTANTIATION(cl::sycl::buffer<double COMMA 1>);
#endif //CCL_ENABLE_SYCL

API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(char, char);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(char, int);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(char, ccl::bfp16);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(char, float);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(char, double);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(char, int64_t);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(char, uint64_t);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(int, char);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(int, int);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(int, ccl::bfp16);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(int, float);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(int, double);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(int, int64_t);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(int, uint64_t);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(int64_t, char);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(int64_t, int);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(int64_t, ccl::bfp16);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(int64_t, float);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(int64_t, double);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(int64_t, int64_t);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(int64_t, uint64_t);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(uint64_t, char);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(uint64_t, int);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(uint64_t, ccl::bfp16);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(uint64_t, float);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(uint64_t, double);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(uint64_t, int64_t);
API_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(uint64_t, uint64_t);

#ifdef CCL_ENABLE_SYCL
    API_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(cl::sycl::buffer<int COMMA 1>,
                                                      cl::sycl::buffer<float COMMA 1>);
    API_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(cl::sycl::buffer<int COMMA 1>,
                                                      cl::sycl::buffer<ccl::bfp16 COMMA 1>);

    API_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(cl::sycl::buffer<int64_t COMMA 1>,
                                                      cl::sycl::buffer<float COMMA 1>);
    API_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(cl::sycl::buffer<int64_t COMMA 1>,
                                                      cl::sycl::buffer<ccl::bfp16 COMMA 1>);
#endif //CCL_ENABLE_SYCL
#undef COMMA

#endif //TODO
