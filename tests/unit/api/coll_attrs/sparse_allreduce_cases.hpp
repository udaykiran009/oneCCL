namespace coll_attr_suite {
TEST(coll_attr, sparse_allreduce_attr_creation) {
    const void* fn_ctx = nullptr;
    auto attr = ccl::v1::create_coll_attr<ccl::v1::sparse_allreduce_attr>(
        ccl::v1::attr_val<ccl::v1::sparse_allreduce_attr_id::fn_ctx>(fn_ctx));

    ASSERT_TRUE(attr.get<ccl::v1::operation_attr_id::version>().full != nullptr);
    ASSERT_EQ(attr.get<ccl::v1::sparse_allreduce_attr_id::fn_ctx>(), fn_ctx);
}

} // namespace coll_attr_suite
