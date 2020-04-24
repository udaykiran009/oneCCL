#include "base.hpp"
#include "topology/hw_topology_test.hpp"

int main(int ac, char* av[])
{
#ifndef STANDALONE_UT
  testing::InitGoogleTest(&ac, av);
  return RUN_ALL_TESTS();
#else
{
}
#endif
}

