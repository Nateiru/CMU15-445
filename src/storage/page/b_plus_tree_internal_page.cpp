//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/page/b_plus_tree_internal_page.cpp
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <sstream>

#include "common/exception.h"
#include "storage/page/b_plus_tree_internal_page.h"

namespace bustub {
/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/
/*
 * Init method after creating a new internal page
 * Including set page type, set current size, set page id, set parent id and set
 * max page size
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Init(page_id_t page_id, page_id_t parent_id, int max_size) {
  SetPageType(IndexPageType::INTERNAL_PAGE);
  SetSize(0);
  SetPageId(page_id);
  SetParentPageId(parent_id);
  SetMaxSize(max_size);
}
/*
 * Helper method to get/set the key associated with input "index"(a.k.a
 * array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::KeyAt(int index) const -> KeyType {
  // replace with your own code
  return array_[index].first;
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetKeyAt(int index, const KeyType &key) { array_[index].first = key; }

/*
 * Helper method to get the value associated with input "index"(a.k.a array
 * offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueAt(int index) const -> ValueType { return array_[index].second; }

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetValueAt(int index, const ValueType &value) { array_[index].second = value; }
/*
 * Helper method to find and return array index(or offset), so that its value
 * equals to input "value"
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueIndex(const ValueType &value) const -> int {
  for (int i = 0; i < GetSize(); i++) {
    if (value == ValueAt(i)) {
      return i;
    }
  }
  return -1;
}
/*****************************************************************************
 * LOOKUP
 *****************************************************************************/
/*
 * Find and return the child pointer(page_id) which points to the child page
 * that contains input "key"
 * Start the search from the second key(the first key should always be invalid)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::Lookup(const KeyType &key, const KeyComparator &comparator) const -> ValueType {
  assert(GetSize() > 1);
  int st = 1;
  int ed = GetSize() - 1;
  while (st <= ed) {  // find the last key in array <= input
    int mid = (ed - st) / 2 + st;
    if (comparator(array_[mid].first, key) <= 0) {
      st = mid + 1;
    } else {
      ed = mid - 1;
    }
  }
  return array_[st - 1].second;
}
/*****************************************************************************
 * SPLIT
 *****************************************************************************/
/*
 * Remove half of key & value pairs from this page to "recipient" page
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveHalfTo(BPlusTreeInternalPage *recipient,
                                                BufferPoolManager *buffer_pool_manager) {
  assert(recipient != nullptr);
  // assert(GetSize() == GetMaxSize() + 1);
  // copy last half [mid, size) 移动到新的页
  int total = GetSize();
  int copy_start_index = total >> 1;
  page_id_t recipient_page_id = recipient->GetPageId();
  for (int i = copy_start_index; i < total; i++) {
    recipient->array_[i - copy_start_index] = array_[i];
    // 儿子的父亲指针
    page_id_t child_page_id = array_[i].second;
    auto child_raw_page = buffer_pool_manager->FetchPage(child_page_id);
    auto child_page = reinterpret_cast<BPlusTreePage *>(child_raw_page->GetData());
    child_page->SetParentPageId(recipient_page_id);
    buffer_pool_manager->UnpinPage(child_page_id, true);
  }
  // set size, is odd, bigger is last part
  SetSize(copy_start_index);
  recipient->SetSize(total - copy_start_index);
}
/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Populate new root page with old_value + new_key & new_value
 * When the insertion cause overflow from leaf page all the way upto the root
 * page, you should create a new root page and populate its elements.
 * NOTE: This method is only called within InsertIntoParent()(b_plus_tree.cpp)
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::PopulateNewRoot(const ValueType &old_value, const KeyType &new_key,
                                                     const ValueType &new_value) {
  SetSize(2);
  SetKeyAt(1, new_key);
  SetValueAt(0, old_value);
  SetValueAt(1, new_value);
}
/*
 * Insert new_key & new_value pair right after the pair with its value ==
 * old_value
 * @return:  new size after insertion
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::InsertNodeAfter(const ValueType &old_value, const KeyType &new_key,
                                                     const ValueType &new_value) -> int {
  int idx = ValueIndex(old_value) + 1;
  assert(idx > 0);
  IncreaseSize(1);
  int cur_size = GetSize();
  for (int i = cur_size - 1; i > idx; i--) {
    array_[i] = array_[i - 1];
  }
  array_[idx].first = new_key;
  array_[idx].second = new_value;
  return cur_size;
}
/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Remove the key & value pair in internal page according to input index(a.k.a
 * array offset)
 * NOTE: store key&value pair continuously after deletion
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Remove(int index) {
  assert(index >= 0 && index < GetSize());
  for (int i = index + 1; i < GetSize(); i++) {
    array_[i - 1] = array_[i];
  }
  IncreaseSize(-1);
}
/*
 * Remove the only key & value pair in internal page and return the value
 * NOTE: only call this method within AdjustRoot()(in b_plus_tree.cpp)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::RemoveAndReturnOnlyChild() -> ValueType {
  ValueType ret = ValueAt(0);
  IncreaseSize(-1);
  assert(GetSize() == 0);
  return ret;
}
/*****************************************************************************
 * MERGE
 *****************************************************************************/
/*
 * Remove all of key & value pairs from this page to "recipient" page, then
 * update relavent key & value pair in its parent page.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveAllTo(BPlusTreeInternalPage *recipient, int index_in_parent,
                                               BufferPoolManager *buffer_pool_manager) {
  int start = recipient->GetSize();
  page_id_t recipient_page_id = recipient->GetPageId();
  // first find parent
  Page *page = buffer_pool_manager->FetchPage(GetParentPageId());
  assert(page != nullptr);
  auto parent = reinterpret_cast<BPlusTreeInternalPage *>(page->GetData());

  // the separation key from parent
  SetKeyAt(0, parent->KeyAt(index_in_parent));
  buffer_pool_manager->UnpinPage(parent->GetPageId(), false);
  for (int i = 0; i < GetSize(); ++i) {
    recipient->array_[start + i].first = array_[i].first;
    recipient->array_[start + i].second = array_[i].second;
    // update children's parent page
    auto child_raw_page = buffer_pool_manager->FetchPage(array_[i].second);
    auto child_node = reinterpret_cast<BPlusTreePage *>(child_raw_page->GetData());
    child_node->SetParentPageId(recipient_page_id);
    buffer_pool_manager->UnpinPage(array_[i].second, true);
  }
  // update relavent key & value pair in its parent page.
  recipient->SetSize(start + GetSize());
  assert(recipient->GetSize() <= recipient->GetMaxSize());
  SetSize(0);
}

/*****************************************************************************
 * REDISTRIBUTE
 *****************************************************************************/
/*
 * Remove the first key & value pair from this page to tail of "recipient"
 * page, then update relavent key & value pair in its parent page.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveFirstToEndOf(BPlusTreeInternalPage *recipient,
                                                      BufferPoolManager *buffer_pool_manager) {
  // 防御性编程
  assert(recipient->GetSize() + 1 <= recipient->GetMaxSize());
  assert(GetSize() >= 1);
  // reset and move
  recipient->array_[recipient->GetSize()] = MappingType{KeyAt(0), ValueAt(0)};
  auto child_page_id = ValueAt(0);
  recipient->IncreaseSize(1);
  IncreaseSize(-1);
  for (int i = 0; i < GetSize(); i++) {
    array_[i] = array_[i + 1];
  }
  // update child parent page id
  auto page = buffer_pool_manager->FetchPage(child_page_id);
  assert(page != nullptr);
  auto child = reinterpret_cast<BPlusTreePage *>(page->GetData());
  child->SetParentPageId(recipient->GetPageId());
  assert(child->GetParentPageId() == recipient->GetPageId());
  buffer_pool_manager->UnpinPage(child->GetPageId(), true);
  // update relavent key & value pair in its parent page.
  // page = buffer_pool_manager->FetchPage(GetParentPageId());
  // auto parent = reinterpret_cast<B_PLUS_TREE_INTERNAL_PAGE_TYPE *>(page->GetData());
  // parent->SetKeyAt(parent->ValueIndex(GetPageId()), array_[0].first);
  // buffer_pool_manager->UnpinPage(GetParentPageId(), true);
}

/*
 * Remove the last key & value pair from this page to head of "recipient"
 * page, then update relavent key & value pair in its parent page.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveLastToFrontOf(BPlusTreeInternalPage *recipient, int parent_index,
                                                       BufferPoolManager *buffer_pool_manager) {
  MappingType pair{KeyAt(GetSize() - 1), ValueAt(GetSize() - 1)};
  IncreaseSize(-1);
  recipient->CopyFirstFrom(pair, parent_index, buffer_pool_manager);
}
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::CopyFirstFrom(const MappingType &pair, int parent_index,
                                                   BufferPoolManager *buffer_pool_manager) {
  assert(GetSize() + 1 < GetMaxSize());
  // memmove(array_ + 1, array_, static_cast<size_t>(GetSize() * sizeof(MappingType)));
  for (int i = GetSize() - 1; i >= 0; --i) {
    array_[i + 1] = array_[i];
  }
  IncreaseSize(1);
  array_[0] = pair;
  // update child parent page id
  auto child_page_id = pair.second;
  auto page = buffer_pool_manager->FetchPage(child_page_id);
  assert(page != nullptr);
  auto child = reinterpret_cast<BPlusTreePage *>(page->GetData());
  child->SetParentPageId(GetPageId());
  assert(child->GetParentPageId() == GetPageId());
  buffer_pool_manager->UnpinPage(child->GetPageId(), true);
  // update relavent key & value pair in its parent page.
  // page = buffer_pool_manager->FetchPage(GetParentPageId());
  // auto parent = reinterpret_cast<B_PLUS_TREE_INTERNAL_PAGE_TYPE *>(page->GetData());
  // parent->SetKeyAt(parent_index, array_[0].first);
  // buffer_pool_manager->UnpinPage(GetParentPageId(), true);
}
// valuetype for internalNode should be page id_t
template class BPlusTreeInternalPage<GenericKey<4>, page_id_t, GenericComparator<4>>;
template class BPlusTreeInternalPage<GenericKey<8>, page_id_t, GenericComparator<8>>;
template class BPlusTreeInternalPage<GenericKey<16>, page_id_t, GenericComparator<16>>;
template class BPlusTreeInternalPage<GenericKey<32>, page_id_t, GenericComparator<32>>;
template class BPlusTreeInternalPage<GenericKey<64>, page_id_t, GenericComparator<64>>;
}  // namespace bustub
