#pragma once
#include "common/comm/l0/modules/kernel_argument_policies.hpp"

namespace native {
// base class for kernel argument
template <size_t pos, class policy_impl>
struct kernel_arg : public policy_impl {
    enum { index = pos };
    using policy = policy_impl;
    using arg_type = typename policy::arg_type;
    using return_t = typename policy::return_t;
};

// thread-safe argument: used for concurrent read/write applications
template <size_t pos, class type>
using thread_safe_arg = kernel_arg<pos, arg_access_policy_atomic<pos, type, false>>;

// thread-safe unchashed argument: used for concurrent read/write applications
template <size_t pos, class type>
using thread_safe_uncached_arg = kernel_arg<pos, arg_access_policy_atomic_uncached<pos, type, false>>;

// thread-safe destructive-copying argument (rechargable): used for concurrent read/write applications, where reader take-away exising value
template <size_t pos, class type>
using thread_exchangable_arg = kernel_arg<pos, arg_access_policy_atomic_move<pos, type, false>>;

// default, single threaded access argument
template <size_t pos, class type>
using arg = kernel_arg<pos, arg_access_policy_default<pos, type>>;

// empty argument
template <size_t pos>
using stub_arg = kernel_arg<pos, arg_no_access_policy<pos>>;

// utilities
namespace detail {
struct args_printer {
    args_printer(std::stringstream& ss) : out(ss) {}

    template <typename Arg>
    void operator()(const Arg& arg) {
        out << "idx: " << Arg::index << "\t";
        dump_arg_value(arg, out);
        out << std::endl;
    }

    // atomic argument pretty printing
    template <size_t pos, class type>
    void operator()(const thread_safe_arg<pos, type>& arg) {
        out << "idx: " << pos << "\t";
        dump_arg_value(arg, out);
        out << "\tATOMIC" << std::endl;
    }

    template <size_t pos, class type>
    void operator()(const thread_exchangable_arg<pos, type>& arg) {
        out << "idx: " << pos << "\t";
        arg.dump(out);
        out << "\tATOMIC_EXG" << std::endl;
    }

    // stub argument pretty printing
    template <size_t pos>
    void operator()(const stub_arg<pos>& arg) {
        out << "idx: " << pos << "\tSTUB" << std::endl;
    }
    std::stringstream& out;

private:
    template <typename Arg>
    void dump_arg_value(const Arg& arg, std::stringstream& ss) {
        if (arg.test()) {
            auto ret = arg.load();
            ss << "{ " << ret.second << " }";
        }
        else {
            out << "{ EMPTY }";
        }
    }
};
} // namespace detail
} // namespace native
