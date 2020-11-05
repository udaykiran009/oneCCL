#pragma once
#include "common/comm/l0/modules/kernel_argument_policies.hpp"

namespace native {

namespace options {

// Options for kernel arguments, here we should defile all the aspects that don't change parameter's
// behaviour, i.e. don't affect load/store functions such. Things that does that(i.e. thread-safety)
// should be defined as separate policy.
template <bool uncached = false>
struct generic {
    static constexpr bool is_uncached() {
        return uncached;
    }
};

using empty = generic<>;
using uncached = generic<true>;

} // namespace options

// base class for kernel argument
template <size_t pos, class policy_impl, class options = options::empty>
struct kernel_arg : public policy_impl, options {
    enum { index = pos };
    using policy = policy_impl;
    using arg_type = typename policy::arg_type;
    using return_t = typename policy::return_t;
    using options_t = options;
};

// thread-safe argument: used for concurrent read/write applications
template <size_t pos, class type, class options = options::empty>
using thread_safe_arg = kernel_arg<pos, arg_access_policy_atomic<pos, type, false>, options>;

// thread-safe destructive-copying argument (rechargable): used for concurrent read/write applications, where reader take-away exising value
template <size_t pos, class type, class options = options::empty>
using thread_exchangable_arg =
    kernel_arg<pos, arg_access_policy_atomic_move<pos, type, false>, options>;

// default, single threaded access argument
template <size_t pos, class type, class options = options::empty>
using arg = kernel_arg<pos, arg_access_policy_default<pos, type>, options>;

// empty argument
template <size_t pos, class options = options::empty>
using stub_arg = kernel_arg<pos, arg_no_access_policy<pos>, options>;

// utilities
namespace detail {
struct args_printer {
    args_printer(std::stringstream& ss) : out(ss) {}

    template <typename Arg>
    void operator()(const Arg& arg) {
        out << "idx: " << Arg::index << "\t";
        dump_arg_value(arg, out);
        using opt = typename Arg::options_t;
        print_options(opt{}, out);
        out << std::endl;
    }

    // atomic argument pretty printing
    template <size_t pos, class type, class options>
    void operator()(const thread_safe_arg<pos, type, options>& arg) {
        out << "idx: " << pos << "\t";
        dump_arg_value(arg, out);
        print_options(options{}, out);
        out << "\tATOMIC" << std::endl;
    }

    template <size_t pos, class type, class options>
    void operator()(const thread_exchangable_arg<pos, type, options>& arg) {
        out << "idx: " << pos << "\t";
        arg.dump(out);
        print_options(options{}, out);
        out << "\tATOMIC_EXG" << std::endl;
    }

    // stub argument pretty printing
    template <size_t pos, class options>
    void operator()(const stub_arg<pos, options>& arg) {
        out << "idx: " << pos;
        print_options(options{}, out);
        out << "\tSTUB" << std::endl;
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
            ss << "{ EMPTY }";
        }
    }

    template <class Options>
    void print_options(Options opt, std::stringstream& ss) {
        if (opt.is_uncached())
            ss << "\tUNCACHED ";
    }
};
} // namespace detail
} // namespace native
