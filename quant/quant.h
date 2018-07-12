#ifndef QUANT_H
#define QUANT_H

#include <mpi.h>

#define MPI_QUANT_OP (MPI_Op)0xdeadbeef

typedef struct
{
    char* lib_path;                 /* quantization lib path */
    char* quant_buffer_func_name;   /* quantize buffer function name */
    char* dequant_buffer_func_name; /* de-quantize buffer function name */
    char* reduce_sum_func_name;     /* reduce sum for quantized buffer function name */
    size_t block_size;              /* quantize meta data: one block's bytes */
    size_t elem_in_block;           /* quantize meta data: elements in one block */
} quant_params_t;

typedef enum quant_lib_status
{
    quant_lib_loaded   = 1,
    quant_lib_unloaded = 0
} quant_lib_status;

void quant_init(quant_params_t* qparam);
void quant_finalize();
size_t quant_get_reduce_count(size_t count);
MPI_Op quant_get_op();
MPI_Datatype quant_get_data_type();
void quant_quantize(void* buf, size_t count);
void quant_dequantize(void* buf, size_t count);

#endif
