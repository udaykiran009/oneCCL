namespace coll_attr_suite {
TEST(coll_attr, allgather_attr_creation) {
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr>();

    ASSERT_TRUE(attr.get<ccl::operation_attr_id::version>().full != nullptr);
}

TEST(coll_attr, allgather_attr_creation_match_id) {
    ccl_string match_id("aaaa");
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr>(
        ccl::attr_val<ccl::operation_attr_id::match_id>(match_id));

    ASSERT_TRUE(attr.get<ccl::operation_attr_id::version>().full != nullptr);
    ASSERT_EQ(attr.get<ccl::operation_attr_id::match_id>(), match_id);
}

TEST(coll_attr, allgather_creation_attr_with_common_attr) {
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr>(
        ccl::attr_val<ccl::operation_attr_id::priority>(10),
        ccl::attr_val<ccl::operation_attr_id::to_cache>(false));

    ASSERT_TRUE(attr.get<ccl::operation_attr_id::version>().full != nullptr);
    ASSERT_EQ(attr.get<ccl::operation_attr_id::priority>(), 10);
    ASSERT_EQ(attr.get<ccl::operation_attr_id::to_cache>(), false);
}

TEST(coll_attr, allgather_copy_on_write_attr) {
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr>(
        ccl::attr_val<ccl::operation_attr_id::priority>(10));

    auto original_inner_impl_ptr = attr.get_impl();

    ASSERT_EQ(attr.get<ccl::operation_attr_id::priority>(), 10);

    //set new val
    attr.set<ccl::operation_attr_id::priority>(11);
    ASSERT_EQ(attr.get<ccl::operation_attr_id::priority>(), 11);

    //make sure original impl is unchanged
    ASSERT_TRUE(original_inner_impl_ptr != attr.get_impl());
    ASSERT_EQ(std::static_pointer_cast<ccl::ccl_operation_attr_impl_t>(original_inner_impl_ptr)
                  ->get_attribute_value(
                      ccl::detail::ccl_api_type_attr_traits<ccl::operation_attr_id,
                                                             ccl::operation_attr_id::priority>{}),
              10);
}

TEST(coll_attr, allgather_copy_attr) {
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr>();

    auto original_inner_impl_ptr = attr.get_impl().get();
    auto attr2 = attr;
    auto copied_inner_impl_ptr = attr2.get_impl().get();
    ASSERT_TRUE(original_inner_impl_ptr != copied_inner_impl_ptr);
    ASSERT_TRUE(attr.get_impl());
}

TEST(coll_attr, allgather_move_attr) {
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr>();

    auto original_inner_impl_ptr = attr.get_impl().get();

    auto attr2 = (std::move(attr));
    auto moved_inner_impl_ptr = attr2.get_impl().get();
    ASSERT_EQ(original_inner_impl_ptr, moved_inner_impl_ptr);
    ASSERT_TRUE(not attr.get_impl());
}

TEST(coll_attr, allgather_creation_attr_setters) {
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr>(
        ccl::attr_val<ccl::operation_attr_id::priority>(10),
        ccl::attr_val<ccl::operation_attr_id::to_cache>(false));

    ASSERT_TRUE(attr.get<ccl::operation_attr_id::version>().full != nullptr);
    ASSERT_EQ(attr.get<ccl::operation_attr_id::priority>(), 10);
    ASSERT_EQ(attr.get<ccl::operation_attr_id::to_cache>(), false);

    attr.set<ccl::operation_attr_id::priority>(11);
    ASSERT_EQ(attr.get<ccl::operation_attr_id::priority>(), 11);

    attr.set<ccl::operation_attr_id::to_cache>(true);
    ASSERT_EQ(attr.get<ccl::operation_attr_id::to_cache>(), true);

    ccl::library_version new_ver;
    bool exception_throwed = false;
    try {
        attr.set<ccl::operation_attr_id::version>(new_ver);
    }
    catch (...) {
        exception_throwed = true;
    }
    ASSERT_EQ(exception_throwed, true);
}
} // namespace coll_attr_suite
