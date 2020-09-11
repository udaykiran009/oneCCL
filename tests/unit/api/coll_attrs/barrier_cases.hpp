namespace coll_attr_suite {
TEST(coll_attr, barrier_attr_creation) {
    auto attr = ccl::create_coll_attr<ccl::barrier_attr>();

    ASSERT_TRUE(attr.get<ccl::operation_attr_id::version>().full != nullptr);
}

TEST(coll_attr, barrier_copy_attr) {
    auto attr = ccl::create_coll_attr<ccl::barrier_attr>();

    auto original_inner_impl_ptr = attr.get_impl().get();
    auto attr2 = attr;
    auto copied_inner_impl_ptr = attr2.get_impl().get();
    ASSERT_TRUE(original_inner_impl_ptr != copied_inner_impl_ptr);
    ASSERT_TRUE(attr.get_impl());
}

TEST(coll_attr, barrier_move_attr) {
    auto attr = ccl::create_coll_attr<ccl::barrier_attr>();

    auto original_inner_impl_ptr = attr.get_impl().get();

    auto attr2 = (std::move(attr));
    auto moved_inner_impl_ptr = attr2.get_impl().get();
    ASSERT_EQ(original_inner_impl_ptr, moved_inner_impl_ptr);
    ASSERT_TRUE(not attr.get_impl());
}
} // namespace coll_attr_suite
