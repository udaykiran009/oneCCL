namespace coll_attr_suite
{
TEST(coll_attr, allgather_attr_creation)
{
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr_t>(
                                                ccl::attr_arg<ccl::allgatherv_op_attr_id::vector_buf>(666));

    ASSERT_TRUE(attr.get_value<ccl::common_op_attr_id::version>().full != nullptr);
    ASSERT_EQ(attr.get_value<ccl::allgatherv_op_attr_id::vector_buf>(), 666);
}

TEST(coll_attr, allgather_creation_attr_with_common_attr)
{
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr_t>(
                                                    ccl::attr_arg<ccl::allgatherv_op_attr_id::vector_buf>(666),
                                                    ccl::attr_arg<ccl::common_op_attr_id::priority>(10),
                                                    ccl::attr_arg<ccl::common_op_attr_id::to_cache>(0));

    ASSERT_TRUE(attr.get_value<ccl::common_op_attr_id::version>().full != nullptr);
    ASSERT_EQ(attr.get_value<ccl::allgatherv_op_attr_id::vector_buf>(), 666);
    ASSERT_EQ(attr.get_value<ccl::common_op_attr_id::priority>(), 10);
    ASSERT_EQ(attr.get_value<ccl::common_op_attr_id::to_cache>(), 0);
}

TEST(coll_attr, allgather_copy_on_write_attr)
{
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr_t>(
                                                ccl::attr_arg<ccl::common_op_attr_id::priority>(10));

    auto original_inner_impl_ptr = attr.get_impl();

    ASSERT_EQ(attr.get_value<ccl::common_op_attr_id::priority>(), 10);

    //set new val
    attr.set_value<ccl::common_op_attr_id::priority>(11);
    ASSERT_EQ(attr.get_value<ccl::common_op_attr_id::priority>(), 11);

    //make sure original impl is unchanged
    ASSERT_TRUE(original_inner_impl_ptr !=attr.get_impl());
    ASSERT_EQ(std::static_pointer_cast<ccl::ccl_common_op_attr_impl_t>(original_inner_impl_ptr)->get_attribute_value(
    ccl::details::ccl_api_type_attr_traits<ccl::common_op_attr_id, ccl::common_op_attr_id::priority> {}), 10);


}

TEST(coll_attr, allgather_copy_attr)
{
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr_t>(
                                                ccl::attr_arg<ccl::allgatherv_op_attr_id::vector_buf>(666));

    auto original_inner_impl_ptr = attr.get_impl().get();
    auto attr2 = attr;
    auto copied_inner_impl_ptr = attr2.get_impl().get();
    ASSERT_TRUE(original_inner_impl_ptr != copied_inner_impl_ptr);
    ASSERT_TRUE(attr.get_impl());

}

TEST(coll_attr, allgather_move_attr)
{
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr_t>(
                                                ccl::attr_arg<ccl::allgatherv_op_attr_id::vector_buf>(666));

    auto original_inner_impl_ptr = attr.get_impl().get();

    auto attr2 = (std::move(attr));
    auto moved_inner_impl_ptr = attr2.get_impl().get();
    ASSERT_EQ(original_inner_impl_ptr, moved_inner_impl_ptr);
    ASSERT_TRUE(not attr.get_impl());
}

TEST(coll_attr, allgather_creation_attr_setters)
{
    auto attr = ccl::create_coll_attr<ccl::allgatherv_attr_t>(
                                                    ccl::attr_arg<ccl::allgatherv_op_attr_id::vector_buf>(666),
                                                    ccl::attr_arg<ccl::common_op_attr_id::priority>(10),
                                                    ccl::attr_arg<ccl::common_op_attr_id::to_cache>(0));

    ASSERT_TRUE(attr.get_value<ccl::common_op_attr_id::version>().full != nullptr);
    ASSERT_EQ(attr.get_value<ccl::allgatherv_op_attr_id::vector_buf>(), 666);
    ASSERT_EQ(attr.get_value<ccl::common_op_attr_id::priority>(), 10);
    ASSERT_EQ(attr.get_value<ccl::common_op_attr_id::to_cache>(), 0);

    attr.set_value<ccl::allgatherv_op_attr_id::vector_buf>(999);
    ASSERT_EQ(attr.get_value<ccl::allgatherv_op_attr_id::vector_buf>(), 999);

    attr.set_value<ccl::common_op_attr_id::priority>(11);
    ASSERT_EQ(attr.get_value<ccl::common_op_attr_id::priority>(), 11);

    attr.set_value<ccl::common_op_attr_id::to_cache>(1);
    ASSERT_EQ(attr.get_value<ccl::common_op_attr_id::to_cache>(), 1);

    ccl_version_t new_ver;
    bool exception_throwed = false;
    try
    {
        attr.set_value<ccl::common_op_attr_id::version>(new_ver);
    }
    catch(...)
    {
        exception_throwed = true;
    }
    ASSERT_EQ(exception_throwed, true);
}
}
