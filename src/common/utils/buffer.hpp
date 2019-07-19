#pragma once

#include <iostream>
#include <stddef.h>

enum class iccl_buffer_type
{
    DIRECT = 0,
    INDIRECT
};

inline std::ostream& operator << (std::ostream& os, const iccl_buffer_type& type)
{
   os << static_cast<std::underlying_type<iccl_buffer_type>::type>(type);
   return os;
}

class iccl_buffer
{

private:

    void* src;
    ssize_t size;
    int offset;
    iccl_buffer_type type;

public:

    iccl_buffer(void* src, ssize_t size, int offset, iccl_buffer_type type)
        : src(src), size(size),
          offset(offset), type(type)
    {
        LOG_DEBUG("create iccl_buffer: src ", src, ", size ", size, ", offset ", offset, ", type ", type);
        ICCL_ASSERT(offset >= 0, "unexpected offset");
        ICCL_ASSERT((size == -1) || (offset <= size), "unexpected offset ", offset, ", size ", size);
    }

    iccl_buffer() : iccl_buffer(nullptr, -1, 0, iccl_buffer_type::DIRECT) {}
    iccl_buffer(void* src, ssize_t size) : iccl_buffer(src, size, 0, iccl_buffer_type::DIRECT) {}
    iccl_buffer(void* src, ssize_t size, int offset) : iccl_buffer(src, size, offset, iccl_buffer_type::DIRECT) {}
    iccl_buffer(void* src, ssize_t size, iccl_buffer_type type) : iccl_buffer(src, size, 0, type) {}

    iccl_buffer(const iccl_buffer& buf)
        : src(buf.src),
          size(buf.size),
          offset(buf.offset),
          type(buf.type)
    {
        ICCL_ASSERT(offset >= 0, "unexpected offset");
        ICCL_ASSERT((size == -1) || (offset <= size), "unexpected offset ", offset, ", size ", size);
    };

    void set(void* src, ssize_t size, int offset, iccl_buffer_type type)
    {
        LOG_DEBUG("set iccl_buffer: src ", src, ", size ", size, ", offset ", offset, ", type ", type);
        ICCL_ASSERT(src, "new src is null");
        ICCL_ASSERT(offset >= 0, "unexpected offset");

        this->src = src;
        this->size = size;
        this->offset = offset;
        this->type = type;
    }

    void set(void* src) { set(src, -1, 0, iccl_buffer_type::DIRECT); }
    void set(void* src, ssize_t size) { set(src, size, 0, iccl_buffer_type::DIRECT); }
    void set(void* src, ssize_t size, iccl_buffer_type type) { set(src, size, 0, type); }
    void set(void* src, ssize_t size, int offset) { set(src, size, offset, iccl_buffer_type::DIRECT); }

    void* get_src() const { return src; }
    ssize_t get_size() const { return size; }
    int get_offset() const { return offset; }
    iccl_buffer_type get_type() const { return type; }

    iccl_buffer operator+ (size_t val)
    {
        return iccl_buffer(src, size, offset + val, type);
    }

    iccl_buffer operator- (size_t val)
    {
        return iccl_buffer(src, size, offset - val, type);
    }

    iccl_buffer operator+ (int val)
    {
        return iccl_buffer(src, size, offset + val, type);
    }

    iccl_buffer operator- (int val)
    {
        return iccl_buffer(src, size, offset - val, type);
    }

    void* get_ptr(ssize_t access_offset = 0) const
    {
        ICCL_ASSERT(offset >= 0, "unexpected size");
        ICCL_ASSERT((size == -1) || (offset + access_offset <= size),
                    "unexpected access: size ", size,
                    ", offset ", offset,
                    ", access_offset ", access_offset);

        if (!src)
            return nullptr;

        if (type == iccl_buffer_type::DIRECT)
            return ((char*)src + offset);
        else
        {
            return (*((char**)src)) ? (*((char**)src) + offset) : nullptr;
        }
    }

    operator bool() const
    {
        if (type == iccl_buffer_type::DIRECT)
            return src;
        else
            return (src && (*(void**)src));
    }

    bool operator ==(iccl_buffer const& other) const
    {
        return ((get_ptr() == other.get_ptr()) &&
                (get_type() == other.get_type() &&
                (get_size() == other.get_size())));
    }

    bool operator !=(iccl_buffer const& other) const
    {
        return !(*this == other);
    }

    bool operator >(iccl_buffer const& other) const
    {
        ICCL_ASSERT(get_type() == other.get_type(), "types should match");
        return (get_ptr() > other.get_ptr());
    }

    friend std::ostream& operator<< (std::ostream& out, const iccl_buffer& buf)
    {
        out << "src: " << buf.get_src()
            << ", size " << buf.get_size()
            << ", off " << buf.get_offset()
            << ", type: " << buf.get_type();
        return out;
    }
};
