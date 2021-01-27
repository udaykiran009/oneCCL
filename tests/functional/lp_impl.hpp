#pragma once

template <typename T>
void convert_fp32_to_lp_arrays(T* buf, short* lp_buf, size_t count, ccl_data_type dtype) {
    size_t floats_in_reg = FLOATS_IN_M512;
    short tail[16] = { 0 };

    for (size_t i = 0; i < count; i += floats_in_reg) {
        if (i / floats_in_reg == count / floats_in_reg) {
            convert_fp32_to_lp(buf + i, tail, dtype);
            for (size_t j = 0; j < (count - i); j++) {
                lp_buf[i + j] = tail[j];
            }
        }
        else {
            convert_fp32_to_lp(buf + i, lp_buf + i, dtype);
        }
    }
}

template <typename T>
void convert_lp_to_fp32_arrays(short* lp_buf, T* buf, size_t count, ccl_data_type dtype) {
    size_t floats_in_reg = FLOATS_IN_M512;
    T tail[16] = { 0 };

    for (size_t i = 0; i < count; i += floats_in_reg) {
        if (i / floats_in_reg == count / floats_in_reg) {
            convert_lp_to_fp32(lp_buf + i, tail, dtype);
            for (size_t j = 0; j < (count - i); j++) {
                buf[i + j] = tail[j];
            }
        }
        else {
            convert_lp_to_fp32(lp_buf + i, buf + i, dtype);
        }
    }
}

template <typename T>
void make_lp_prologue(typed_test_param<T>& param, size_t count) {
    ccl_data_type dtype = param.test_conf.datatype;
    for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
        if (param.test_conf.place_type == PT_IN) {
            T* recv_buf_fp32 = param.recv_buf[buf_idx].data();
            short* recv_buf_lp = param.recv_buf_lp[buf_idx].data();
            convert_fp32_to_lp_arrays(recv_buf_fp32, recv_buf_lp, count, dtype);
        }
        else {
            T* send_buf_fp32 = param.send_buf[buf_idx].data();
            short* send_buf_lp = param.send_buf_lp[buf_idx].data();
            convert_fp32_to_lp_arrays(send_buf_fp32, send_buf_lp, count, dtype);
        }
    }
}

template <typename T>
void make_lp_epilogue(typed_test_param<T>& param, size_t count) {
    ccl_data_type dtype = param.test_conf.datatype;
    for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
        T* recv_buf_fp32 = param.recv_buf[buf_idx].data();
        short* recv_buf_lp = param.recv_buf_lp[buf_idx].data();
        convert_lp_to_fp32_arrays(recv_buf_lp, recv_buf_fp32, count, dtype);
    }
}