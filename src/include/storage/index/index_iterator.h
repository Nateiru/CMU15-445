//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/include/index/index_iterator.h
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
/**
 * index_iterator.h
 * For range scan of b+ tree
 */
#pragma once
#include "storage/page/b_plus_tree_leaf_page.h"

namespace bustub {

#define INDEXITERATOR_TYPE IndexIterator<KeyType, ValueType, KeyComparator>

INDEX_TEMPLATE_ARGUMENTS
class IndexIterator {
 public:
  // you may define your own constructor based on your member variables
  explicit IndexIterator(B_PLUS_TREE_LEAF_PAGE_TYPE *leaf = nullptr, int index = 0,
                         BufferPoolManager *buffer_pool_manager = nullptr);
  ~IndexIterator();  // NOLINT

  auto IsEnd() -> bool;

  auto operator*() -> const MappingType &;

  auto operator++() -> IndexIterator &;

  auto operator==(const IndexIterator &itr) const -> bool { return ((leaf_ == itr.leaf_) && (index_ == itr.index_)); }

  auto operator!=(const IndexIterator &itr) const -> bool { return ((leaf_ != itr.leaf_) || (index_ != itr.index_)); }

  explicit IndexIterator(const IndexIterator &rhs) noexcept : 
                      leaf_(rhs.leaf_), index_(rhs.index_), buffer_pool_manager_(rhs.buffer_pool_manager_){
    if (leaf_->GetPageId() != INVALID_PAGE_ID) {
      buffer_pool_manager_->FetchPage(leaf_->GetPageId())->RLatch();
    }
  }
                
  friend void swap(IndexIterator &a, IndexIterator &b) noexcept {
    std::swap(a.leaf_, b.leaf_);
    std::swap(a.index_, b.index_);
    std::swap(a.buffer_pool_manager_, b.buffer_pool_manager_);
  }

  auto operator=(IndexIterator itr) noexcept -> IndexIterator & {
    swap(*this, itr);
    return *this;
  }

 private:
  // add your own private member variables here
  void UnlatchAndUnPin() {
    if (leaf_ == nullptr) {
      return;
    }
    buffer_pool_manager_->FetchPage(leaf_->GetPageId())->RUnlatch();
    buffer_pool_manager_->UnpinPage(leaf_->GetPageId(), false);
    buffer_pool_manager_->UnpinPage(leaf_->GetPageId(), false);
  }
  B_PLUS_TREE_LEAF_PAGE_TYPE *leaf_;
  int index_;
  BufferPoolManager *buffer_pool_manager_;
};

}  // namespace bustub
