#pragma once

#include "iccl_types.hpp"

#include <iostream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <cstring>
#include <assert.h>

#define __FILENAME__                                                        \
({                                                                          \
        const char *ptr = strrchr(__FILE__, '/');                           \
        if(ptr)                                                             \
        {                                                                   \
            ++ptr;                                                          \
        }                                                                   \
        else                                                                \
        {                                                                   \
            ptr = __FILE__;                                                 \
        }                                                                   \
        ptr;                                                                \
})

constexpr size_t LOGGER_BUFFER_SIZE = 2048;

enum class iccl_log_level
{
    ERROR = 0,
    INFO,
    DEBUG,
    TRACE
};

/**
 * Wrapper over streambuf class to provide presistent buffer
 */
class iccl_streambuf : public std::streambuf
{
public:
    explicit iccl_streambuf(size_t s) : size(s), buffer(new char[size])
    {
        reset();
    }

    iccl_streambuf(const iccl_streambuf& other) = delete;
    iccl_streambuf(iccl_streambuf&& other) = delete;

    iccl_streambuf& operator=(const iccl_streambuf& other) = delete;
    iccl_streambuf& operator=(iccl_streambuf&& other) = delete;

    friend std::ostream& operator<<(std::ostream& os,
                                    iccl_streambuf& buf);

private:
    size_t size;
    std::unique_ptr<char[]> buffer;

    void reset()
    {
        //reset pointer to start/cur/end positions in streambuf
        setp(buffer.get(), buffer.get() + size);
    }

    void set_eol()
    {
        auto pos = pptr();
        if (pos)
        {
            *pos = '\0';
        }
    }
};

/**
 * Logging interface
 * All methods support c++ stream format listed in <iomanip> or https://en.cppreference.com/w/cpp/io/manip
 *
 * Set witdth:
 * printf approach:
 * printf("Formatted digit: %10d, usual format: %d\n", 123, 234);
 * new approach:
 * log.info("Formatted digit:", std::setw(10), "123", ", usual format: ", 234);
 *
 * Floating point precision:
 * printf approach:
 * printf("Formatted digit: %.4f\n", 213,345355);
 * new approach:
 * To format output of floating point types one can use std::fixed with std::setprecision or std::scientific
 * log.info("Formatted digit: ", std::setprecision(4), 213,345355);
 *
 * To format base of digital values one can use std::dec, std::hex, std::oct as a parameter of logger interface
 * To set justification one can use std::left, std::right, std::internal as a parameter of logger interface
 */
class iccl_logger
{
public:
    iccl_logger() :
        streambuf(LOGGER_BUFFER_SIZE),
        out_stream(&streambuf),
        initial_flags(out_stream.flags())
    {
    }

    iccl_logger(const iccl_logger& other) = delete;
    iccl_logger(iccl_logger&& other) = delete;

    iccl_logger& operator=(const iccl_logger& other) = delete;
    iccl_logger& operator=(iccl_logger&& other) = delete;

    static void set_log_level(iccl_log_level lvl)
    {
        level = lvl;
    }

    template<typename T, typename ...Tpackage>
    void error(T&& first,
               Tpackage&& ... others)
    {
        if (level >= iccl_log_level::ERROR)
        {
            write_stream_wrapper(out_stream, std::cerr, "ERROR: ",
                                 std::forward<T>(first), std::forward<Tpackage>(others)...);

            write_backtrace(out_stream);
            std::cerr << streambuf;
            std::flush(std::cerr);

            out_stream.flags(initial_flags);
        }
    }

    template<typename T, typename ...Tpackage>
    void info(T&& first,
              Tpackage&& ... others)
    {
        if (level >= iccl_log_level::INFO)
        {
            write_stream_wrapper(out_stream, std::cout, std::forward<T>(first),
                                 std::forward<Tpackage>(others)...);
        }
    }

    template<typename T, typename ...Tpackage>
    void debug(T&& first,
               Tpackage&& ... others)
    {
        if (level >= iccl_log_level::DEBUG)
        {
            write_stream_wrapper(out_stream, std::cout, std::forward<T>(first),
                                 std::forward<Tpackage>(others)...);
        }
    }

    template<typename T, typename ...Tpackage>
    void trace(T&& first,
               Tpackage&& ... others)
    {
        if (level >= iccl_log_level::TRACE)
        {
            write_stream_wrapper(out_stream, std::cout, std::forward<T>(first),
                                 std::forward<Tpackage>(others)...);
        }
    }

