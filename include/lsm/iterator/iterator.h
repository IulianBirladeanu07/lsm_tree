#pragma once
#include <string_view>

namespace lsm {

class IIterator {
public:
    virtual ~IIterator() = default;
    virtual bool valid() const = 0;
    virtual void next() = 0;
    virtual std::string_view key() const = 0;
    virtual std::string_view value() const = 0;
    virtual void seek(std::string_view key) = 0;
};

} // namespace lsm