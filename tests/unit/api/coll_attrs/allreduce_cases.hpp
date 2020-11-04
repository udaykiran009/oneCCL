namespace coll_attr_suite {

void stub_reduction(const void*, size_t, void*, size_t*, ccl::datatype, const ccl::fn_context*) {}

TEST(coll_attr, allreduce_attr_creation) {
    ccl::reduction_fn function{ nullptr };

    auto attr = ccl::v1::create_coll_attr<ccl::v1::allreduce_attr>(
        ccl::v1::attr_val<ccl::v1::allreduce_attr_id::reduction_fn>(function));

    ASSERT_TRUE(attr.get<ccl::v1::operation_attr_id::version>().full != nullptr);
    ASSERT_EQ(attr.get<ccl::v1::allreduce_attr_id::reduction_fn>().get(), function);
}

TEST(coll_attr, allreduce_copy_on_write_attr) {
    ccl::reduction_fn function{ nullptr };

    auto attr = ccl::v1::create_coll_attr<ccl::v1::allreduce_attr>(
        ccl::v1::attr_val<ccl::v1::allreduce_attr_id::reduction_fn>(function));

    auto original_inner_impl_ptr = attr.get_impl();

    ASSERT_EQ(attr.get<ccl::v1::allreduce_attr_id::reduction_fn>().get(), function);

    //set new val
    {
        ccl::detail::function_holder<ccl::reduction_fn> check_val{ stub_reduction };
        attr.set<ccl::v1::allreduce_attr_id::reduction_fn>((ccl::reduction_fn)stub_reduction);
        ASSERT_EQ(attr.get<ccl::v1::allreduce_attr_id::reduction_fn>().get(), check_val.get());
    }

    //make sure original impl is unchanged
    ASSERT_TRUE(original_inner_impl_ptr != attr.get_impl());
    ASSERT_EQ(
        original_inner_impl_ptr
            ->get_attribute_value(
                ccl::detail::ccl_api_type_attr_traits<ccl::v1::allreduce_attr_id,
                                                      ccl::v1::allreduce_attr_id::reduction_fn>{})
            .get(),
        function);
}

TEST(coll_attr, allreduce_copy_attr) {
    auto attr = ccl::v1::create_coll_attr<ccl::v1::allreduce_attr>(
        ccl::v1::attr_val<ccl::v1::allreduce_attr_id::reduction_fn>(stub_reduction));

    auto original_inner_impl_ptr = attr.get_impl().get();
    auto attr2 = attr;
    auto copied_inner_impl_ptr = attr2.get_impl().get();
    ASSERT_TRUE(original_inner_impl_ptr != copied_inner_impl_ptr);
    ASSERT_TRUE(attr.get_impl());
}

TEST(coll_attr, allreduce_move_attr) {
    auto attr = ccl::v1::create_coll_attr<ccl::v1::allreduce_attr>(
        ccl::v1::attr_val<ccl::v1::allreduce_attr_id::reduction_fn>(stub_reduction));

    auto original_inner_impl_ptr = attr.get_impl().get();

    auto attr2 = (std::move(attr));
    auto moved_inner_impl_ptr = attr2.get_impl().get();
    ASSERT_EQ(original_inner_impl_ptr, moved_inner_impl_ptr);
    ASSERT_TRUE(not attr.get_impl());
}
} // namespace coll_attr_suite
