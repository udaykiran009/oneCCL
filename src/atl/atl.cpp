#include <algorithm>
#include <assert.h>
#include <cstring>
#include <dirent.h>
#include <dlfcn.h>

#include "atl/atl.h"
#include "common/log/log.hpp"

#define LIB_SUFFIX ".so"

static int initialized = 0;

static int
atl_lib_filter(const struct dirent* entry)
{
    size_t l = strlen(entry->d_name);
    size_t sfx = sizeof(LIB_SUFFIX) - 1;

    if (l > sfx)
        return strstr((entry->d_name), LIB_SUFFIX) ? 1 : 0;
    else
        return 0;
}

static void
atl_ini_dir(const char* transport_name,
            int* argc, char*** argv,
            atl_attr_t* attr,
            atl_ctx_t** ctx,
            const char* dir)
{
    int n = 0;
    char* lib;
    void* dlhandle;
    struct dirent** liblist = NULL;
    typedef atl_status_t (*init_f)(atl_transport_t *);
    init_f init_func;
    size_t transport_name_len = strlen(transport_name);

    if (strcmp(transport_name, "mpi") == 0 && !getenv("I_MPI_ROOT"))
    {
        LOG_INFO("ATL MPI transport is requested but seems Intel MPI environment is not set. "
                 "Please source release_mt version of Intel MPI (2019 or higher version).");
    }

    n = scandir(dir, &liblist, atl_lib_filter, NULL);
    if (n < 0)
        goto libdl_done;

    while (n--)
    {
        if (asprintf(&lib, "%s/%s", dir, liblist[n]->d_name) < 0)
            goto libdl_done;

        LOG_DEBUG("opening lib ", lib);
        dlhandle = dlopen(lib, RTLD_NOW);
        free(liblist[n]);
        if (dlhandle == NULL)
        {
            LOG_INFO("can't open lib ", lib, ", error ", dlerror());
            free(lib);
            continue;
        }

        init_func = reinterpret_cast<init_f >(dlsym(dlhandle, "atl_ini"));
        if (init_func == NULL)
        {
            dlclose(dlhandle);
            free(lib);
        }
        else
        {
            LOG_DEBUG("lib ", lib, " contains necessary symbol");
            free(lib);

            atl_transport_t transport;
            atl_status_t ret;

            if ((init_func)(&transport) != ATL_STATUS_SUCCESS)
                continue;

            if (strncmp(transport.name, transport_name,
                        std::min(transport_name_len, strlen(transport.name))))
                continue;

            ret = transport.init(argc, argv, attr, ctx);
            if (ret != ATL_STATUS_SUCCESS)
                continue;

            break;
        }
    }

libdl_done:
    while (n-- > 0)
        free(liblist[n]);
    free(liblist);
}

/*
    Split the given string "s" using the specified delimiter(s) in the string
    "delim" and return an array of strings. The array is terminated with a NULL
    pointer. Returned array should be freed with ofi_free_string_array().

    Returns NULL on failure.
 */
static char**
atl_split_and_alloc(const char* s, const char* delim, size_t* count)
{
    int i, n;
    char* tmp;
    char* dup = NULL;
    char** arr = NULL;

    if (!s || !delim)
        return NULL;

    dup = strdup(s);
    if (!dup)
        return NULL;

    /* compute the array size */
    n = 1;
    for (tmp = dup; *tmp != '\0'; ++tmp)
    {
        for (i = 0; delim[i] != '\0'; ++i)
        {
            if (*tmp == delim[i])
            {
                ++n;
                break;
            }
        }
    }

    /* +1 to leave space for NULL terminating pointer */
    arr = static_cast<char**>(calloc(n + 1, sizeof(*arr)));
    if (!arr)
        goto cleanup;

    /* set array elts to point inside the dup'ed string */
    for (tmp = dup, i = 0; tmp != NULL; ++i)
        arr[i] = strsep(&tmp, delim);

    assert(i == n);

    if (count)
        *count = n;

    return arr;

cleanup:
    free(dup);
    return NULL;
}

/* see atl_split_and_alloc() */
static void
atl_free_string_array(char** s)
{
    /* all strings are allocated from the same strdup'ed slab, so just free
     * the first element */
    if (s != NULL)
        free(s[0]);

    /* and then the actual array of pointers */
    free(s);
}

atl_status_t atl_init(const char* transport_name,
                      int* argc, char*** argv,
                      atl_attr_t* attr,
                      atl_ctx_t** ctx)
{
    const char* transport_dl_dir = NULL;
    int n = 0;
    char** dirs;
    void* dlhandle;

    if (initialized)
        return ATL_STATUS_FAILURE;

    dlhandle = dlopen(NULL, RTLD_NOW);
    if (dlhandle == NULL)
        goto err_dlopen;

    dlclose(dlhandle);

    transport_dl_dir = getenv("CCL_ATL_TRANSPORT_PATH");
    if (!transport_dl_dir)
        transport_dl_dir = ATL_TRANSPORT_DL_DIR;

    dirs = atl_split_and_alloc(transport_dl_dir, ":", NULL);
    if (dirs)
    {
        for (n = 0; dirs[n]; ++n)
        {
            atl_ini_dir(transport_name,
                        argc, argv,
                        attr, ctx,
                        dirs[n]);
        }
        atl_free_string_array(dirs);
    }

    return ATL_STATUS_SUCCESS;

err_dlopen:
    return ATL_STATUS_FAILURE;
}
