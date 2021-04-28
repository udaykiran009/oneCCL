namespace native_api_test {

TEST(NATIVE_API, event_simple_creation) {
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
    UT_ASSERT_GLOBAL(pool->get_allocated_events() == 0, "Just created pool has no allocated event");
    auto dup_pool = pool->get_ptr();
    UT_ASSERT_GLOBAL(dup_pool == pool, "Same pool");

    std::shared_ptr<event> e = pool->create_event();
    UT_ASSERT_GLOBAL(e, "Event must be created");
    UT_ASSERT_GLOBAL(pool->get_allocated_events() == 1,
                     "Invalid alloacted count after first event allocation");

    e->signal();
    UT_ASSERT_GLOBAL(e->status() == ZE_RESULT_SUCCESS,
                     "status() must be  successed after event signal() ");
    UT_ASSERT_GLOBAL(e->wait() == true,
                     "wait() must be successed after event status() and signal() ");

    bool passed = false;
    try {
        std::shared_ptr<event> e_non_created = pool->create_event();
    }
    catch (const std::exception& ex) {
        passed = true;
    }

    UT_ASSERT_GLOBAL(passed, "Non expected duplicated event creation is not possible")

    e.reset();
    UT_ASSERT_GLOBAL(pool->get_allocated_events() == 0,
                     "Invalid allocated count after event destruction ");
}

TEST(NATIVE_API, event_host_operation) {
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
    std::shared_ptr<event> e = pool->create_event();
    UT_ASSERT_GLOBAL(e, "Event must be created");

    UT_ASSERT_GLOBAL(e->status() == ZE_RESULT_NOT_READY,
                     "status() must be ZE_RESULT_NOT_READY on created  event");
    UT_ASSERT_GLOBAL(e->wait(1000) == false,
                     "wait() must return timeout after 1000 nanosec on created event");

    e->signal();
    UT_ASSERT_GLOBAL(e->wait(1000) == true, "wait() must success after signal()");
    UT_ASSERT_GLOBAL(e->status() == ZE_RESULT_SUCCESS,
                     "status() must be ZE_RESULT_SUCCESS  after signal()");

    ze_result_t res = zeEventHostReset(e->get());
    UT_ASSERT_GLOBAL(res == ZE_RESULT_SUCCESS, "Reset event from host failed");
    UT_ASSERT_GLOBAL(e->status() == ZE_RESULT_NOT_READY,
                     "status() must be ZE_RESULT_NOT_READY after event reset");
    UT_ASSERT_GLOBAL(e->wait(1000) == false,
                     "wait() must return timeout after 1000 nanosec after event reset");

    e->signal();
    UT_ASSERT_GLOBAL(e->wait(1000) == true, "wait() must success after signal() on resetted event");
    UT_ASSERT_GLOBAL(e->status() == ZE_RESULT_SUCCESS,
                     "status() must be ZE_RESULT_SUCCESS after signal() on resetted event");
}
} // namespace native_api_test
