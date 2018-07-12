#include "atl.h"

atl_ret_val_t atl_init(ARG_LIST)
{
    return ATL_OFI_INIT(argc, argv, proc_idx, proc_count, attr, atl_comms, atl_desc);
}
