#pragma once
#include <initializer_list>
#include "../topology_utils.hpp"
#include "common/comm/l0/topology/ring/ring_construction_utils.hpp"

namespace topology_suite
{

TEST_F(router_fixture, simple_merge_test)
{
    using namespace native;
    using namespace native::details;

    {
        size_t process_index = 1;
        size_t terminator_index = 3;
        process_creator_params params = prepare_process_params(process_index, {}, {});

        allied_process_group_ring_topology top(process_index, params.thread_ids.size(),
                                           *pg_comm, *pg_comm->gpu_device_storage,
                                           params.cluster_device_rank_offset,
                                           params.cluster_device_size);

        pg_comm->cluster_gpu_indices = {
                                        {"unit_tests",
                                            {
                                                {0,
                                                    {
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                                    }
                                                },
                                                {process_index,
                                                    {
                                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                                    }
                                                },
                                                {2,
                                                    {
                                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                                    }
                                                },
                                            },
                                        }};

        size_t proces_num = pg_comm->cluster_gpu_indices[std::string("unit_tests")].size();
        global_sorted_colored_plain_graphs my_colored_rings{
                            {0,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)}
                                    }
                                }
                            }
                        };
        std::stringstream ss;
        global_colored_plain_graphs merged_cluster_graphs(proces_num);
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            global_colored_plain_graphs local_merged_cluster_graphs =
                    top.merge_allied_nodes_in_colored_plain_graphs(ss, pg_comm->cluster_gpu_indices,
                                                                   pr_index, proces_num, my_colored_rings);
            merged_cluster_graphs[pr_index] = local_merged_cluster_graphs[pr_index];
        }

        {
            global_colored_plain_graphs to_check{
                            {0,
                                {
                                    {
                                        {terminator_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,2, ccl::unused_index_value)},

                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},

                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)}
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    }
                                }
                            }
                        };
            if (merged_cluster_graphs != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid cluster data after merge");
            }
        }


        global_sorted_colored_plain_graphs final_global_merged_cluster_graphs;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            colored_plain_graph_list local_final_rings =
                    top.resize_merged_colored_graphs_for_process(pr_index,
                                                                proces_num,
                                                                merged_cluster_graphs,
                                                                my_colored_rings[pr_index], ss);
            final_global_merged_cluster_graphs[pr_index] = local_final_rings;
        }

        {
            global_sorted_colored_plain_graphs to_check{
                            {0,
                                {
                                    {
                                        {terminator_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,2, ccl::unused_index_value)},

                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},

                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)}
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},

                                        {terminator_index, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    }
                                }
                            }
                        };
            if (final_global_merged_cluster_graphs != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid final_global_merged_cluster_graphs data");
            }
        }

        std::map<size_t, ccl::process_device_indices_t> scaleout_devices;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            scaleout_devices[pr_index] =
                        top.create_scaleout_devices_in_colored_graphs_for_process(
                                                                pr_index,
                                                                proces_num,
                                                                final_global_merged_cluster_graphs,
                                                                my_colored_rings,
                                                                ss);
            UT_ASSERT(scaleout_devices[pr_index].empty(), "No scaleup devices failed");
        }

        std::map<size_t, ccl::process_device_indices_t> ipc_devices;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            ipc_devices[pr_index] =
                        top.create_ipc_devices_in_colored_graphs_for_process(
                                                                pr_index,
                                                                proces_num,
                                                                final_global_merged_cluster_graphs,
                                                                my_colored_rings,
                                                                ss);
            UT_ASSERT(!ipc_devices[pr_index].empty(), "Not empty ipc devices failed");
        }
        {
            std::map<size_t, ccl::process_device_indices_t> to_check{
                        {0,
                            {
                                {1,
                                    {
                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                    },
                                },
                                {2,
                                    {
                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {1,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    },
                                },
                                {2,
                                    {
                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {2,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    },
                                },
                                {1,
                                    {
                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                    }
                                },
                            }
                        }};
            if (ipc_devices != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid ipc_devices data after merge");
            }
        }

        /**/
        std::map<size_t, ccl::process_device_indices_t> ipc_src;
        std::map<size_t, ccl::process_device_indices_t> ipc_dst;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            const colored_plain_graph_list &local_rings =
                        final_global_merged_cluster_graphs[pr_index];
            for (const colored_plain_graph& graph : local_rings)
            {
                ccl::process_device_indices_t ipc_src_local;
                ccl::process_device_indices_t ipc_dst_local;

                native::details::separate_ipc_devices(ipc_devices[pr_index], pr_index,
                                     proces_num, graph, ipc_src_local,
                                     ipc_dst_local);

                ipc_src[pr_index] = ipc_src_local;
                ipc_dst[pr_index] = ipc_dst_local;
            }
        }
        {
            std::map<size_t, ccl::process_device_indices_t> to_check_src{
                        {0,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    },
                                },
                            }
                        },
                        {1,
                            {
                                {1,
                                    {
                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                    },
                                },
                            }
                        },
                        {2,
                            {
                                {2,
                                    {
                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                    }
                                },
                            }
                        }};
            std::map<size_t, ccl::process_device_indices_t> to_check_dst{
                        {0,
                            {
                                {1,
                                    {
                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                    },
                                },
                            }
                        },
                        {1,
                            {
                                {2,
                                    {
                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                    },
                                },
                            }
                        },
                        {2,
                            {
                                {terminator_index,
                                    {
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    }
                                },
                            }
                        }};
            if (ipc_src != to_check_src)
            {
                abort();
                UT_ASSERT(false, "Invalid ipc_src data");
            }
            if (ipc_dst != to_check_dst)
            {
                abort();
                UT_ASSERT(false, "Invalid ipc_dst data");
            }
        }
    }
}

