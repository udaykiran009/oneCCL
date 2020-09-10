namespace coll_attr_suite
{
TEST(coll_attr, sparse_allreduce_attr_creation)
{
    const void* fn_ctx = nullptr;
    auto attr = ccl::create_coll_attr<ccl::sparse_allreduce_attr>(
                                                ccl::attr_arg<ccl::sparse_allreduce_attr_id::fn_ctx>(fn_ctx));

    ASSERT_TRUE(attr.get<ccl::operation_attr_id::version>().full != nullptr);
    ASSERT_EQ(attr.get<ccl::sparse_allreduce_attr_id::fn_ctx>(), fn_ctx);
}

}
