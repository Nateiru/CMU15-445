//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/page/b_plus_tree_leaf_page.cpp
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <sstream>
#include "storage/page/b_plus_tree_internal_page.h"

#include "common/exception.h"
#include "common/rid.h"
#include "storage/page/b_plus_tree_leaf_page.h"

namespace bustub {

/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/

/**
 * Init method after creating a new leaf page
 * Including set page type, set current size to zero, set page id/parent id, set
 * next page id and set max size
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::Init(page_id_t page_id, page_id_t parent_id, int max_size) {
  SetPageType(IndexPageType::LEAF_PAGE);
  SetSize(0);
  SetPageId(page_id);
  SetParentPageId(parent_id);
  SetMaxSize(max_size);
  SetNextPageId(INVALID_PAGE_ID);
}

/**
 * Helper methods to set/get next page id
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetNextPageId() const -> page_id_t { return next_page_id_; }

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::SetNextPageId(page_id_t next_page_id) { next_page_id_ = next_page_id; }

/*
 * Helper method to find and return the key associated with input "index"(a.k.a
 * array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::KeyAt(int index) const -> KeyType {
  assert(index >= 0);
  return array_[index].first;
}

/**
 * Helper method to find the first index i so that array_[i].first >= key
 * if key > ALL(array_) return GetSize()
 * NOTE: This method is only used when generating index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::KeyIndex(const KeyType &key, const KeyComparator &comparator) const -> int {
  assert(GetSize() >= 0);
  int st = 0;
  int ed = GetSize() - 1;
  while (st <= ed) {  // find the last key in array <= input
    int mid = (ed - st) / 2 + st;
    if (comparator(array_[mid].first, key) >= 0) {
      ed = mid - 1;
    } else {
      st = mid + 1;
    }
  }
  return ed + 1;
}
/*
 * Helper method to find and return the value associated with input "index"(a.k.a
 * array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::ValueAt(int index) const -> ValueType {
  assert(index >= 0);
  return array_[index].second;
}
/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert key & value pair into leaf page ordered by key
 * @return  page size after insertion
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::Insert(const KeyType &key, const ValueType &value, const KeyComparator &comparator)
    -> int {
  int idx = KeyIndex(key, comparator);  // first larger than key
  assert(idx >= 0);
  IncreaseSize(1);
  int cur_size = GetSize();
  for (int i = cur_size - 1; i > idx; --i) {
    array_[i] = array_[i - 1];
  }
  array_[idx] = MappingType{key, value};
  return cur_size;
}
/*****************************************************************************
 * LOOKUP
 *****************************************************************************/
/*
 * For the given key, check to see whether it exists in the leaf page. If it
 * does, then store its corresponding value in input "value" and return true.
 * If the key does not exist, then return false
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::Lookup(const KeyType &key, ValueType &value, const KeyComparator &comparator) const
    -> bool {
  int idx = KeyIndex(key, comparator);
  if (idx < GetSize() && comparator(array_[idx].first, key) == 0) {
    value = array_[idx].second;
    return true;
  }
  return false;
}
/*****************************************************************************
 * SPLIT
 *****************************************************************************/
/*
 * Remove half of key & value pairs from this page to "recipient" page
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveHalfTo(BPlusTreeLeafPage *recipient,
                                            __attribute__((unused)) BufferPoolManager *buffer_pool_manager) {
  assert(recipient != nullptr);
  // assert(GetSize() == GetMaxSize());
  // copy last half [mid, size) 移动到新的页
  int total = GetSize();
  int copy_start_index = total >> 1;
  for (int i = copy_start_index; i < total; i++) {
    recipient->array_[i - copy_start_index] = array_[i];
  }
  // set pointer
  recipient->SetNextPageId(GetNextPageId());
  SetNextPageId(recipient->GetPageId());
  // set size, is odd, bigger is last part
  SetSize(copy_start_index);
  recipient->SetSize(total - copy_start_index);
}
/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * First look through leaf page to see whether delete key exist or not. If
 * exist, perform deletion, otherwise return immdiately.
 * NOTE: store key&value pair continuously after deletion
 * @return   page size after deletion
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::RemoveAndDeleteRecord(const KeyType &key, const KeyComparator &comparator) -> int {
  // 第一个 >= key 的索引
  auto first_lge_idx = KeyIndex(key, comparator);
  if (first_lge_idx >= GetSize() || comparator(KeyAt(first_lge_idx), key) != 0) {
    return GetSize();
  }
  // 存在 key
  for (int i = first_lge_idx; i < GetSize() - 1; ++i) {
    array_[i] = array_[i + 1];
  }
  IncreaseSize(-1);
  return GetSize();
}
/*****************************************************************************
 * MERGE
 *****************************************************************************/
/*
 * Remove all of key & value pairs from this page to "recipient" page, then
 * update next page id
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveAllTo(BPlusTreeLeafPage *recipient, int /*unused*/,
                                           BufferPoolManager * /*unused*/) {
  assert(recipient != nullptr);
  int start_index = recipient->GetSize();
  for (int i = 0; i < GetSize(); i++) {
    recipient->array_[i + start_index] = array_[i];
  }
  // set pointer
  recipient->SetNextPageId(GetNextPageId());
  // add size
  recipient->IncreaseSize(GetSize());
  SetSize(0);
}
/*****************************************************************************
 * REDISTRIBUTE
 *****************************************************************************/