TEST_F(router_fixture, simple_multithreaded_merge_test)
{
    using namespace native;
    using namespace native::details;

    {
        size_t process_index = 1;
        size_t terminator_index = 3;
        process_creator_params params = prepare_process_params(process_index, {}, {});

        allied_process_group_ring_topology top(process_index, params.thread_ids.size(),
                                           *pg_comm, *pg_comm->gpu_device_storage,
                                           params.cluster_device_rank_offset,
                                           params.cluster_device_size);

        pg_comm->cluster_gpu_indices = {
                                        {"unit_tests",
                                            {
                                                {0,
                                                    {
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                                    }
                                                },
                                                {process_index,
                                                    {
                                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                                        ccl::device_index_type(0,3, ccl::unused_index_value),
                                                        ccl::device_index_type(0,3, ccl::unused_index_value),

                                                    }
                                                },
                                                {2,
                                                    {
                                                        ccl::device_index_type(0,4, ccl::unused_index_value),
                                                        ccl::device_index_type(0,4, ccl::unused_index_value),
                                                        ccl::device_index_type(0,5, ccl::unused_index_value),
                                                        ccl::device_index_type(0,5, ccl::unused_index_value),

                                                    }
                                                },
                                            },
                                        }};

        size_t proces_num = pg_comm->cluster_gpu_indices[std::string("unit_tests")].size();
        global_sorted_colored_plain_graphs my_colored_rings{
                            {0,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    },
                                    {
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                    },
                                    {
                                        {process_index, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {2, ccl::device_index_type(0,4, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,4, ccl::unused_index_value)},
                                    },
                                    {
                                        {2, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                    }
                                }
                            }
                        };




        adjacency_matrix expected_matrix {
                                {
                                    ccl::device_index_type(0,0, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 1},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,1, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,2, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,3, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,4, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,5, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 1},
                                    }
                                }};

        std::stringstream ss;
        global_colored_plain_graphs merged_cluster_graphs(proces_num);
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            global_colored_plain_graphs local_merged_cluster_graphs =
                    top.merge_allied_nodes_in_colored_plain_graphs(ss, pg_comm->cluster_gpu_indices,
                                                                   pr_index, proces_num,
                                                                   my_colored_rings,
                                                 std::bind(utils::test_custom_p2p_ping,
                                                           std::placeholders::_1,
                                                           std::placeholders::_2,
                                                           expected_matrix));
            merged_cluster_graphs[pr_index] = local_merged_cluster_graphs[pr_index];
        }

        {
            global_colored_plain_graphs to_check{
                            {0,
                                {
                                    {
                                        {terminator_index, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    },
                                    {
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)}
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)}
                                    },
                                    {
                                        {process_index, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,4, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,4, ccl::unused_index_value)}
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {process_index, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,4, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,4, ccl::unused_index_value)}
                                    },
                                    {
                                        {2, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    }
                                }
                            }
                        };
            if (merged_cluster_graphs != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid cluster data after merge");
            }
        }


        global_sorted_colored_plain_graphs final_global_merged_cluster_graphs;
        for (size_t pr_index = 0; pr_index <proces_num; pr_index++)
        {
            colored_plain_graph_list local_final_rings =
                    top.resize_merged_colored_graphs_for_process(pr_index,
                                                                proces_num,
                                                                merged_cluster_graphs,
                                                                my_colored_rings[pr_index], ss);
            final_global_merged_cluster_graphs[pr_index] = local_final_rings;
        }

        std::map<size_t, ccl::process_device_indices_t> scaleout_devices;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            scaleout_devices[pr_index] =
                        top.create_scaleout_devices_in_colored_graphs_for_process(
                                                                pr_index,
                                                                proces_num,
                                                                final_global_merged_cluster_graphs,
                                                                my_colored_rings,
                                                                ss);
        }
        {
            std::map<size_t, ccl::process_device_indices_t> to_check{
                        {0,
                            {
                            }
                        },
                        {1,
                            {
                            }
                        },
                        {2,
                            {
                            }
                        }};
            if (scaleout_devices != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid scaleout_devices data after merge");
            }
        }

        std::map<size_t, ccl::process_device_indices_t> ipc_devices;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            ipc_devices[pr_index] =
                        top.create_ipc_devices_in_colored_graphs_for_process(
                                                                pr_index,
                                                                proces_num,
                                                                final_global_merged_cluster_graphs,
                                                                my_colored_rings,
                                                                ss);
            UT_ASSERT(!ipc_devices[pr_index].empty(), "Not empty ipc devices failed");
        }
        {
            std::map<size_t, ccl::process_device_indices_t> to_check{
                        {0,
                            {
                                {1,
                                    {
                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                    },
                                },
                                {2,
                                    {
                                        ccl::device_index_type(0,5, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {1,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                    },
                                },
                                {2,
                                    {
                                        ccl::device_index_type(0,4, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {2,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    },
                                },
                                {1,
                                    {
                                        ccl::device_index_type(0,3, ccl::unused_index_value)
                                    }
                                },
                            }
                        }};
            if (ipc_devices != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid ipc_devices data after merge");
            }
        }
    }
}

TEST_F(router_fixture, simple_scaleout_test)
{
    using namespace native;
    using namespace native::details;

    {
        size_t process_index = 1;
        process_creator_params params = prepare_process_params(process_index, {}, {});

        allied_process_group_ring_topology top(process_index, params.thread_ids.size(),
                                           *pg_comm, *pg_comm->gpu_device_storage,
                                           params.cluster_device_rank_offset,
                                           params.cluster_device_size);

        pg_comm->cluster_gpu_indices = {
                                        {"unit_tests",
                                            {
                                                {0,
                                                    {
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                                    }
                                                },
                                                {process_index,
                                                    {
                                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                                    }
                                                },
                                                {2,
                                                    {
                                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                                    }
                                                },
                                            },
                                        }};

        size_t proces_num = pg_comm->cluster_gpu_indices[std::string("unit_tests")].size();
        global_sorted_colored_plain_graphs my_colored_rings{
                            {0,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)}
                                    }
                                }
                            }
                        };
        std::stringstream ss;
        global_colored_plain_graphs merged_cluster_graphs(proces_num);
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            global_colored_plain_graphs local_merged_cluster_graphs =
                    top.merge_allied_nodes_in_colored_plain_graphs(ss, pg_comm->cluster_gpu_indices,
                                                                   pr_index, proces_num,
                                                                   my_colored_rings,
                                                                   [](const native::ccl_device &lhs,
                                                                    const native::ccl_device &rhs) -> size_t
                                                                    {
                                                                        return 0;
                                                                    });
            merged_cluster_graphs[pr_index] = local_merged_cluster_graphs[pr_index];
        }

        {
            global_colored_plain_graphs to_check{
                            {0,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    },
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                    },
                                }
                            },
                            {2,
                                {
                                    {
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                    },
                                }
                            }
                        };
            if (merged_cluster_graphs != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid cluster data after merge");
            }
        }


        global_sorted_colored_plain_graphs final_global_merged_cluster_graphs;
        for (size_t pr_index = 0; pr_index <proces_num; pr_index++)
        {
            colored_plain_graph_list local_final_rings =
                    top.resize_merged_colored_graphs_for_process(pr_index,
                                                                proces_num,
                                                                merged_cluster_graphs,
                                                                my_colored_rings[pr_index], ss);
            final_global_merged_cluster_graphs[pr_index] = local_final_rings;
        }

        {
            global_sorted_colored_plain_graphs to_check{
                            {0,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                    }
                                }
                            }
                        };
            if (final_global_merged_cluster_graphs != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid final_global_merged_cluster_graphs data");
            }
        }

        std::map<size_t, ccl::process_device_indices_t> scaleout_devices;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            scaleout_devices[pr_index] =
                        top.create_scaleout_devices_in_colored_graphs_for_process(
                                                                pr_index,
                                                                proces_num,
                                                                final_global_merged_cluster_graphs,
                                                                my_colored_rings,
                                                                ss);
            UT_ASSERT(!scaleout_devices[pr_index].empty(), "Not emptyscaleup devices failed");
        }
        {
            std::map<size_t, ccl::process_device_indices_t> to_check{
                        {0,
                            {
                                {1,
                                    {
                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                    },
                                },
                                {2,
                                    {
                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {1,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    },
                                },
                                {2,
                                    {
                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {2,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    },
                                },
                                {1,
                                    {
                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                    }
                                },
                            }
                        }};
            if (scaleout_devices != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid scaleout_devices data after merge");
            }
        }

        std::map<size_t, ccl::process_device_indices_t> ipc_devices;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            ipc_devices[pr_index] =
                        top.create_ipc_devices_in_colored_graphs_for_process(
                                                                pr_index,
                                                                proces_num,
                                                                final_global_merged_cluster_graphs,
                                                                my_colored_rings,
                                                                ss);
            UT_ASSERT(ipc_devices[pr_index].empty(), "No ipc devices failed");
        }
        {
            std::map<size_t, ccl::process_device_indices_t> to_check{
                        {0,
                            {
                            }
                        },
                        {1,
                            {
                            }
                        },
                        {2,
                            {
                            }
                        }};
            if (ipc_devices != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid ipc_devices data after merge");
            }
        }
    }
}

