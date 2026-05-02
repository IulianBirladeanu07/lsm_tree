#include "lsm/iterator/merging_iterator.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <string>

namespace lsm {

class VectorIterator : public IIterator {
public:
    VectorIterator(std::vector<std::pair<std::string, std::string>> data)
        : data_(std::move(data)), idx_(0) {}

    bool valid() const override { return idx_ < data_.size(); }
    void next() override { if (valid()) ++idx_; }
    std::string_view key() const override { return data_[idx_].first; }
    std::string_view value() const override { return data_[idx_].second; }
    void seek(std::string_view key) override {
        idx_ = 0;
        while (idx_ < data_.size() && data_[idx_].first < key) ++idx_;
    }

private:
    std::vector<std::pair<std::string, std::string>> data_;
    std::size_t idx_;
};

} // namespace lsm

int main() {
    std::vector<std::unique_ptr<lsm::IIterator>> iters;
    iters.push_back(std::make_unique<lsm::VectorIterator>(
        std::vector<std::pair<std::string,std::string>>{{"b","2"},{"d","4"},{"f","6"}}
    ));
    iters.push_back(std::make_unique<lsm::VectorIterator>(
        std::vector<std::pair<std::string,std::string>>{{"a","1"},{"c","3"},{"e","5"}}
    ));

    lsm::MergingIterator mit(std::move(iters));

    std::vector<std::string> keys;
    while (mit.valid()) {
        keys.push_back(std::string(mit.key()));
        mit.next();
    }

    assert(keys.size() == 6);
    assert(keys[0] == "a");
    assert(keys[1] == "b");
    assert(keys[2] == "c");
    assert(keys[3] == "d");
    assert(keys[4] == "e");
    assert(keys[5] == "f");

    std::cout << "merge order: OK\n";

    std::vector<std::unique_ptr<lsm::IIterator>> iters2;
    iters2.push_back(std::make_unique<lsm::VectorIterator>(
        std::vector<std::pair<std::string,std::string>>{{"a","1"},{"c","3"},{"e","5"}}
    ));

    lsm::MergingIterator mit2(std::move(iters2));
    mit2.seek("c");
    assert(mit2.valid());
    assert(mit2.key() == "c");

    std::cout << "seek: OK\n";
    std::cout << "all tests passed\n";
    return 0;
}