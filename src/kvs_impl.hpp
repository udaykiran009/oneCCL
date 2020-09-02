#pragma once
#include "ccl_types.hpp"
#include "ccl_kvs.hpp"

namespace ccl
{

class kvs_impl
{
    //STUB
};

const kvs::addr_t& CCL_API kvs::get_addr() const
{
    //TODO: add logic;
    throw;
}

vector_class<char> CCL_API kvs::get(const string_class& prefix, const string_class& key) const
{
    //TODO: add logic;
    throw;
}

void CCL_API kvs::set(const string_class& prefix,
              const string_class& key,
              const vector_class<char>& data) const
{
    //TODO: add logic;
    throw;
}
CCL_API kvs::~kvs()
{
    //TODO: add logic;
}

CCL_API kvs::kvs(const kvs::addr_t& addr)
{
    //TODO: add logic;
}

CCL_API kvs::kvs()
{
    //TODO: add logic;
}


}
