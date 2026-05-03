#include <gtest/gtest.h>
#include "lsm/iterator/merging_iterator.h"

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

}

static std::vector<std::unique_ptr<lsm::IIterator>> make_iters(
    std::vector<std::vector<std::pair<std::string, std::string>>> sources)
{
    std::vector<std::unique_ptr<lsm::IIterator>> iters;
    for (auto& src : sources) {
        iters.push_back(std::make_unique<lsm::VectorIterator>(std::move(src)));
    }
    return iters;
}

TEST(MergingIterator, MergesTwoSortedSources) {
    auto iters = make_iters({
        {{"b", "2"}, {"d", "4"}, {"f", "6"}},
        {{"a", "1"}, {"c", "3"}, {"e", "5"}}
    });

    lsm::MergingIterator mit(std::move(iters));

    std::vector<std::string> keys;
    while (mit.valid()) {
        keys.push_back(std::string(mit.key()));
        mit.next();
    }

    ASSERT_EQ(keys.size(), 6u);
    EXPECT_EQ(keys, (std::vector<std::string>{"a", "b", "c", "d", "e", "f"}));
}

TEST(MergingIterator, EmptyIterators) {
    auto iters = make_iters({});
    lsm::MergingIterator mit(std::move(iters));
    EXPECT_FALSE(mit.valid());
}

TEST(MergingIterator, SingleSource) {
    auto iters = make_iters({{{"a", "1"}, {"b", "2"}, {"c", "3"}}});
    lsm::MergingIterator mit(std::move(iters));

    std::vector<std::string> keys;
    while (mit.valid()) {
        keys.push_back(std::string(mit.key()));
        mit.next();
    }

    EXPECT_EQ(keys, (std::vector<std::string>{"a", "b", "c"}));
}

TEST(MergingIterator, Seek) {
    auto iters = make_iters({{{"a", "1"}, {"c", "3"}, {"e", "5"}}});
    lsm::MergingIterator mit(std::move(iters));

    mit.seek("c");
    ASSERT_TRUE(mit.valid());
    EXPECT_EQ(mit.key(), "c");
}

TEST(MergingIterator, SeekPastEnd) {
    auto iters = make_iters({{{"a", "1"}, {"b", "2"}}});
    lsm::MergingIterator mit(std::move(iters));

    mit.seek("z");
    EXPECT_FALSE(mit.valid());
}

TEST(MergingIterator, ThreeSources) {
    auto iters = make_iters({
        {{"a", "1"}, {"d", "4"}},
        {{"b", "2"}, {"e", "5"}},
        {{"c", "3"}, {"f", "6"}}
    });

    lsm::MergingIterator mit(std::move(iters));

    std::vector<std::string> keys;
    while (mit.valid()) {
        keys.push_back(std::string(mit.key()));
        mit.next();
    }

    EXPECT_EQ(keys, (std::vector<std::string>{"a", "b", "c", "d", "e", "f"}));
}
