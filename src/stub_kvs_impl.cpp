#ifdef CCL_ENABLE_STUB_BACKEND

#include "stub_kvs_impl.hpp"

namespace ccl {

static int get_unique_id() {
    const char* env_val = getenv("CCL_STUB_BACKEND_COMM_ID");
    int id = 0;
    if (env_val != nullptr)
        id = atoi(env_val);

    return id;
}

static kvs::address_type convert_id_to_addr(int id) {
    kvs::address_type addr;

    memset(addr.data(), 0, sizeof(addr));
    memcpy(addr.data(), &id, sizeof(id));

    return addr;
}

static int convert_addr_to_id(const kvs::address_type& addr) {
    int id = 0;
    memcpy(&id, addr.data(), sizeof(id));

    return id;
}

stub_kvs_impl::stub_kvs_impl() : base_kvs_impl(convert_id_to_addr(get_unique_id())) {
    CCL_THROW_IF_NOT(ccl::global_data::env().backend == backend_mode::stub, "unexpected backend");
}

stub_kvs_impl::stub_kvs_impl(const kvs::address_type& addr) : base_kvs_impl(addr) {
    CCL_THROW_IF_NOT(ccl::global_data::env().backend == backend_mode::stub, "unexpected backend");
}

int stub_kvs_impl::get_id() const {
    return convert_addr_to_id(base_kvs_impl::get_addr());
}

} // namespace ccl

#endif // CCL_ENABLE_STUB_BACKEND
