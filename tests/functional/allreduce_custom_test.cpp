#define TEST_ICCL_REDUCE
#define TEST_ICCL_CUSTOM_PROLOG
#define TEST_ICCL_CUSTOM_EPILOG
#define TEST_ICCL_CUSTOM_REDUCE
#define Collective_Name "ICCL_ALLREDUCE_ALGO"

#include "base.hpp"
#include <functional>
#include <vector>
#include <chrono>
#include <cmath>
template <typename T>
iccl_status_t do_prologue_T_2x(const void* in_buf, size_t in_count, iccl_datatype_t in_dtype,
                                   void** out_buf, size_t* out_count, iccl_datatype_t* out_dtype,
                                   size_t* out_dtype_size)
{
    size_t idx;
    ASSERT(out_buf, "null ptr");

    if (out_buf) *out_buf = (void*)in_buf;
    if (out_count) *out_count = in_count;
    if (out_dtype) *out_dtype = in_dtype;
    if (out_dtype_size) *out_dtype_size = sizeof(T);

    for (idx = 0; idx < in_count; idx++)
    {
        ((T*)(*out_buf))[idx] = ((T*)in_buf)[idx] * 2;
        // printf("do_prologue_T_2x out_buf[%zu]=%f\n",idx, (double)((T*)out_buf)[idx]);
        // printf("do_prologue_T_2x out_buf[%zu]=%f\n",idx, (double)((T*)in_buf)[idx]);
    }
    return iccl_status_success;
}
template <typename T>
iccl_status_t do_epilogue_T_2x(const void* in_buf, size_t in_count, iccl_datatype_t in_dtype,
                                   void* out_buf, size_t* out_count, iccl_datatype_t out_dtype)
{
    size_t idx;
    if (out_count) *out_count = in_count;
    for (idx = 0; idx < in_count; idx++)
    {
        ((T*)out_buf)[idx] = ((T*)in_buf)[idx] * 2;

    }
        // printf("do_epilogue_T_2x_________________in_buf[65]=%d\n", ((int*)in_buf)[65]);
        // printf("do_epilogue_T_2x_________________out_buf[65]=%d\n", ((int*)out_buf)[65]);

    return iccl_status_success;
}
const char char_max = (char)(((unsigned char) char(-1)) / 2);
template <typename T>
iccl_status_t do_prologue_T_to_char(const void* in_buf, size_t in_count, iccl_datatype_t in_dtype,
                                        void** out_buf, size_t* out_count, iccl_datatype_t* out_dtype,
                                        size_t* out_dtype_size)
{
    size_t idx;
    ASSERT(out_buf, "null ptr");

    if (out_buf) *out_buf = malloc(in_count);
    if (out_count) *out_count = in_count;
    if (out_dtype) *out_dtype = iccl_dtype_char;
    if (out_dtype_size) *out_dtype_size = 1;

    for (idx = 0; idx < in_count; idx++)
    {
        T fval = ((T*)in_buf)[idx];
        int ival = (int)fval;
        ((char*)(*out_buf))[idx] = (char)(ival % 256);
        // printf("fval=%f, %zu\n",(double)fval, idx);
        // printf("ival=%f, %zu\n",(double)ival, idx);
        // int new_ival = (int)((char*)(*out_buf))[idx];
        // printf("new_ival=%d, %zu\n",new_ival, idx);
        // printf("out_buf[%zu]=%d\n",idx, ((char*)(*out_buf))[idx]);
    }
    // printf("prolog_________________in_buf[65]=%d\n",((char*)in_buf)[65]);
    // printf("prolog_________________out_buf[65]=%d\n",((char*)(*out_buf))[65]);
    // printf("prolog_________________in_buf[66]=%d\n",((char*)in_buf)[66]);
    // printf("prolog_________________out_buf[66]=%d\n",((char*)(*out_buf))[66]);

    return iccl_status_success;
}

template <typename T>
iccl_status_t do_epilogue_char_to_T(const void* in_buf, size_t in_count, iccl_datatype_t in_dtype,
                                        void* out_buf, size_t* out_count, iccl_datatype_t out_dtype)
{
    size_t idx;
    if (out_count) *out_count = in_count;
    for (idx = 0; idx < in_count; idx++)
    {
        ((T*)out_buf)[idx] = (T)(((char*)in_buf)[idx]);
    }
    if (in_buf != out_buf) free((void*)in_buf);
    return iccl_status_success;
}

