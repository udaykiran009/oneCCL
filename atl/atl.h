#ifndef ATL_H
#define ATL_H

#include <stddef.h>

#include "atl_comm.h"

// TODO: remove:
#define HAVE_OFI    1
#define HAVE_OFI_DL 0

#ifndef container_of
#define container_of(ptr, type, field) \
	((type *) ((char *)ptr - offsetof(type, field)))
#endif

typedef enum atl_ret_val {
    ATL_FAILURE = -1,
    ATL_SUCCESS = 0,
} atl_ret_val_t;

typedef struct atl_desc atl_desc_t;

typedef struct atl_ops {
    atl_ret_val_t (*finalize)(atl_desc_t *desc, atl_comm_t **comms);
} atl_ops_t;

struct atl_desc {
    atl_ops_t *ops;
};

typedef struct atl_attr {
    size_t comm_count;
    atl_comm_attr_t comm_attr;
} atl_attr_t;

#define ARG_LIST                          \
    int *argc, char ***argv,              \
    size_t *proc_idx, size_t *proc_count, \
    atl_attr_t *attr,                     \
    atl_comm_t ***atl_comms,              \
    atl_desc_t **atl_desc

/*
 * Dynamically loaded providers must export the following entry point.
 * This is invoked by the ATL framework when the provider library
 * is loaded.
 */
#define ATL_EXT_INI \
__attribute__((visibility ("default"),EXTERNALLY_VISIBLE)) \
atl_ret_val_t atl_ini(ARG_LIST)


/* Provider initialization function signature that built-in providers
 * must specify. */
#define INI_SIG(name)   \
atl_ret_val_t name(ARG_LIST)

/* for each provider defines for three scenarios:
 * dl: externally visible ctor with known name (see fi_prov.h)
 * built-in: ctor function def, don't export symbols
 * not built: no-op call for ctor
 */

#if (HAVE_OFI) && (HAVE_OFI_DL)
#  define ATL_OFI_INI ATL_EXT_INI
#  define ATL_OFI_INIT atl_noop_init
#elif (HAVE_OFI)
#  define ATL_OFI_INI INI_SIG(atl_ofi_init)
#  define ATL_OFI_INIT atl_ofi_init
ATL_OFI_INI ;
#else
#  define ATL_OFI_INIT atl_noop_init
#endif

static inline INI_SIG(atl_noop_init)
{
    return 0;
}

atl_ret_val_t atl_init(ARG_LIST);

static inline atl_ret_val_t atl_finalize(atl_desc_t *desc, atl_comm_t **comms)
{
    return desc->ops->finalize(desc, comms);
}

#endif