TEST_F(router_fixture, simple_scaleout_multithreaded_merge_test)
{
    using namespace native;
    using namespace native::details;

    {
        size_t process_index = 1;
        process_creator_params params = prepare_process_params(process_index, {}, {});

        allied_process_group_ring_topology top(process_index, params.thread_ids.size(),
                                           *pg_comm, *pg_comm->gpu_device_storage,
                                           params.cluster_device_rank_offset,
                                           params.cluster_device_size);

        pg_comm->cluster_gpu_indices = {
                                        {"unit_tests",
                                            {
                                                {0,
                                                    {
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                                    }
                                                },
                                                {process_index,
                                                    {
                                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                                        ccl::device_index_type(0,3, ccl::unused_index_value),
                                                        ccl::device_index_type(0,3, ccl::unused_index_value),

                                                    }
                                                },
                                                {2,
                                                    {
                                                        ccl::device_index_type(0,4, ccl::unused_index_value),
                                                        ccl::device_index_type(0,4, ccl::unused_index_value),
                                                        ccl::device_index_type(0,5, ccl::unused_index_value),
                                                        ccl::device_index_type(0,5, ccl::unused_index_value),

                                                    }
                                                },
                                            },
                                        }};

        size_t proces_num = pg_comm->cluster_gpu_indices[std::string("unit_tests")].size();
        global_sorted_colored_plain_graphs my_colored_rings{
                            {0,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    },
                                    {
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                    },
                                    {
                                        {process_index, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {2, ccl::device_index_type(0,4, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,4, ccl::unused_index_value)},
                                    },
                                    {
                                        {2, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                    }
                                }
                            }
                        };




        adjacency_matrix expected_matrix {
                                {
                                    ccl::device_index_type(0,0, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,1, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,2, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,3, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,4, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,5, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 1},
                                    }
                                }};

        std::stringstream ss;
        global_colored_plain_graphs merged_cluster_graphs(proces_num);
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            global_colored_plain_graphs local_merged_cluster_graphs =
                    top.merge_allied_nodes_in_colored_plain_graphs(ss, pg_comm->cluster_gpu_indices,
                                                                   pr_index, proces_num,
                                                                   my_colored_rings,
                                                 std::bind(utils::test_custom_p2p_ping,
                                                           std::placeholders::_1,
                                                           std::placeholders::_2,
                                                           expected_matrix));
            merged_cluster_graphs[pr_index] = local_merged_cluster_graphs[pr_index];
        }

        {
            global_colored_plain_graphs to_check{
                            {0,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    },
                                    {
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)}
                                    },
                                    {
                                        {process_index, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {2, ccl::device_index_type(0,4, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,4, ccl::unused_index_value)}
                                    },
                                    {
                                        {2, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                    }
                                }
                            }
                        };
            if (merged_cluster_graphs != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid cluster data after merge");
            }
        }


        global_sorted_colored_plain_graphs final_global_merged_cluster_graphs;
        for (size_t pr_index = 0; pr_index <proces_num; pr_index++)
        {
            colored_plain_graph_list local_final_rings =
                    top.resize_merged_colored_graphs_for_process(pr_index,
                                                                proces_num,
                                                                merged_cluster_graphs,
                                                                my_colored_rings[pr_index], ss);
            final_global_merged_cluster_graphs[pr_index] = local_final_rings;
        }

        std::map<size_t, ccl::process_device_indices_t> scaleout_devices;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            scaleout_devices[pr_index] =
                        top.create_scaleout_devices_in_colored_graphs_for_process(
                                                                pr_index,
                                                                proces_num,
                                                                final_global_merged_cluster_graphs,
                                                                my_colored_rings,
                                                                ss);
        }
        {
            std::map<size_t, ccl::process_device_indices_t> to_check{
                        {0,
                            {
                                {1,
                                    {
                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                    },
                                },
                                {2,
                                    {
                                        ccl::device_index_type(0,5, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {1,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                    },
                                },
                                {2,
                                    {
                                        ccl::device_index_type(0,4, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {2,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    },
                                },
                                {1,
                                    {
                                        ccl::device_index_type(0,3, ccl::unused_index_value)
                                    }
                                },
                            }
                        }};
            if (scaleout_devices != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid scaleout_devices data after merge");
            }
        }

        std::map<size_t, ccl::process_device_indices_t> ipc_devices;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            ipc_devices[pr_index] =
                        top.create_ipc_devices_in_colored_graphs_for_process(
                                                                pr_index,
                                                                proces_num,
                                                                final_global_merged_cluster_graphs,
                                                                my_colored_rings,
                                                                ss);
        }
        {
            std::map<size_t, ccl::process_device_indices_t> to_check{
                        {0,
                            {
                            }
                        },
                        {1,
                            {
                            }
                        },
                        {2,
                            {
                            }
                        }};
            if (ipc_devices != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid ipc_devices data after merge");
            }
        }
    }
}

TEST_F(router_fixture, symmetric_scaleout_test)
{
    using namespace native;
    using namespace native::details;

    {
        size_t process_index = 1;
        process_creator_params params = prepare_process_params(process_index, {}, {});

        allied_process_group_ring_topology top(process_index, params.thread_ids.size(),
                                           *pg_comm, *pg_comm->gpu_device_storage,
                                           params.cluster_device_rank_offset,
                                           params.cluster_device_size);

        pg_comm->cluster_gpu_indices = {
                                        {"unit_tests",
                                            {
                                                {0,
                                                    {
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                                    }
                                                },
                                                {process_index,
                                                    {
                                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                                    }
                                                },
                                                {2,
                                                    {
                                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                                    }
                                                },
                                                {3,
                                                    {
                                                        ccl::device_index_type(0,3, ccl::unused_index_value),
                                                        ccl::device_index_type(0,3, ccl::unused_index_value),
                                                        ccl::device_index_type(0,3, ccl::unused_index_value)
                                                    }
                                                },
                                            },
                                        }};

        size_t proces_num = pg_comm->cluster_gpu_indices[std::string("unit_tests")].size();
        global_sorted_colored_plain_graphs my_colored_rings{
                            {0,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)}
                                    }
                                }
                            },
                            {3,
                                {
                                    {
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)}
                                    }
                                }
                            }
                        };
        std::stringstream ss;
        adjacency_matrix expected_matrix {
                                {
                                    ccl::device_index_type(0,0, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,1, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,2, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 1},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,3, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 1},
                                    }
                                }};
        global_colored_plain_graphs merged_cluster_graphs(proces_num);
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            global_colored_plain_graphs local_merged_cluster_graphs =
                    top.merge_allied_nodes_in_colored_plain_graphs(ss, pg_comm->cluster_gpu_indices,
                                                                   pr_index, proces_num,
                                                                   my_colored_rings,
                                                std::bind(utils::test_custom_p2p_ping,
                                                           std::placeholders::_1,
                                                           std::placeholders::_2,
                                                           expected_matrix));
            merged_cluster_graphs[pr_index] = local_merged_cluster_graphs[pr_index];
        }

        {
            global_colored_plain_graphs to_check{
                            {0,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                    },
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                    },
                                }
                            },
                            {2,
                                {
                                    {
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                    },
                                }
                            },
                            {3,
                                {
                                    {
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                    },
                                }
                            }
                        };
            if (merged_cluster_graphs != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid cluster data after merge");
            }
        }


        global_sorted_colored_plain_graphs final_global_merged_cluster_graphs;
        for (size_t pr_index = 0; pr_index <proces_num; pr_index++)
        {
            colored_plain_graph_list local_final_rings =
                    top.resize_merged_colored_graphs_for_process(pr_index,
                                                                proces_num,
                                                                merged_cluster_graphs,
                                                                my_colored_rings[pr_index], ss);
            final_global_merged_cluster_graphs[pr_index] = local_final_rings;
        }

        {
            global_sorted_colored_plain_graphs to_check{
                            {0,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {3,
                                {
                                    {
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                    }
                                }
                            }
                        };
            if (final_global_merged_cluster_graphs != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid final_global_merged_cluster_graphs data");
            }
        }

        std::map<size_t, ccl::process_device_indices_t> scaleout_devices;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            scaleout_devices[pr_index] =
                        top.create_scaleout_devices_in_colored_graphs_for_process(
                                                                pr_index,
                                                                proces_num,
                                                                final_global_merged_cluster_graphs,
                                                                my_colored_rings,
                                                                ss);
            UT_ASSERT(!scaleout_devices[pr_index].empty(), "Not emptyscaleup devices failed");
        }
        {
            std::map<size_t, ccl::process_device_indices_t> to_check{
                        {0,
                            {
                                {3,
                                    {
                                        ccl::device_index_type(0,3, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {1,
                            {
                                {2,
                                    {
                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {2,
                            {
                                {1,
                                    {
                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {3,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    }
                                },
                            }
                        }};
            if (scaleout_devices != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid scaleout_devices data after merge");
            }
        }

        std::map<size_t, ccl::process_device_indices_t> ipc_devices;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            ipc_devices[pr_index] =
                        top.create_ipc_devices_in_colored_graphs_for_process(
                                                                pr_index,
                                                                proces_num,
                                                                final_global_merged_cluster_graphs,
                                                                my_colored_rings,
                                                                ss);
        }
        {
            std::map<size_t, ccl::process_device_indices_t> to_check{
                        {0,
                            {
                                {1,
                                    {
                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                    },
                                },
                            }
                        },
                        {1,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    },
                                },
                            }
                        },
                        {2,
                            {
                                {3,
                                    {
                                        ccl::device_index_type(0,3, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {3,
                            {
                                {2,
                                    {
                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                    }
                                },
                            }
                        }};
            if (ipc_devices != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid ipc_devices data after merge");
            }
        }
    }
}

TEST_F(router_fixture, symmetric_scaleout_multithreaded_merge_test)
{
    using namespace native;
    using namespace native::details;

    {
        size_t process_index = 1;
        size_t terminator_index = 4;
        process_creator_params params = prepare_process_params(process_index, {}, {});

        allied_process_group_ring_topology top(process_index, params.thread_ids.size(),
                                           *pg_comm, *pg_comm->gpu_device_storage,
                                           params.cluster_device_rank_offset,
                                           params.cluster_device_size);

        pg_comm->cluster_gpu_indices = {
                                        {"unit_tests",
                                            {
                                                {0,
                                                    {
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                                    }
                                                },
                                                {process_index,
                                                    {
                                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                                        ccl::device_index_type(0,3, ccl::unused_index_value),
                                                        ccl::device_index_type(0,3, ccl::unused_index_value),

                                                    }
                                                },
                                                {2,
                                                    {
                                                        ccl::device_index_type(0,4, ccl::unused_index_value),
                                                        ccl::device_index_type(0,4, ccl::unused_index_value),
                                                        ccl::device_index_type(0,5, ccl::unused_index_value),
                                                        ccl::device_index_type(0,5, ccl::unused_index_value),

                                                    }
                                                },
                                                {3,
                                                    {
                                                        ccl::device_index_type(0,6, ccl::unused_index_value),
                                                        ccl::device_index_type(0,6, ccl::unused_index_value),
                                                        ccl::device_index_type(0,7, ccl::unused_index_value),
                                                        ccl::device_index_type(0,7, ccl::unused_index_value),

                                                    }
                                                }
                                            },
                                        }};

        size_t proces_num = pg_comm->cluster_gpu_indices[std::string("unit_tests")].size();
        global_sorted_colored_plain_graphs my_colored_rings{
                            {0,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    },
                                    {
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                    },
                                    {
                                        {process_index, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {2, ccl::device_index_type(0,4, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,4, ccl::unused_index_value)},
                                    },
                                    {
                                        {2, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {3,
                                {
                                    {
                                        {3, ccl::device_index_type(0,6, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,6, ccl::unused_index_value)},
                                    },
                                    {
                                        {3, ccl::device_index_type(0,7, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,7, ccl::unused_index_value)},
                                    }
                                }
                            }
                        };




        adjacency_matrix expected_matrix {
                                {
                                    ccl::device_index_type(0,0, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,6,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,7,ccl::unused_index_value), 1},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,1, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,6,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,7,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,2, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,6,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,7,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,3, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,6,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,7,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,4, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,6,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,7,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,5, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,6,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,7,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,6, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,6,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,7,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,7, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,4,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,5,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,6,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,7,ccl::unused_index_value), 1},
                                    }
                                }
                                };

        std::stringstream ss;
        global_colored_plain_graphs merged_cluster_graphs(proces_num);
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            global_colored_plain_graphs local_merged_cluster_graphs =
                    top.merge_allied_nodes_in_colored_plain_graphs(ss, pg_comm->cluster_gpu_indices,
                                                                   pr_index, proces_num,
                                                                   my_colored_rings,
                                                 std::bind(utils::test_custom_p2p_ping,
                                                           std::placeholders::_1,
                                                           std::placeholders::_2,
                                                           expected_matrix));
            merged_cluster_graphs[pr_index] = local_merged_cluster_graphs[pr_index];
        }

        {
            global_colored_plain_graphs to_check{
                            {0,
                                {
                                    {
                                        {terminator_index, ccl::device_index_type(0,7, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,7, ccl::unused_index_value)},

                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    },
                                    {
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)}
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,2, ccl::unused_index_value)}
                                    },
                                    {
                                        {process_index, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {2, ccl::device_index_type(0,4, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,4, ccl::unused_index_value)}
                                    },
                                    {
                                        {2, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,6, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,6, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {3,
                                {
                                    {
                                        {2, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,5, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,6, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,6, ccl::unused_index_value)},
                                    },
                                    {
                                        {3, ccl::device_index_type(0,7, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,7, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {terminator_index, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    }
                                }
                            }
                        };
            if (merged_cluster_graphs != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid cluster data after merge");
            }
        }


        global_sorted_colored_plain_graphs final_global_merged_cluster_graphs;
        for (size_t pr_index = 0; pr_index <proces_num; pr_index++)
        {
            colored_plain_graph_list local_final_rings =
                    top.resize_merged_colored_graphs_for_process(pr_index,
                                                                proces_num,
                                                                merged_cluster_graphs,
                                                                my_colored_rings[pr_index], ss);
            final_global_merged_cluster_graphs[pr_index] = local_final_rings;
        }

        std::map<size_t, ccl::process_device_indices_t> scaleout_devices;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            scaleout_devices[pr_index] =
                        top.create_scaleout_devices_in_colored_graphs_for_process(
                                                                pr_index,
                                                                proces_num,
                                                                final_global_merged_cluster_graphs,
                                                                my_colored_rings,
                                                                ss);
        }
        {
            std::map<size_t, ccl::process_device_indices_t> to_check{
                        {0,
                            {
                            }
                        },
                        {1,
                            {
                                {2,
                                    {
                                        ccl::device_index_type(0,4, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {2,
                            {
                                {1,
                                    {
                                        ccl::device_index_type(0,3, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {3,
                            {
                            }
                        }};
            if (scaleout_devices != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid scaleout_devices data after merge");
            }
        }

        std::map<size_t, ccl::process_device_indices_t> ipc_devices;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            ipc_devices[pr_index] =
                        top.create_ipc_devices_in_colored_graphs_for_process(
                                                                pr_index,
                                                                proces_num,
                                                                final_global_merged_cluster_graphs,
                                                                my_colored_rings,
                                                                ss);
        }
        {
            std::map<size_t, ccl::process_device_indices_t> to_check{
                        {0,
                            {
                                {process_index,
                                    {
                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                    }
                                },
                                {3,
                                    {
                                        ccl::device_index_type(0,7, ccl::unused_index_value),
                                    }
                                },
                            }
                        },
                        {process_index,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {2,
                            {
                                {3,
                                    {
                                        ccl::device_index_type(0,6, ccl::unused_index_value)
                                    }
                                }
                            }
                        },
                        {3,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    }
                                },
                                {2,
                                    {
                                        ccl::device_index_type(0,5, ccl::unused_index_value)
                                    }
                                }
                            }
                        }};
            if (ipc_devices != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid ipc_devices data after merge");
            }
        }
    }
}

TEST_F(router_fixture, unsymmetric_scaleout_test)
{
    using namespace native;
    using namespace native::details;

    {
        size_t process_index = 1;
        process_creator_params params = prepare_process_params(process_index, {}, {});

        allied_process_group_ring_topology top(process_index, params.thread_ids.size(),
                                           *pg_comm, *pg_comm->gpu_device_storage,
                                           params.cluster_device_rank_offset,
                                           params.cluster_device_size);

        pg_comm->cluster_gpu_indices = {
                                        {"unit_tests",
                                            {
                                                {0,
                                                    {
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                                    }
                                                },
                                                {process_index,
                                                    {
                                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                                    }
                                                },
                                                {2,
                                                    {
                                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                                        ccl::device_index_type(0,2, ccl::unused_index_value),
                                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                                    }
                                                },
                                                {3,
                                                    {
                                                        ccl::device_index_type(0,3, ccl::unused_index_value),
                                                        ccl::device_index_type(0,3, ccl::unused_index_value),
                                                        ccl::device_index_type(0,3, ccl::unused_index_value)
                                                    }
                                                },
                                            },
                                        }};

        size_t proces_num = pg_comm->cluster_gpu_indices[std::string("unit_tests")].size();
        global_sorted_colored_plain_graphs my_colored_rings{
                            {0,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)}
                                    }
                                }
                            },
                            {3,
                                {
                                    {
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)}
                                    }
                                }
                            }
                        };
        std::stringstream ss;
        adjacency_matrix expected_matrix {
                                {
                                    ccl::device_index_type(0,0, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,1, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,2, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 1},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 0},
                                    }
                                },
                                {
                                    ccl::device_index_type(0,3, ccl::unused_index_value),
                                    {
                                        {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,2,ccl::unused_index_value), 0},
                                        {ccl::device_index_type(0,3,ccl::unused_index_value), 1},
                                    }
                                }};
        global_colored_plain_graphs merged_cluster_graphs(proces_num);
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            global_colored_plain_graphs local_merged_cluster_graphs =
                    top.merge_allied_nodes_in_colored_plain_graphs(ss, pg_comm->cluster_gpu_indices,
                                                                   pr_index, proces_num,
                                                                   my_colored_rings,
                                                std::bind(utils::test_custom_p2p_ping,
                                                           std::placeholders::_1,
                                                           std::placeholders::_2,
                                                           expected_matrix));
            merged_cluster_graphs[pr_index] = local_merged_cluster_graphs[pr_index];
        }

        {
            global_colored_plain_graphs to_check{
                            {0,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                    },
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                    },
                                }
                            },
                            {2,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                    },
                                }
                            },
                            {3,
                                {
                                    {
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                    },
                                }
                            }
                        };
            if (merged_cluster_graphs != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid cluster data after merge");
            }
        }


        global_sorted_colored_plain_graphs final_global_merged_cluster_graphs;
        for (size_t pr_index = 0; pr_index <proces_num; pr_index++)
        {
            colored_plain_graph_list local_final_rings =
                    top.resize_merged_colored_graphs_for_process(pr_index,
                                                                proces_num,
                                                                merged_cluster_graphs,
                                                                my_colored_rings[pr_index], ss);
            final_global_merged_cluster_graphs[pr_index] = local_final_rings;
        }

        {
            global_sorted_colored_plain_graphs to_check{
                            {0,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {process_index,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {2,
                                {
                                    {
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {0, ccl::device_index_type(0,0, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {process_index, ccl::device_index_type(0,1, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                        {2, ccl::device_index_type(0,2, ccl::unused_index_value)},
                                    }
                                }
                            },
                            {3,
                                {
                                    {
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                        {3, ccl::device_index_type(0,3, ccl::unused_index_value)},
                                    }
                                }
                            }
                        };
            if (final_global_merged_cluster_graphs != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid final_global_merged_cluster_graphs data");
            }
        }

        std::map<size_t, ccl::process_device_indices_t> scaleout_devices;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            scaleout_devices[pr_index] =
                        top.create_scaleout_devices_in_colored_graphs_for_process(
                                                                pr_index,
                                                                proces_num,
                                                                final_global_merged_cluster_graphs,
                                                                my_colored_rings,
                                                                ss);
        }
        {
            std::map<size_t, ccl::process_device_indices_t> to_check{
                        {0,
                            {
                                {3,
                                    {
                                        ccl::device_index_type(0,3, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {1,
                            {
                            }
                        },
                        {2,
                            {
                                {3,
                                    {
                                        ccl::device_index_type(0,3, ccl::unused_index_value)
                                    }
                                },
                            }
                        },
                        {3,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    }
                                },
                                {2,
                                    {
                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                    }
                                },
                            }
                        }};
            if (scaleout_devices != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid scaleout_devices data after merge");
            }
        }

                std::map<size_t, ccl::process_device_indices_t> ipc_devices;
        for (size_t pr_index = 0; pr_index < proces_num; pr_index++)
        {
            ipc_devices[pr_index] =
                        top.create_ipc_devices_in_colored_graphs_for_process(
                                                                pr_index,
                                                                proces_num,
                                                                final_global_merged_cluster_graphs,
                                                                my_colored_rings,
                                                                ss);
        }
        {
            std::map<size_t, ccl::process_device_indices_t> to_check{
                        {0,
                            {
                                {process_index,
                                    {
                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                    }
                                }
                            }
                        },
                        {process_index,
                            {
                                {0,
                                    {
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    },
                                },
                                {2,
                                    {
                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                    },
                                }
                            }
                        },
                        {2,
                            {
                                {1,
                                    {
                                        ccl::device_index_type(0,1, ccl::unused_index_value)
                                    }
                                }
                            }
                        },
                        {3,
                            {
                            }
                        }};
            if (ipc_devices != to_check)
            {
                abort();
                UT_ASSERT(false, "Invalid ipc_devices data after merge");
            }
        }
    }
}

}
