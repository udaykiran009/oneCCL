#include "base_fixture.hpp"

class allreduce_single_device_fixture : public common_fixture {
protected:
    allreduce_single_device_fixture() : common_fixture(get_global_device_indices() /*"[0:0]"*/) {}

    ~allreduce_single_device_fixture() {}

    void SetUp() override {
        create_global_platform();
        local_affinity = global_affinities.at(0);
        create_local_platform();
        create_module_descr("kernels/ring_allreduce.spv");
    }

    void TearDown() override {}
};

class gpu_aggregator_fixture : public common_fixture {
protected:
    gpu_aggregator_fixture() : common_fixture("") {}

    virtual ~gpu_aggregator_fixture() {}

    void create_global_platform() {
        // enumerates all available driver and all available devices(for foreign devices ipc handles recover)
        global_platform.reset(new native::ccl_device_platform());
    }

    virtual void SetUp() override {
        create_global_platform();
    }

    virtual void TearDown() override {}
};

#if 0
class communicator_fixture : public testing::Test, public tracer {
public:
    communicator_fixture() : global_comm(new ccl::communicator(ccl::preview::create_communicator())) {
        ccl::barrier(*global_comm);
    //communicator_fixture() : global_comm(ccl::details::environment::instance().create_communicator()) {
    //    global_comm->barrier();
    }
    ~communicator_fixture() override {}

    void initialize_global_mask(ccl::cluster_device_indices_t new_mask) {
        global_mask.swap(new_mask);
    }

    size_t get_fixture_rank() {
        return get_fixture_comm()->rank();
    }

    size_t get_fixture_size() {
        return get_fixture_comm()->size();
    }

    std::shared_ptr<ccl::communicator> get_fixture_comm() {
        return global_comm;
    }

    const ccl::cluster_device_indices_t& get_global_mask() const {
        return global_mask;
    }

    const ccl::device_indices_t& get_process_mask(const std::string& hostname,
                                                  size_t process_order) {
        auto node_it = global_mask.find(hostname);
        if (node_it == global_mask.end()) {
            std::cerr << __FUNCTION__ << "global_mask doesn't contain host info: " << hostname
                      << std::endl;
            abort();
        }

        const auto& proc_it = node_it->second.find(process_order);
        if (proc_it == node_it->second.end()) {
            std::cerr << __FUNCTION__ << "proc mask for host: " << hostname
                      << " doesn't contain proc id: " << process_order << std::endl;
            abort();
        }
        return proc_it->second;
    }
    ccl::cluster_device_indices_t global_mask;
    std::shared_ptr<ccl::communicator> global_comm;
};
#endif

class shared_context_fixture : public common_fixture {
protected:
    shared_context_fixture() : common_fixture(get_global_device_indices()) {}

    ~shared_context_fixture() {}

    void SetUp() override {
        create_global_platform();
        local_affinity = global_affinities.at(0);
        create_local_platform();
        create_module_descr("kernels/observer_event.spv");
    }

    void TearDown() override {}
};

