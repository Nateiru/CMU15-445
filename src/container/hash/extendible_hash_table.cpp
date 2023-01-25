//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_hash_table.cpp
//
// Identification: src/container/hash/extendible_hash_table.cpp
//
// Copyright (c) 2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include <cstdlib>
#include <functional>
#include <list>
#include <utility>

#include "container/hash/extendible_hash_table.h"
#include "storage/page/page.h"

namespace bustub {

template <typename K, typename V>
ExtendibleHashTable<K, V>::ExtendibleHashTable(size_t bucket_size)
    : global_depth_(0), bucket_size_(bucket_size), num_buckets_(1),
      dir_(1, std::make_shared<Bucket>(bucket_size)) {}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::IndexOf(const K &key) -> size_t {
  int mask = (1 << global_depth_) - 1;
  return std::hash<K>()(key) & mask;
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetGlobalDepth() const -> int {
  // std::scoped_lock<std::mutex> lock(latch_);
  return GetGlobalDepthInternal();
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetGlobalDepthInternal() const -> int {
  return global_depth_;
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetLocalDepth(int dir_index) const -> int {
  // std::scoped_lock<std::mutex> lock(latch_);
  return GetLocalDepthInternal(dir_index);
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetLocalDepthInternal(int dir_index) const -> int {
  return dir_[dir_index]->GetDepth();
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetNumBuckets() const -> int {
  // std::scoped_lock<std::mutex> lock(latch_);
  return GetNumBucketsInternal();
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetNumBucketsInternal() const -> int {
  return num_buckets_;
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Find(const K &key, V &value) -> bool {
  // std::lock_guard<std::mutex> guard(latch_);
  auto target_bucket = dir_[IndexOf(key)];
  return target_bucket->Find(key, value);
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Remove(const K &key) -> bool {
  // std::lock_guard<std::mutex> guard(latch_);
  auto target_bucket = dir_[IndexOf(key)];
  return target_bucket->Remove(key);
}

template <typename K, typename V>
void ExtendibleHashTable<K, V>::Insert(const K &key, const V &value) {
  std::lock_guard<std::mutex> guard(latch_);
  // 桶满 需要扩容
  while(dir_[IndexOf(key)]->IsFull()) {
    assert(dir_[IndexOf(key)]->IsFull());

    auto full_bucket = dir_[IndexOf(key)];
    // 全局深度 = 局部深度 先扩增dir_
    if (GetGlobalDepth() == full_bucket->GetDepth()) {
      global_depth_++;
      size_t capacity = dir_.size();
      dir_.resize(capacity << 1);
      for (size_t i = 0; i < capacity; ++i) {
        dir_[i + capacity] = dir_[i];
      }
    }
    // 桶分裂
    unsigned int mask = 1 << full_bucket->GetDepth();
    auto zero_bucket = std::make_shared<Bucket>(bucket_size_, full_bucket->GetDepth() + 1);
    auto one_bucket = std::make_shared<Bucket>(bucket_size_, full_bucket->GetDepth() + 1);
    for (const auto &item : full_bucket->GetItems()) {
      size_t hashkey = std::hash<K>()(item.first);
      if ((hashkey & mask) != 0U) {
        one_bucket->Insert(item.first, item.second);
      } else {
        zero_bucket->Insert(item.first, item.second);
      }
    } 
    // // 分裂完成 
    num_buckets_++;

    for (size_t i = 0; i < dir_.size(); i++) {
      if (dir_[i] == full_bucket) {
        if ((i & mask) != 0U) {
          dir_[i] = one_bucket;
        }else {
          dir_[i] = zero_bucket;
        }
      }
    }
  }
  // 桶没满
  auto index = IndexOf(key);
  auto target_bucket = dir_[index];
  // 更新
  target_bucket->Insert(key, value);
}

//===--------------------------------------------------------------------===//
// Bucket
//===--------------------------------------------------------------------===//
template <typename K, typename V>
ExtendibleHashTable<K, V>::Bucket::Bucket(size_t array_size, int depth) : size_(array_size), depth_(depth) {}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Bucket::Find(const K &key, V &value) -> bool {
  for (const auto &item : list_) {
    if (item.first == key) {
      value = item.second;
      return true;
    }
  } 
  return false;
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Bucket::Remove(const K &key) -> bool {
  for (auto it = list_.begin(); it != list_.end(); ++it) {
    if (it->first == key) {
      list_.erase(it);
      return true;
    }
  }
  return false;
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Bucket::Insert(const K &key, const V &value) -> bool {
  for (auto &item : list_) {
    if (item.first == key) {
      item.second = value;
      return true;
    }
  }
  list_.emplace_back(key, value);
  // 什么情况下插入失败？
  return true;
}

template class ExtendibleHashTable<page_id_t, Page *>;
template class ExtendibleHashTable<Page *, std::list<Page *>::iterator>;
template class ExtendibleHashTable<int, int>;
// test purpose
template class ExtendibleHashTable<int, std::string>;
template class ExtendibleHashTable<int, std::list<int>::iterator>;

}  // namespace bustub
