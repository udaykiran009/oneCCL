#pragma once

#include <mutex>
#include <unordered_map>
#include <utility>

#include "oneapi/ccl/ccl_types.hpp"
#include "common/log/log.hpp"
#include "common/utils/spinlock.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_datatype_attr_ids.hpp"
#include "oneapi/ccl/ccl_datatype_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_datatype_attr.hpp"

class ccl_datatype {
public:
    ccl_datatype(ccl_datatype_t idx, size_t size);
    ccl_datatype() = default;
    ~ccl_datatype() = default;
    ccl_datatype& operator=(const ccl_datatype& other) = default;

    ccl_datatype(const ccl_datatype& other) = default;

    // ccl_datatype_t idx() const
    // {
    //     return m_idx;
    // }

    ccl::datatype idx() const {
        return (ccl::datatype)(m_idx);
    }

    size_t size() const {
        CCL_THROW_IF_NOT(m_size > 0, "non-positive datatype size ", m_size);
        return m_size;
    }

private:
    ccl_datatype_t m_idx;
    size_t m_size;
};

/* frequently used in multiple places */
extern ccl_datatype ccl_datatype_char;

using ccl_datatype_lock_t = ccl_spinlock;

using ccl_datatype_table_t =
    std::unordered_map<ccl_datatype_t, std::pair<ccl_datatype, std::string>>;

class ccl_datatype_storage {
public:
    ccl_datatype_storage();
    ~ccl_datatype_storage();

    ccl_datatype_storage(const ccl_datatype_storage& other) = delete;
    ccl_datatype_storage& operator=(const ccl_datatype_storage& other) = delete;

    ccl::datatype create(const ccl::datatype_attr& attr);
    void free(ccl::datatype idx);
    void free(ccl_datatype_t idx);

    const ccl_datatype& get(ccl_datatype_t idx) const;
    const ccl_datatype& get(ccl::datatype idx) const;

    const std::string& name(const ccl_datatype& dtype) const;
    const std::string& name(ccl_datatype_t idx) const;

    static bool is_predefined_datatype(ccl::datatype idx);

private:
    ccl::datatype create_by_datatype_size(size_t datatype_size);
    void create_internal(ccl_datatype_table_t& table,
                         size_t idx,
                         size_t size,
                         const std::string& name);

    mutable ccl_datatype_lock_t guard{};

    ccl_datatype_t custom_idx;

    ccl_datatype_table_t predefined_table;
    ccl_datatype_table_t custom_table;
};
