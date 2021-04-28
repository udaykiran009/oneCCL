namespace native_api_test {

struct mock_device : public native::ccl_device {
    mock_device(const native::ccl_device& origin)
            : native::ccl_device(
                  origin.get(),
                  const_cast<native::ccl_device::base::owner_ptr_t&&>(origin.get_owner()),
                  const_cast<native::ccl_device::base::context_ptr_t&&>(origin.get_ctx()))

    {}

    ~mock_device() {
        native::ccl_device::handle = nullptr;
        native::ccl_device::owner.reset();
        native::ccl_device::context.reset();
    }
};

bool operator==(const ze_event_pool_desc_t lhs, const ze_event_pool_desc_t& rhs) {
    return (lhs.stype == rhs.stype) and (lhs.pNext == rhs.pNext) and (lhs.flags == rhs.flags) and
           (lhs.count == rhs.count);
}

TEST(NATIVE_API, event_pool_simple_creation) {
    using namespace native;

    const ccl_device_platform& pl = get_platform();
    std::shared_ptr<ccl_context> ctx = pl.get_driver(0)->create_context();

    ze_event_pool_desc_t pool_descr{
        ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,
        nullptr,
        ZE_EVENT_POOL_FLAG_HOST_VISIBLE, // all events in pool are visible to Host
        1 // count
    };

    std::shared_ptr<ccl_event_pool> pool = ctx->create_event_pool({}, pool_descr);
    UT_ASSERT_GLOBAL(pool->get_pool_description() == pool_descr,
                     "Invalid pool description after pool creation");
    UT_ASSERT_GLOBAL(pool->get_allocated_events() == 0, "Just created pool has no allocated event");
    {
        auto pools = ctx->get_shared_event_pool();
        UT_ASSERT_GLOBAL(pools.size() == 1, "Expected only one shared pool");
        UT_ASSERT_GLOBAL(pools[0] == pool, "Shared pools are different");
    }
}

TEST(NATIVE_API, event_pool_simple_target_creation) {
    using namespace native;

    const ccl_device_platform& pl = get_platform();
    std::shared_ptr<ccl_device_driver> driver = pl.get_driver(0);
    UT_ASSERT_GLOBAL(driver, "Expected driver by index 0");

    auto devices = driver->get_devices();
    UT_ASSERT_GLOBAL(!devices.empty(), "Expected at least 1 device in driver");

    std::shared_ptr<ccl_device> real_device = devices.begin()->second;
    std::shared_ptr<ccl_context> ctx = driver->create_context();

    ze_event_pool_desc_t pool_descr{
        ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,
        nullptr,
        ZE_EVENT_POOL_FLAG_HOST_VISIBLE, // all events in pool are visible to Host
        1 // count
    };

    mock_device device(*real_device);
    std::shared_ptr<ccl_event_pool> pool = ctx->create_event_pool({ &device }, pool_descr);
    UT_ASSERT_GLOBAL(pool->get_pool_description() == pool_descr,
                     "Invalid pool description after pool creation");
    UT_ASSERT_GLOBAL(pool->get_allocated_events() == 0, "Just created pool has no allocated event");

    {
        auto pools = ctx->get_shared_event_pool({ &device });
        UT_ASSERT_GLOBAL(pools.size() == 1, "Expected only one resident pool");
        UT_ASSERT_GLOBAL(pools[0] == pool, "Shared pools are different");
    }
}

TEST(NATIVE_API, event_pool_multiple_target_creation) {
    using namespace native;

    const ccl_device_platform& pl = get_platform();
    std::shared_ptr<ccl_device_driver> driver = pl.get_driver(0);
    UT_ASSERT_GLOBAL(driver, "Expected driver by index 0");

    auto devices = driver->get_devices();
    UT_ASSERT_GLOBAL(!devices.empty(), "Expected at least 1 device in driver");

    std::shared_ptr<ccl_device> real_device = devices.begin()->second;
    std::shared_ptr<ccl_context> ctx = driver->create_context();

    ze_event_pool_desc_t pool_descr{
        ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,
        nullptr,
        ZE_EVENT_POOL_FLAG_HOST_VISIBLE, // all events in pool are visible to Host
        1 // count
    };

    std::shared_ptr<ccl_event_pool> shared_pool = ctx->create_event_pool({}, pool_descr);
    UT_ASSERT_GLOBAL(shared_pool->get_pool_description() == pool_descr,
                     "Invalid pool description after shared_pool creation");
    UT_ASSERT_GLOBAL(shared_pool->get_allocated_events() == 0,
                     "Just created shared_pool has no allocated event");
    {
        auto pools = ctx->get_shared_event_pool();
        UT_ASSERT_GLOBAL(pools.size() == 1, "Expected only one shared pool");
        UT_ASSERT_GLOBAL(pools[0] == shared_pool, "Shared pools are different");
    }

    mock_device device_1(*real_device);
    std::shared_ptr<ccl_event_pool> dev_1_pool = ctx->create_event_pool({ &device_1 }, pool_descr);
    UT_ASSERT_GLOBAL(dev_1_pool->get_pool_description() == pool_descr,
                     "Invalid pool description after dev_1_pool creation");
    UT_ASSERT_GLOBAL(dev_1_pool->get_allocated_events() == 0,
                     "Just created dev_1_pool has no allocated event");
    {
        auto pools = ctx->get_shared_event_pool();
        UT_ASSERT_GLOBAL(pools.size() == 1, "Expected only one shared pool");
        UT_ASSERT_GLOBAL(pools[0] == shared_pool, "Shared pools are different");

        auto pools_1 = ctx->get_shared_event_pool({ &device_1 });
        UT_ASSERT_GLOBAL(pools_1.size() == 1, "Expected only one resident pool for device_1");
        UT_ASSERT_GLOBAL(pools_1[0] == dev_1_pool, "resident pools for device_1 are different");
    }

    mock_device device_2(*real_device);
    std::shared_ptr<ccl_event_pool> dev_2_pool = ctx->create_event_pool({ &device_2 }, pool_descr);
    UT_ASSERT_GLOBAL(dev_2_pool->get_pool_description() == pool_descr,
                     "Invalid pool description after dev_2_pool creation");
    UT_ASSERT_GLOBAL(dev_2_pool->get_allocated_events() == 0,
                     "Just created dev_2_pool has no allocated event");
    {
        auto pools = ctx->get_shared_event_pool();
        UT_ASSERT_GLOBAL(pools.size() == 1, "Expected only one shared pool");
        UT_ASSERT_GLOBAL(pools[0] == shared_pool, "Shared pools are different");

        auto pools_2 = ctx->get_shared_event_pool({ &device_2 });
        UT_ASSERT_GLOBAL(pools_2.size() == 1, "Expected only one resident pool for device_2");
        UT_ASSERT_GLOBAL(pools_2[0] == dev_2_pool, "resident pools for device_2 are different");
    }
    mock_device device_3(*real_device);
    std::shared_ptr<ccl_event_pool> dev_3_pool = ctx->create_event_pool({ &device_3 }, pool_descr);
    UT_ASSERT_GLOBAL(dev_3_pool->get_pool_description() == pool_descr,
                     "Invalid pool description after dev_3_pool creation");
    UT_ASSERT_GLOBAL(dev_3_pool->get_allocated_events() == 0,
                     "Just created dev_3_pool has no allocated event");
    {
        auto pools = ctx->get_shared_event_pool();
        UT_ASSERT_GLOBAL(pools.size() == 1, "Expected only one shared pool");
        UT_ASSERT_GLOBAL(pools[0] == shared_pool, "Shared pools are different");

        auto pools_3 = ctx->get_shared_event_pool({ &device_3 });
        UT_ASSERT_GLOBAL(pools_3.size() == 1, "Expected only one resident pool for device_3");
        UT_ASSERT_GLOBAL(pools_3[0] == dev_3_pool, "resident pools for device_3 are different");
    }
}

TEST(NATIVE_API, event_pool_multiple_shared_target_creation) {
    using namespace native;

    const ccl_device_platform& pl = get_platform();
    std::shared_ptr<ccl_device_driver> driver = pl.get_driver(0);
    UT_ASSERT_GLOBAL(driver, "Expected driver by index 0");

    auto devices = driver->get_devices();
    UT_ASSERT_GLOBAL(!devices.empty(), "Expected at least 1 device in driver");

    std::shared_ptr<ccl_device> real_device = devices.begin()->second;
    std::shared_ptr<ccl_context> ctx = driver->create_context();

    ze_event_pool_desc_t pool_descr{
        ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,
        nullptr,
        ZE_EVENT_POOL_FLAG_HOST_VISIBLE, // all events in pool are visible to Host
        1 // count
    };

    std::shared_ptr<ccl_event_pool> shared_pool = ctx->create_event_pool({}, pool_descr);
    {
        auto pools = ctx->get_shared_event_pool();
        UT_ASSERT_GLOBAL(pools.size() == 1, "Expected only one shared pool");
        UT_ASSERT_GLOBAL(pools[0] == shared_pool, "Shared pools are different");
    }

    mock_device device_1(*real_device);
    mock_device device_2(*real_device);
    mock_device device_3(*real_device);
    std::shared_ptr<ccl_event_pool> dev_pool =
        ctx->create_event_pool({ &device_1, &device_2, &device_3 }, pool_descr);
    {
        auto pools = ctx->get_shared_event_pool();
        UT_ASSERT_GLOBAL(pools.size() == 1, "Expected only one shared pool");
        UT_ASSERT_GLOBAL(pools[0] == shared_pool, "Shared pools are different");

        auto pools_1 = ctx->get_shared_event_pool({ &device_1 });
        UT_ASSERT_GLOBAL(pools_1.size() == 1, "Expected only one shared pool for device_1");
        UT_ASSERT_GLOBAL(pools_1[0] == dev_pool, "resident pools for device_1 are different");

        auto pools_2 = ctx->get_shared_event_pool({ &device_2 });
        UT_ASSERT_GLOBAL(pools_2.size() == 1, "Expected only one shared pool for device_2");
        UT_ASSERT_GLOBAL(pools_2[0] == dev_pool, "resident pools for device_2 are different");

        auto pools_3 = ctx->get_shared_event_pool({ &device_3 });
        UT_ASSERT_GLOBAL(pools_3.size() == 1, "Expected only one shared pool for device_3");
        UT_ASSERT_GLOBAL(pools_3[0] == dev_pool, "resident pools for device_3 are different");
    }
}
} // namespace native_api_test
