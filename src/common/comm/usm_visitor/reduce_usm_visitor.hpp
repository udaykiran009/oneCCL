#pragma once

#include "oneapi/ccl.hpp"
#include "oneapi/ccl/type_traits.hpp"

template <class communicator_impl>
struct reduce_usm_visitor {
    using self_t = communicator_impl;

    self_t* get_self() {
        return static_cast<self_t*>(this);
    }

    const self_t* get_self() const {
        return static_cast<const self_t*>(const_cast<reduce_usm_visitor*>(this)->get_self());
    }

    template <class... Args>
    bool visit(ccl::event& req,
               ccl::datatype dtype,
               const void* send_buf,
               void* recv_buf,
               size_t count,
               Args&&... args) {
        bool processed = false;
        LOG_TRACE("comm: ",
                  /*get_self()->to_string(),*/
                  " - starting to find visitor for datatype: ",
                  ccl::to_string(dtype),
                  " , handle: ",
                  utils::enum_to_underlying(dtype));

        switch (dtype) {
            case ccl::datatype::int8: {
                using type = int8_t;
                req = get_self()->template reduce_impl<type>(static_cast<const type*>(send_buf),
                                                             static_cast<type*>(recv_buf),
                                                             count,
                                                             std::forward<Args>(args)...);
                processed = true;
                break;
            }
            case ccl::datatype::uint8: {
                using type = uint8_t;
                req = get_self()->template reduce_impl<type>(static_cast<const type*>(send_buf),
                                                             static_cast<type*>(recv_buf),
                                                             count,
                                                             std::forward<Args>(args)...);
                processed = true;
                break;
            }
            case ccl::datatype::int16: {
                using type = int16_t;
                req = get_self()->template reduce_impl<type>(static_cast<const type*>(send_buf),
                                                             static_cast<type*>(recv_buf),
                                                             count,
                                                             std::forward<Args>(args)...);
                processed = true;
                break;
            }
            case ccl::datatype::uint16: {
                using type = uint16_t;
                req = get_self()->template reduce_impl<type>(static_cast<const type*>(send_buf),
                                                             static_cast<type*>(recv_buf),
                                                             count,
                                                             std::forward<Args>(args)...);
                processed = true;
                break;
            }
            case ccl::datatype::int32: {
                using type = int32_t;
                req = get_self()->template reduce_impl<type>(static_cast<const type*>(send_buf),
                                                             static_cast<type*>(recv_buf),
                                                             count,
                                                             std::forward<Args>(args)...);
                processed = true;
                break;
            }
            case ccl::datatype::uint32: {
                using type = uint32_t;
                req = get_self()->template reduce_impl<type>(static_cast<const type*>(send_buf),
                                                             static_cast<type*>(recv_buf),
                                                             count,
                                                             std::forward<Args>(args)...);
                processed = true;
                break;
            }
            case ccl::datatype::int64: {
                using type = int64_t;
                req = get_self()->template reduce_impl<type>(static_cast<const type*>(send_buf),
                                                             static_cast<type*>(recv_buf),
                                                             count,
                                                             std::forward<Args>(args)...);
                processed = true;
                break;
            }
            case ccl::datatype::uint64: {
                using type = uint64_t;
                req = get_self()->template reduce_impl<type>(static_cast<const type*>(send_buf),
                                                             static_cast<type*>(recv_buf),
                                                             count,
                                                             std::forward<Args>(args)...);
                processed = true;
                break;
            }
            case ccl::datatype::float16: {
                using type = ccl::float16;
                req = get_self()->template reduce_impl<type>(static_cast<const type*>(send_buf),
                                                             static_cast<type*>(recv_buf),
                                                             count,
                                                             std::forward<Args>(args)...);
                processed = true;
                break;
            }
            case ccl::datatype::float32: {
                using type = float;
                req = get_self()->template reduce_impl<type>(static_cast<const type*>(send_buf),
                                                             static_cast<type*>(recv_buf),
                                                             count,
                                                             std::forward<Args>(args)...);
                processed = true;
                break;
            }
            case ccl::datatype::float64: {
                using type = double;
                req = get_self()->template reduce_impl<type>(static_cast<const type*>(send_buf),
                                                             static_cast<type*>(recv_buf),
                                                             count,
                                                             std::forward<Args>(args)...);
                processed = true;
                break;
            }
            case ccl::datatype::bfloat16: {
                using type = ccl::bfloat16;
                req = get_self()->template reduce_impl<type>(static_cast<const type*>(send_buf),
                                                             static_cast<type*>(recv_buf),
                                                             count,
                                                             std::forward<Args>(args)...);
                processed = true;
                break;
            }
            default: {
                CCL_THROW("unknown datatype ", dtype);
                LOG_DEBUG("comm: ",
                          /*get_self()->to_string(),*/
                          " - no found visitor for datatype: ",
                          ccl::to_string(dtype),
                          " , handle: ",
                          utils::enum_to_underlying(dtype),
                          ", use RAW types");
                break;
            }
        }
        return processed;
    }
};