/*
 * Remove the first key & value pair from this page to "recipient" page, then
 * update relavent key & value pair in its parent page.
 */
// recipient <= this
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveFirstToEndOf(BPlusTreeLeafPage *recipient,
                                                  BufferPoolManager *buffer_pool_manager) {
  assert(recipient->GetSize() + 1 <= recipient->GetMaxSize());
  assert(GetSize() >= 1);
  recipient->array_[GetSize()] = MappingType{KeyAt(0), ValueAt(0)};
  recipient->IncreaseSize(1);
  IncreaseSize(-1);
  for (int i = 0; i < GetSize(); i++) {
    array_[i] = array_[i + 1];
  }
  // update relavent key & value pair in its parent page.
  // Page *page = buffer_pool_manager->FetchPage(GetParentPageId());
  // auto parent_node = reinterpret_cast<B_PLUS_TREE_INTERNAL_PAGE_TYPE *>(page->GetData());
  // parent_node->SetKeyAt(parent_node->ValueIndex(GetPageId()), array_[0].first);
  // buffer_pool_manager->UnpinPage(GetParentPageId(), true);
}
/*
 * Remove the last key & value pair from this page to "recipient" page, then
 * update relavent key & value pair in its parent page.
 */
// this <= recipient
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveLastToFrontOf(BPlusTreeLeafPage *recipient, int parent_index,
                                                   BufferPoolManager *buffer_pool_manager) {
  assert(recipient->GetSize() + 1 <= recipient->GetMaxSize());
  assert(GetSize() >= 1);
  for (int i = 1; i < GetSize(); i++) {
    recipient->array_[i] = recipient->array_[i - 1];
  }
  recipient->IncreaseSize(1);
  recipient->array_[0] = MappingType{KeyAt(GetSize() - 1), ValueAt(GetSize() - 1)};
  IncreaseSize(-1);
  // update relavent key & value pair in its parent page.
  // Page *page = buffer_pool_manager->FetchPage(recipient->GetParentPageId());
  // auto parent_node = reinterpret_cast<B_PLUS_TREE_INTERNAL_PAGE_TYPE *>(page->GetData());
  // parent_node->SetKeyAt(parent_index, recipient->KeyAt(0));
  // buffer_pool_manager->UnpinPage(recipient->GetParentPageId(), true);
}
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::Debug() {
  std::cout << "DEBUG +++++++++++++++++\n";
  for (int i = 0; i < GetSize(); i++) {
    std::cout << array_[i].first << ' ';
  }
  std::cout << "\nDEBUG +++++++++++++++++\n\n";
}

template class BPlusTreeLeafPage<GenericKey<4>, RID, GenericComparator<4>>;
template class BPlusTreeLeafPage<GenericKey<8>, RID, GenericComparator<8>>;
template class BPlusTreeLeafPage<GenericKey<16>, RID, GenericComparator<16>>;
template class BPlusTreeLeafPage<GenericKey<32>, RID, GenericComparator<32>>;
template class BPlusTreeLeafPage<GenericKey<64>, RID, GenericComparator<64>>;
}  // namespace bustub
