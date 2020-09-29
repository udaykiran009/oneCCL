#pragma once

//#include <CL/sycl.hpp>
#include <exception>
#include <string>

namespace ccl {

class exception : public std::exception {
    std::string msg;

public:
    exception(const std::string &domain, const std::string &function, const std::string &info = "")
            : std::exception() {
        msg = std::string("oneCCL: ") + domain +
               ((domain.length() != 0 && function.length() != 0) ? "/" : "") + function +
               ((info.length() != 0)
                    ? (((domain.length() + function.length() != 0) ? ": " : "") + info)
                    : "");
    }

    exception(const std::string &info = "")
            : std::exception() {
        msg = std::string("oneCCL: ") + info;
    }

    exception(const char* info)
            : std::exception() {
        msg = std::string("oneCCL: ") + std::string(info);
    }

    const char *what() const noexcept {
        return msg.c_str();
    }
};

class invalid_argument : public ccl::exception {
public:
    invalid_argument(const std::string &domain, const std::string &function,
                     const std::string &info = "")
            : ccl::exception(domain, function, "invalid argument " + info) {}
};

class host_bad_alloc : public ccl::exception {
public:
    host_bad_alloc(const std::string &domain, const std::string &function)
            : ccl::exception(domain, function, "cannot allocate memory on host") {}
};

// class device_bad_alloc : public ccl::exception {
// public:
//     device_bad_alloc(const std::string &domain, const std::string &function,
//                      const cl::sycl::device &device)
//             : ccl::exception(
//                   domain, function,
//                   "cannot allocate memory on " + device.get_info<cl::sycl::info::device::name>()) {}
// };

class unimplemented : public ccl::exception {
public:
    unimplemented(const std::string &domain, const std::string &function,
                  const std::string &info = "")
            : ccl::exception(domain, function, "function is not implemented " + info) {}
};

class unsupported : public ccl::exception {
public:
    unsupported(const std::string &domain, const std::string &function,
                  const std::string &info = "")
            : ccl::exception(domain, function, "function is not supported " + info) {}
};

}
