#include "atl.h"

atl_status_t atl_init(ARG_LIST)
{
    return ATL_OFI_INIT(argc, argv, proc_idx, proc_count, attr, atl_comms, atl_desc);
}
