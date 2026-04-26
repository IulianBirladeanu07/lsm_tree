#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace lsm {

class ConcurrentSkipList {
public:
    static constexpr int kMaxHeight = 12;

    struct Node {
        std::string key;
        std::string value;
        int height;
        std::array<std::atomic<Node*>, kMaxHeight> next{};

        Node(std::string k, std::string v, int h) : key(std::move(k)), value(std::move(v)), height(std::move(h)) {
            for (int i = 0; i < kMaxHeight; i++) {
                next[i].store(nullptr, std::memory_order_relaxed);
            }
        } 
    };

    ConcurrentSkipList() : head_(new Node("", "", kMaxHeight)), height_(1), elements_(0), rng_(std::random_device{}()) {}
    ~ConcurrentSkipList() {
        Node* current = head_.load(std::memory_order_relaxed);
        while (current) {
            Node* next = current->next[0].load(std::memory_order_relaxed);
            delete current;
            current = next;
        }
    }

    Node* find_previous(std::string_view key, std::array<Node*, kMaxHeight> &previous) {
            Node* current = head_.load(std::memory_order_acquire);
            for (int i = height_.load() - 1; i >= 0; i--) {
                Node* next_node = current->next[i].load(std::memory_order_acquire);
                while (next_node != nullptr && key > next_node->key) {
                    current = next_node;
                    next_node = current->next[i].load(std::memory_order_acquire);
                }
                previous[i] = current;
            }
            return current->next[0].load(std::memory_order_acquire);
        }

    int random_height() {
        int height = 1;
        while (height < kMaxHeight && std::uniform_int_distribution<int>(0, 1)(rng_) == 0) {
            height++;
        }
        return height;
    }

    void put(std::string key, std::string_view value) {
        std::lock_guard<std::mutex> lock(mutex_);

        std::array<Node*, kMaxHeight> previous{};
        Node* existing_node = find_previous(key, previous);

        if (existing_node != nullptr && existing_node->key == key) {
            existing_node->value = std::string(value);
            return;
        }
        int new_height = random_height();
        int current_height  = height_.load(std::memory_order_relaxed);

        if (new_height > current_height) {
            for(int i = current_height; i < new_height; i++ ) {
                previous[i] = head_.load(std::memory_order_relaxed);
            }
            height_.store(new_height, std::memory_order_relaxed);
        }

        Node* new_node = new Node(std::string(key), std::string(value), new_height);
        for (int i = 0; i < new_height; i++) {
            new_node->next[i].store(previous[i]->next[i].load(std::memory_order_relaxed), std::memory_order_relaxed);
            previous[i]->next[i].store(new_node, std::memory_order_release);
        }
        elements_.fetch_add(1, std::memory_order_relaxed);
    }

    std::optional<std::string> get(std::string_view key) const {
        Node* current = head_.load(std::memory_order_acquire);
        for (int i = height_.load(std::memory_order_relaxed) - 1; i >= 0; i--) {
            Node* candidate = current->next[i].load(std::memory_order_relaxed);
            while (candidate != nullptr && candidate->key < key) {
                current = candidate;
                candidate = current->next[i].load(std::memory_order_acquire);
            }
        }
        Node* candidate = current->next[0].load(std::memory_order_acquire);
        if (candidate != nullptr && candidate->key == key)
            return candidate->value;
        return std::nullopt;
    }

    size_t size() const {
        return static_cast<size_t>(elements_.load(std::memory_order_relaxed));
    }

    bool contains(std::string_view key) const {
        return get(key).has_value();
    }

private:
    std::atomic<Node*> head_;
    std::atomic<int> elements_;
    std::atomic<int> height_;
    std::mutex mutex_;
    std::mt19937 rng_;

};
} // namespace lsm