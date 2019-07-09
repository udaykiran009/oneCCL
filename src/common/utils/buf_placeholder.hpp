#pragma once
#include <iostream>

class iccl_buf_placeholder
{
public:
    iccl_buf_placeholder(void** ptr, size_t offset): p_to_buf(ptr), offset(offset) { };
    iccl_buf_placeholder(void** ptr): p_to_buf(ptr), offset(0) { };
    iccl_buf_placeholder(): p_to_buf(nullptr), offset(0) { };
    iccl_buf_placeholder(const iccl_buf_placeholder &buf): p_to_buf(buf.p_to_buf), offset(buf.offset) { };
 
    void** p_to_buf;
    size_t offset;

    iccl_buf_placeholder operator+ (int val)
    {
        return iccl_buf_placeholder(p_to_buf, offset + val);
    }

    iccl_buf_placeholder operator- (int val)
    {
        return iccl_buf_placeholder(p_to_buf, offset - val);
    }

    iccl_buf_placeholder operator+ (size_t val)
    {
        return iccl_buf_placeholder(p_to_buf, offset + val);
    }

    iccl_buf_placeholder operator- (size_t val)
    {
        return iccl_buf_placeholder(p_to_buf, offset - val);
    }

    char* get_ptr() const
    {
        return (p_to_buf && *p_to_buf) ? ((char*)*p_to_buf + offset) : nullptr;
    }

    operator bool()
    {
        return (p_to_buf && *p_to_buf);
    }

    friend bool operator !=(iccl_buf_placeholder const & a, iccl_buf_placeholder const & b)
    {
        return !(a.get_ptr() == b.get_ptr());
    }

    friend bool operator ==(iccl_buf_placeholder const & a, iccl_buf_placeholder const & b)
    {
        return (a.get_ptr() == b.get_ptr());
    }

    friend bool operator >(iccl_buf_placeholder const & a, iccl_buf_placeholder const & b)
    {
        return (a.get_ptr() > b.get_ptr());
    }

    friend std::ostream& operator<< (std::ostream &out, const iccl_buf_placeholder &buf)
    {
        out << buf.get_ptr();
        return out;
    }

};