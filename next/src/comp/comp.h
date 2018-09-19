#ifndef COMP_H
#define COMP_H

#include "mlsl_types.h"

mlsl_status_t mlsl_comp_copy(const void *in_buf, void *out_buf, size_t count, mlsl_data_type_t dtype);
mlsl_status_t mlsl_comp_reduce(const void *in_buf, size_t in_count, void *inout_buf, size_t *out_count,
                               mlsl_data_type_t dtype, mlsl_reduction_t reduction, mlsl_reduction_fn_t reduction_fn);

#endif /* COMP_H */
