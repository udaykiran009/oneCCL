namespace coll_attr_suite
{

ccl_status_t stub_reduction(
    const void*,
    size_t,
    void*,
    size_t*,
    ccl_datatype_t,
    const ccl_fn_context_t*)
{
     return ccl_status_success;
    }


TEST(coll_attr, allreduce_attr_creation)
{
    ccl_reduction_fn_t function {nullptr};

    auto attr = ccl::create_coll_attr<ccl::allreduce_attr_t>(
                                                ccl::attr_arg<ccl::allreduce_op_attr_id::reduction_fn>(function));

    ASSERT_TRUE(attr.get<ccl::common_op_attr_id::version>().full != nullptr);
    ASSERT_EQ(attr.get<ccl::allreduce_op_attr_id::reduction_fn>().get(), function);
}

TEST(coll_attr, allreduce_copy_on_write_attr)
{
    ccl_reduction_fn_t function {nullptr};

    auto attr = ccl::create_coll_attr<ccl::allreduce_attr_t>(
                                                ccl::attr_arg<ccl::allreduce_op_attr_id::reduction_fn>(function));

    auto original_inner_impl_ptr = attr.get_impl();

    ASSERT_EQ(attr.get<ccl::allreduce_op_attr_id::reduction_fn>().get(), function);

    //set new val
    {
        ccl::details::function_holder<ccl_reduction_fn_t> check_val{stub_reduction};
        attr.set<ccl::allreduce_op_attr_id::reduction_fn>((ccl_reduction_fn_t)stub_reduction);
        ASSERT_EQ(attr.get<ccl::allreduce_op_attr_id::reduction_fn>().get(), check_val.get());
    }

    //make sure original impl is unchanged
    ASSERT_TRUE(original_inner_impl_ptr != attr.get_impl());
    ASSERT_EQ(original_inner_impl_ptr->get_attribute_value(
    ccl::details::ccl_api_type_attr_traits<ccl::allreduce_op_attr_id, ccl::allreduce_op_attr_id::reduction_fn> {}).get(), function);
}

TEST(coll_attr, allreduce_copy_attr)
{
    auto attr = ccl::create_coll_attr<ccl::allreduce_attr_t>(
                                                ccl::attr_arg<ccl::allreduce_op_attr_id::reduction_fn>(stub_reduction));

    auto original_inner_impl_ptr = attr.get_impl().get();
    auto attr2 = attr;
    auto copied_inner_impl_ptr = attr2.get_impl().get();
    ASSERT_TRUE(original_inner_impl_ptr != copied_inner_impl_ptr);
    ASSERT_TRUE(attr.get_impl());

}

TEST(coll_attr, allreduce_move_attr)
{
    auto attr = ccl::create_coll_attr<ccl::allreduce_attr_t>(
                                                ccl::attr_arg<ccl::allreduce_op_attr_id::reduction_fn>(stub_reduction));

    auto original_inner_impl_ptr = attr.get_impl().get();

    auto attr2 = (std::move(attr));
    auto moved_inner_impl_ptr = attr2.get_impl().get();
    ASSERT_EQ(original_inner_impl_ptr, moved_inner_impl_ptr);
    ASSERT_TRUE(not attr.get_impl());
}
}
