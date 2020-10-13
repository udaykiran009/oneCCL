#pragma once
#include "../topology_utils.hpp"
#include "common/comm/l0/topology/topology_serializer.hpp"

namespace topology_suite {
TEST(INDICES_SERIALIZE, indices_serialize_deserialize_vector_test) {
    using namespace native::detail;
    using namespace native::detail::serialize;

    using tested_container_t = std::vector<ccl::device_index_type>;

    tested_container_t before{ ccl::device_index_type(0, 1, ccl::unused_index_value),
                               ccl::device_index_type(0, 0, ccl::unused_index_value),
                               ccl::device_index_type(0, 2, ccl::unused_index_value) };

    device_path_serializable::raw_data_t serialized =
        device_path_serializer::serialize_indices(before);

    tested_container_t after =
        device_path_deserializer::deserialize_indices<std::vector, ccl::device_index_type>(
            serialized.begin(), serialized.end());

    if (before != after) {
        ASSERT_TRUE(false) << "Containers content differ:\nBefore:\t"
                           << native::detail::to_string(before) << "\nAfter:\t"
                           << native::detail::to_string(after);
    }
}

TEST(INDICES_SERIALIZE, indices_serialize_deserialize_vector_with_offset_test) {
    using namespace native::detail;
    using namespace native::detail::serialize;

    using tested_container_t = std::vector<ccl::device_index_type>;

    tested_container_t before{ ccl::device_index_type(0, 1, ccl::unused_index_value),
                               ccl::device_index_type(0, 0, ccl::unused_index_value),
                               ccl::device_index_type(0, 2, ccl::unused_index_value) };

    constexpr size_t offset = 7;
    device_path_serializable::raw_data_t serialized =
        device_path_serializer::serialize_indices(before, offset);

    auto offset_start_it = serialized.begin();
    auto offset_end_it = serialized.begin();
    std::advance(offset_end_it, offset);
    for (; offset_start_it != offset_end_it; ++offset_start_it) {
        if (*offset_start_it != 0) {
            ASSERT_TRUE(false) << "Invalid data for requested offset: " << offset << ", at pos: "
                               << std::distance(serialized.begin(), offset_start_it);
        }
    }

    tested_container_t after =
        device_path_deserializer::deserialize_indices<std::vector, ccl::device_index_type>(
            offset_start_it, serialized.end());

    if (before != after) {
        ASSERT_TRUE(false) << "Containers content differ:\nBefore:\t"
                           << native::detail::to_string(before) << "\nAfter:\t"
                           << native::detail::to_string(after);
    }
}

TEST(INDICES_SERIALIZE, indices_serialize_deserialize_graph_list_test) {
    using namespace native::detail;
    using namespace native::detail::serialize;

    plain_graph_list before{ { { ccl::device_index_type(0, 0, ccl::unused_index_value),
                                 ccl::device_index_type(0, 1, ccl::unused_index_value),
                                 ccl::device_index_type(0, 2, ccl::unused_index_value) },
                               {
                                   ccl::device_index_type(0, 3, ccl::unused_index_value),
                               },
                               { ccl::device_index_type(0, 99, ccl::unused_index_value),
                                 ccl::device_index_type(0, 98, ccl::unused_index_value),
                                 ccl::device_index_type(0, 97, ccl::unused_index_value) } } };

    device_path_serializable::raw_data_t serialized =
        device_path_serializer::serialize_indices(before);

    size_t deserialized_bytes = 0;
    plain_graph_list after =
        device_path_deserializer::deserialize_graph_list_indices(serialized, deserialized_bytes);

    if (deserialized_bytes != serialized.size()) {
        ASSERT_TRUE(false) << "Not all data deserialized: " << deserialized_bytes
                           << ", expected: " << serialized.size() << "\nBefore:\t"
                           << native::detail::to_string(before) << "\nAfter:\t"
                           << native::detail::to_string(after);
    }

    if (before != after) {
        ASSERT_TRUE(false) << "graph lists content differ:\nBefore:\t"
                           << native::detail::to_string(before) << "\nAfter:\t"
                           << native::detail::to_string(after);
    }
}

TEST(INDICES_SERIALIZE, indices_serialize_deserialize_cluster_graph_list_test) {
    using namespace native::detail;
    using namespace native::detail::serialize;

    global_sorted_plain_graphs before{
        { 0,
          {
              { ccl::device_index_type(0, 90, ccl::unused_index_value),
                ccl::device_index_type(0, 91, ccl::unused_index_value),
                ccl::device_index_type(0, 92, ccl::unused_index_value) },
              { ccl::device_index_type(0, 80, ccl::unused_index_value),
                ccl::device_index_type(0, 81, ccl::unused_index_value),
                ccl::device_index_type(0, 82, ccl::unused_index_value) },
              { ccl::device_index_type(0, 70, ccl::unused_index_value),
                ccl::device_index_type(0, 71, ccl::unused_index_value),
                ccl::device_index_type(0, 72, ccl::unused_index_value) },
          } },
        { 1,
          {
              { ccl::device_index_type(0, 0, ccl::unused_index_value),
                ccl::device_index_type(0, 1, ccl::unused_index_value),
                ccl::device_index_type(0, 2, ccl::unused_index_value) },
              { ccl::device_index_type(0, 10, ccl::unused_index_value),
                ccl::device_index_type(0, 11, ccl::unused_index_value),
                ccl::device_index_type(0, 12, ccl::unused_index_value) },
              { ccl::device_index_type(0, 20, ccl::unused_index_value),
                ccl::device_index_type(0, 1, ccl::unused_index_value),
                ccl::device_index_type(0, 272, ccl::unused_index_value) },
          } },
        { 4,
          {
              { ccl::device_index_type(0, 90, 0),
                ccl::device_index_type(0, 91, 1),
                ccl::device_index_type(0, 92, 99) },
              { ccl::device_index_type(0, 8, 10),
                ccl::device_index_type(0, 8, 10),
                ccl::device_index_type(0, 8, 10) },
              { ccl::device_index_type(10, 0, 99),
                ccl::device_index_type(20, 71, 199),
                ccl::device_index_type(30, 72, 9) },
          } }
    };

    device_path_serializable::raw_data_t serialized =
        device_path_serializer::serialize_indices(before);

    global_sorted_plain_graphs after =
        device_path_deserializer::deserialize_global_graph_list_indices(serialized);

    if (before != after) {
        ASSERT_TRUE(false) << "cluster graph lists content differ:\nBefore:\t"
                           << native::detail::to_string(before) << "\nAfter:\t"
                           << native::detail::to_string(after);
    }
}

TEST(COLORED_INDICES_SERIALIZE, colored_indices_serialize_deserialize_vector_test) {
    using namespace native::detail;
    using namespace native::detail::serialize;

    using tested_container_t = colored_plain_graph;

    tested_container_t before{ { 0, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                               { 1, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                               { 99, ccl::device_index_type(0, 2, ccl::unused_index_value) } };

    device_path_serializable::raw_data_t serialized =
        device_path_serializer::serialize_indices(before);

    tested_container_t after =
        device_path_deserializer::deserialize_indices<std::vector, colored_idx>(serialized.begin(),
                                                                                serialized.end());

    if (before != after) {
        ASSERT_TRUE(false) << "Containers content differ:\nBefore:\t"
                           << native::detail::to_string(before) << "\nAfter:\t"
                           << native::detail::to_string(after);
    }
}

TEST(COLORED_INDICES_SERIALIZE, colored_indices_serialize_deserialize_vector_with_offset_test) {
    using namespace native::detail;
    using namespace native::detail::serialize;

    using tested_container_t = colored_plain_graph;

    tested_container_t before{ { 78, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                               { 66, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                               { 1, ccl::device_index_type(0, 2, ccl::unused_index_value) } };

    constexpr size_t offset = 7;
    device_path_serializable::raw_data_t serialized =
        device_path_serializer::serialize_indices(before, offset);

    auto offset_start_it = serialized.begin();
    auto offset_end_it = serialized.begin();
    std::advance(offset_end_it, offset);
    for (; offset_start_it != offset_end_it; ++offset_start_it) {
        if (*offset_start_it != 0) {
            ASSERT_TRUE(false) << "Invalid data for requested offset: " << offset << ", at pos: "
                               << std::distance(serialized.begin(), offset_start_it);
        }
    }

    tested_container_t after =
        device_path_deserializer::deserialize_indices<std::vector, colored_idx>(offset_start_it,
                                                                                serialized.end());

    if (before != after) {
        ASSERT_TRUE(false) << "Containers content differ:\nBefore:\t"
                           << native::detail::to_string(before) << "\nAfter:\t"
                           << native::detail::to_string(after);
    }
}

TEST(COLORED_INDICES_SERIALIZE, colored_indices_serialize_deserialize_graph_list_test) {
    using namespace native::detail;
    using namespace native::detail::serialize;

    colored_plain_graph_list before{
        { { { 1, ccl::device_index_type(0, 0, ccl::unused_index_value) },
            { 1, ccl::device_index_type(0, 1, ccl::unused_index_value) },
            { 1, ccl::device_index_type(0, 2, ccl::unused_index_value) } },
          {
              { 0, ccl::device_index_type(0, 3, ccl::unused_index_value) },
          },
          { { 99, ccl::device_index_type(0, 99, ccl::unused_index_value) },
            { 98, ccl::device_index_type(0, 98, ccl::unused_index_value) },
            { 1, ccl::device_index_type(0, 97, ccl::unused_index_value) } } }
    };

    device_path_serializable::raw_data_t serialized =
        device_path_serializer::serialize_indices(before);

    size_t deserialized_bytes = 0;
    colored_plain_graph_list after =
        device_path_deserializer::deserialize_colored_graph_list_indices(serialized,
                                                                         deserialized_bytes);

    if (deserialized_bytes != serialized.size()) {
        ASSERT_TRUE(false) << "Not all data deserialized: " << deserialized_bytes
                           << ", expected: " << serialized.size() << "\nBefore:\t"
                           << native::detail::to_string(before) << "\nAfter:\t"
                           << native::detail::to_string(after);
    }

    if (before != after) {
        ASSERT_TRUE(false) << "graph lists content differ:\nBefore:\t"
                           << native::detail::to_string(before) << "\nAfter:\t"
                           << native::detail::to_string(after);
    }
}

TEST(COLORED_INDICES_SERIALIZE, colored_indices_serialize_deserialize_cluster_graph_list_test) {
    using namespace native::detail;
    using namespace native::detail::serialize;

    global_sorted_colored_plain_graphs before{
        { 0,
          {
              { { 1, ccl::device_index_type(0, 90, ccl::unused_index_value) },
                { 2, ccl::device_index_type(0, 91, ccl::unused_index_value) },
                { 5, ccl::device_index_type(0, 92, ccl::unused_index_value) } },
              { { 7, ccl::device_index_type(0, 80, ccl::unused_index_value) },
                { 9, ccl::device_index_type(0, 81, ccl::unused_index_value) },
                { 11, ccl::device_index_type(0, 82, ccl::unused_index_value) } },
              { { 1, ccl::device_index_type(0, 70, ccl::unused_index_value) },
                { 2, ccl::device_index_type(0, 71, ccl::unused_index_value) },
                { 3, ccl::device_index_type(0, 72, ccl::unused_index_value) } },
          } },
        { 1,
          {
              { { 1, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                { 2, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                { 3, ccl::device_index_type(0, 2, ccl::unused_index_value) } },
              { { 3, ccl::device_index_type(0, 10, ccl::unused_index_value) },
                { 3, ccl::device_index_type(0, 11, ccl::unused_index_value) },
                { 3, ccl::device_index_type(0, 12, ccl::unused_index_value) } },
              { { 3, ccl::device_index_type(0, 20, ccl::unused_index_value) },
                { 3, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                { 3, ccl::device_index_type(0, 272, ccl::unused_index_value) } },
          } },
        { 4,
          {
              { { 3, ccl::device_index_type(0, 90, 0) },
                { 3, ccl::device_index_type(0, 91, 1) },
                { 3, ccl::device_index_type(0, 92, 99) } },
              { { 3, ccl::device_index_type(0, 8, 10) },
                { 3, ccl::device_index_type(0, 8, 10) },
                { 3, ccl::device_index_type(0, 8, 10) } },
              { { 3, ccl::device_index_type(10, 0, 99) },
                { 3, ccl::device_index_type(20, 71, 199) },
                { 3, ccl::device_index_type(30, 72, 9) } },
          } }
    };

    device_path_serializable::raw_data_t serialized =
        device_path_serializer::serialize_indices(before);

    global_sorted_colored_plain_graphs after =
        device_path_deserializer::deserialize_global_colored_graph_list_indices(serialized);

    if (before != after) {
        ASSERT_TRUE(false) << "cluster graph lists content differ:\nBefore:\t"
                           << native::detail::to_string(before) << "\nAfter:\t"
                           << native::detail::to_string(after);
    }
}
} // namespace topology_suite