template <typename T>
iccl_status_t do_reduction_null(const void* in_buf, size_t in_count, void* inout_buf,
                               size_t* out_count, const void** ctx, iccl_datatype_t dtype)
{
    size_t idx;
    if (out_count) *out_count = in_count;
            for (idx = 0; idx < in_count; idx++)
            {
                ((T*)inout_buf)[idx] = (T)0;
            }
    return iccl_status_success;
}
template <typename T>
iccl_status_t do_reduction_custom(const void* in_buf, size_t in_count, void* inout_buf,
                               size_t* out_count, const void** ctx, iccl_datatype_t dtype)
{
    size_t idx;
    if (out_count) *out_count = in_count;
            for (idx = 0; idx < in_count; idx++)
            {
                ((T*)inout_buf)[idx] += ((T*)in_buf)[idx];
            }
    return iccl_status_success;
}
template <typename T>
int set_custom_reduction (TypedTestParam <T> &param){
    TestReductionType customFuncName = param.GetReductionName();
    switch (customFuncName) {
        case RT_CUSTOM:
            param.coll_attr.reduction_fn = do_reduction_custom<T>;
            break;
        case RT_CUSTOM_NULL:
            param.coll_attr.reduction_fn = do_reduction_null<T>;
            break;
        default:
            return TEST_FAILURE;

    }
    return TEST_SUCCESS;
}
template < typename T > class AllReduceCustomTest:public BaseTest < T > {
public:
    int Check(TypedTestParam < T > &param) {
        size_t epilog_coeff = 1;
        size_t prolog_coeff = 1;
        size_t prolog_coeff_prod = 1;
        size_t epilog_coeff_prod = 1;
        T expected_fin = 0;
        if (param.GetPrologType() == PRT_T_TO_2X) {
            prolog_coeff = 2;
            prolog_coeff_prod = pow(2,param.processCount);
        }
        if (param.GetEpilogType() == EPLT_T_TO_2X) {
            epilog_coeff = 2;
            epilog_coeff_prod = epilog_coeff;
        }
        T tmp1;
        for (size_t j = 0; j < param.bufferCount; j++) {
            for (size_t i = 0; i < param.elemCount; i++) {
                if (param.GetReductionName() == RT_SUM) {
                    T expected =
                        ((param.processCount * (param.processCount - 1) / 2) +
                        ((i + j)  * param.processCount));
                    if (param.GetEpilogType() == EPLT_CHAR_TO_T && param.GetPrologType() == PRT_T_TO_CHAR){
                         expected_fin = ((char)(expected * prolog_coeff * epilog_coeff));
                        tmp1 = param.recvBuf[j][i];
                    }
                    else if (param.GetEpilogType() == EPLT_T_TO_2X && param.GetPrologType() == PRT_T_TO_CHAR){
                        expected_fin = ((char)(expected * prolog_coeff * epilog_coeff));
                        tmp1 = ((char*)param.recvBuf[j].data())[i];
                        // printf("PRT_T_TO_CHAR expected_fin=%f\n",(double)expected_fin);
                    }
                    else if (param.GetEpilogType() == EPLT_CHAR_TO_T || param.GetPrologType() == PRT_T_TO_CHAR){
                        expected_fin = ((char)(expected * prolog_coeff * epilog_coeff));
                        tmp1 = ((char*)param.recvBuf[j].data())[i];
                        // printf("PRT_T_TO_CHAR expected_fin=%f\n",(double)expected_fin);
                    }
                    else {
                        // expected_fin = expected;
                        expected_fin = expected *  prolog_coeff * epilog_coeff;
                        tmp1 = param.recvBuf[j][i];
                        // printf("NO PRT_T_TO_CHAR expected_fin=%f\n",(double)expected_fin);
                    }
                    // expected_fin = expected *  prolog_coeff * epilog_coeff;
                    // T tmp1 = ((char*)param.recvBuf[j].data())[i];
                    // char tmp2 = (char)(tmp1 & 0x000000FF);
                    // printf("!!!!!!!!!tmp2 =%d\n",(int)tmp2);
                    // if (param.recvBuf[j][i] != expected_fin) {
                    // int int_size = sizeof(int)*4;
                    // int shift = 1;
                    // char* byte_buf = new char[int_size];
                    // for (int k = 0; k < int_size; k++) {
                        // if ((int)(param.recvBuf[j][i]) & shift)
                            // byte_buf[int_size -k-1] = '1';
                        // else
                            // byte_buf[int_size -k-1] = '0';
                        // shift <<= 1;
                    // }
                    // printf("byte = %s\n", byte_buf);
                    // delete[] byte_buf;
                    if (tmp1 != expected_fin) {
                        // printf("!!!!!!!!!tmp2_2 =%d\n",(int)tmp2);
                        sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f, tmp1 = %f\n",
                            param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected_fin, (double) tmp1);
                        return TEST_FAILURE;
                    }
                }

                else if (param.GetReductionName() == RT_MAX) {
                    T expected = 0;
                    if (param.GetPrologType() == PRT_T_TO_CHAR)
                    {
                        expected = get_expected_max<char>(i, j, param.processCount, prolog_coeff);
                        expected_fin = (char)(expected * epilog_coeff);
                    }
                    else
                    {
                        expected = get_expected_max<T>(i, j, param.processCount, prolog_coeff);

                    }
                    expected_fin = expected * epilog_coeff;
                    if (param.recvBuf[j][i] != expected_fin) {
                        sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                                                  param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected_fin);
                        return TEST_FAILURE;
                    }
                }
                else if (param.GetReductionName()== RT_MIN) {
                    T expected = 0;
                    if (param.GetPrologType() == PRT_T_TO_CHAR)
                    {
                        expected = get_expected_min<char>(i, j, param.processCount, prolog_coeff);
                        expected_fin = (char)(expected * epilog_coeff);
                    }
                    else
                    {
                        expected = get_expected_min<T>(i, j, param.processCount, prolog_coeff);
                        expected_fin = expected * epilog_coeff;
                    }
                     // expected_fin = expected * epilog_coeff;
                    if (param.recvBuf[j][i] != expected_fin) {
                        sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                                                   param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected_fin);
                        return TEST_FAILURE;
                    }
                }
                else if (param.GetReductionName() == RT_PROD) {
                    T expected = 1;
                    for (size_t k = 0; k < param.processCount; k++) {
                        expected *= (i + j + k);
                    }
                    if (param.GetPrologType() == PRT_T_TO_CHAR)
                        expected_fin = ((char)(expected * prolog_coeff_prod * epilog_coeff_prod));
                    else
                        expected_fin = expected * prolog_coeff_prod * epilog_coeff_prod;
                    if (param.recvBuf[j][i] != expected_fin) {

                        sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                            param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected_fin);
                        return TEST_FAILURE;
                    }
                }
                else if (param.GetReductionName() == RT_CUSTOM) {
                    T expected =
                        ((param.processCount * (param.processCount - 1) / 2) +
                        (i * param.processCount));
                    T expected_fin = expected * prolog_coeff * epilog_coeff;
                    if (param.recvBuf[j][i] != expected_fin) {
                        sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                            param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected_fin);
                        return TEST_FAILURE;
                    }
                }
                else if (param.GetReductionName() == RT_CUSTOM_NULL) {
                    T expected = 0;
                    T expected_fin = expected * prolog_coeff * epilog_coeff;
                    if (param.recvBuf[j][i] != expected_fin) {
                        sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                            param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected_fin);
                        return TEST_FAILURE;
                    }
                }
                else
                    return TEST_FAILURE;
            }
        }
        return TEST_SUCCESS;
    }

    int Run(TypedTestParam < T > &param) {
        size_t result = 0;
        for (size_t iter = 0; iter < 2; iter++) {
            SHOW_ALGO(Collective_Name);
            this->FillBuffers (param);
            this->SwapBuffers(param, iter);
            size_t idx = 0;
            size_t* Buffers = param.DefineStartOrder();
            for (idx = 0; idx < param.bufferCount; idx++) {
                this->Init (param, idx);
                if (param.GetPrologType() == PRT_T_TO_2X) {
                    param.coll_attr.prologue_fn = do_prologue_T_2x<T>;
                }
                if (param.GetEpilogType() == EPLT_T_TO_2X) {
                    param.coll_attr.epilogue_fn = do_epilogue_T_2x<T>;
                }
                else if (param.GetEpilogType() == EPLT_CHAR_TO_T && param.GetPrologType() == PRT_T_TO_CHAR) {
                    param.coll_attr.prologue_fn = do_prologue_T_to_char<T>;
                    param.coll_attr.epilogue_fn = do_epilogue_char_to_T<T>;
                }
                //norm
                else if (param.GetEpilogType() == EPLT_CHAR_TO_T && (param.GetPrologType() == PRT_T_TO_2X || param.GetPrologType() == PRT_NULL)) {
                    return TEST_SUCCESS;
                }
                if (param.GetPrologType() == PRT_T_TO_CHAR && param.GetEpilogType() == EPLT_T_TO_2X) {
                    // return TEST_SUCCESS;
                    param.coll_attr.epilogue_fn = do_epilogue_T_2x<T>;
                    param.coll_attr.prologue_fn = do_prologue_T_to_char<T>;
                }
                if (param.GetPrologType() == PRT_T_TO_CHAR) {
                    // return TEST_SUCCESS;

                    param.coll_attr.prologue_fn = do_prologue_T_to_char<T>;
                }
                if (param.GetReductionType() == iccl_reduction_custom) {
                    if (set_custom_reduction<T>(param))
                        return TEST_FAILURE;
                }
                param.req[Buffers[idx]] = (param.GetPlaceType() == PT_IN) ?
                    param.global_comm.allreduce(param.recvBuf[Buffers[idx]].data(), param.recvBuf[Buffers[idx]].data(), param.elemCount,
                                  (iccl::data_type) param.GetDataType(),(iccl::reduction) param.GetReductionType(), &param.coll_attr) :
                    param.global_comm.allreduce(param.sendBuf[Buffers[idx]].data(), param.recvBuf[Buffers[idx]].data(), param.elemCount,
                                  (iccl::data_type) param.GetDataType(),(iccl::reduction) param.GetReductionType(), &param.coll_attr);
            }
            param.DefineCompletionOrderAndComplete();
            result += Check(param);
        }
        return result;
    }
};

RUN_METHOD_DEFINITION(AllReduceCustomTest);
TEST_CASES_DEFINITION(AllReduceCustomTest);
MAIN_FUNCTION();