    /**
     * General purpose method, used to fill @b stream with content
     * @tparam stream any type that supports << operator as a left hand operand
     * @tparam T any type that has << operator
     * @tparam Tpackage any package of types that support << operator
     */
    template<typename stream, typename T, typename ...Tpackage>
    static void format(stream& ss,
                       T&& first,
                       Tpackage&& ... others)
    {
        write_stream(ss, std::forward<T>(first), std::forward<Tpackage>(others)...);
    }


private:
    static iccl_log_level level;

    iccl_streambuf streambuf;
    std::ostream out_stream;
    std::ios::fmtflags initial_flags;

    template<typename stream, typename T>
    static void write_stream(stream& ss,
                             T&& tail)
    {
        ss << tail;
    }

    template<typename stream, typename T, typename ...Tpackage>
    static void write_stream(stream& ss,
                             T&& first,
                             Tpackage&& ...others)
    {
        ss << first;
        write_stream(ss, std::forward<Tpackage>(others)...);
    }

    /**
     * Internal wrapper over write_stream methods. Formats message header, writes arguments to stream and redirects
     * result to the passed output stream
     */
    template<typename stream, typename output, typename T, typename ...Tpackage>
    void write_stream_wrapper(stream& ss,
                              output& out,
                              T&& first,
                              Tpackage&& ...others)
    {
        write_prefix(ss);
        write_stream(ss, std::forward<T>(first), std::forward<Tpackage>(others)...);
        out << streambuf << std::endl;

        ss.flags(initial_flags);
    }

    static void write_prefix(std::ostream& str);

    static void write_backtrace(std::ostream& str);
};

extern thread_local iccl_logger logger;

#define LOG_ERROR(...) logger.error(__FILENAME__,":", __FUNCTION__, ":", __LINE__, " ", ##__VA_ARGS__);
#define LOG_INFO(...) logger.info( __FUNCTION__, ":", __LINE__, " ", ##__VA_ARGS__);
#define LOG_DEBUG(...) logger.debug( __FUNCTION__, ":", __LINE__, " ", ##__VA_ARGS__);
#define LOG_TRACE(...) logger.trace( __FUNCTION__, ":", __LINE__, " ", ##__VA_ARGS__);

/**
 * Macro to handle critical unrecoverable error. Can be used in destructors
 */
#define ICCL_FATAL(...)                                                 \
do                                                                      \
{                                                                       \
    LOG_ERROR(__VA_ARGS__)                                              \
    std::terminate();                                                   \
} while(0)


/**
 * Helper macro to throw iccl::iccl_error exception. Must never be used in destructors
 */
#define ICCL_THROW(...)                                                              \
do                                                                                   \
{                                                                                    \
    std::stringstream throw_msg_ss;                                                  \
    iccl_logger::format(throw_msg_ss, __FILENAME__, ":", __FUNCTION__, ":", __LINE__,\
        ": EXCEPTION: " , ##__VA_ARGS__);                                            \
    throw iccl::iccl_error(throw_msg_ss.str());                                      \
} while(0)

/**
 * Helper macro to throw iccl::iccl_error exception if provided condition is not true.
 * Must never be used in destructors
 */
#define ICCL_THROW_IF_NOT(cond, ...)                                              \
do                                                                                \
{                                                                                 \
    if (!(cond))                                                                  \
    {                                                                             \
        LOG_ERROR("condition ", #cond, " failed\n", ##__VA_ARGS__);               \
        ICCL_THROW(__VA_ARGS__);                                                  \
    }                                                                             \
} while(0)


#define ICCL_UNUSED(expr) do { (void)sizeof(expr); } while(0)

#ifdef ENABLE_DEBUG

/**
 * Raises failed assertion if provided condition is not true. Works in debug build only
 */
#define ICCL_ASSERT(cond, ...)                                                          \
do                                                                                      \
{                                                                                       \
    if (!(cond))                                                                        \
    {                                                                                   \
        LOG_ERROR("ASSERT failed, cond:  ", #cond, " ", ##__VA_ARGS__);                 \
        assert(0);                                                                      \
    }                                                                                   \
} while(0)

#else

/**
 * Raises failed assertion if provided condition is not true. Works in debug build only
 */
#define ICCL_ASSERT(cond, ...) ICCL_UNUSED(cond)

#endif
