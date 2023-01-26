//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager_instance.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager_instance.h"

#include "common/exception.h"
#include "common/macros.h"

namespace bustub {

BufferPoolManagerInstance::BufferPoolManagerInstance(size_t pool_size, DiskManager *disk_manager, size_t replacer_k,
                                                     LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager), log_manager_(log_manager) {
  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  page_table_ = new ExtendibleHashTable<page_id_t, frame_id_t>(bucket_size_);
  replacer_ = new LRUKReplacer(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }

  // TODO(students): remove this line after you have implemented the buffer pool manager
  // throw NotImplementedException(
  //     "BufferPoolManager is not implemented yet. If you have finished implementing BPM, please remove the throw "
  //     "exception line in `buffer_pool_manager_instance.cpp`.");
}

BufferPoolManagerInstance::~BufferPoolManagerInstance() {
  delete[] pages_;
  delete page_table_;
  delete replacer_;
}

auto BufferPoolManagerInstance::NewPgImp(page_id_t *page_id) -> Page * {
  frame_id_t frame_id;
  if (!free_list_.empty()) {
    frame_id = free_list_.back();
    free_list_.pop_back();
  } else {
    // LRU-K 淘汰
    bool success = replacer_->Evict(&frame_id);
    if (!success) {
      return nullptr;
    }
    // 淘汰成功 分配一个 frame_id
    // 该页是脏的  需要重新写回 disk
    if (pages_[frame_id].IsDirty()) {
      pages_[frame_id].is_dirty_ = false;
      disk_manager_->WritePage(pages_[frame_id].GetPageId(), pages_[frame_id].GetData());
    }
    // page_table删除
    page_table_->Remove(pages_[frame_id].GetPageId());
  }
  // 目前已经分配一个 frame_id -> page_id
  *page_id = AllocatePage();
  page_table_->Insert(*page_id, frame_id);
  // 初始化 pages_[frame_id]
  pages_[frame_id].page_id_ = *page_id;
  pages_[frame_id].is_dirty_ = false;
  pages_[frame_id].pin_count_ = 1;
  pages_[frame_id].ResetMemory();
  disk_manager_->WritePage(pages_[frame_id].GetPageId(), pages_[frame_id].GetData());
  replacer_->RecordAccess(frame_id);
  replacer_->SetEvictable(frame_id, false);
  return &pages_[frame_id];
}

auto BufferPoolManagerInstance::FetchPgImp(page_id_t page_id) -> Page * { 
  frame_id_t frame_id;
  bool is_exist = page_table_->Find(page_id, frame_id);
  if (is_exist) {
    pages_[frame_id].pin_count_++;
    replacer_->RecordAccess(frame_id);
    return &pages_[frame_id];
  }
  // buffer pool 中无 page_id 需要将 page 加载入缓存 先得到一个 frame_id
  if (!free_list_.empty()) {
    frame_id = free_list_.back();
    free_list_.pop_back();
  } else {
  // LRU-K 淘汰
    bool success = replacer_->Evict(&frame_id);
    if (!success) {
      return nullptr;
    }
  // 淘汰成功 分配一个 frame_id
  // 该页是脏的  需要重新写回 disk
    if (pages_[frame_id].IsDirty()) {
      pages_[frame_id].is_dirty_ = false;
      disk_manager_->WritePage(pages_[frame_id].GetPageId(), pages_[frame_id].GetData());
    }
    // page_table删除
    page_table_->Remove(pages_[frame_id].GetPageId());
  }
  // 磁盘读入buffer pool
  disk_manager_->ReadPage(page_id, pages_[frame_id].GetData());
  page_table_->Insert(page_id, frame_id);
  pages_[frame_id].page_id_ = page_id;
  pages_[frame_id].is_dirty_ = false;
  pages_[frame_id].pin_count_ = 1;
  replacer_->RecordAccess(frame_id);
  replacer_->SetEvictable(frame_id, false);
  return &pages_[frame_id];  
}

auto BufferPoolManagerInstance::UnpinPgImp(page_id_t page_id, bool is_dirty) -> bool { 
  frame_id_t frame_id;
  bool is_exist = page_table_->Find(page_id, frame_id);
  if (!is_exist) {
    return false;
  }
  if (pages_[frame_id].GetPinCount() <= 0) {
    return false;
  }
  pages_[frame_id].pin_count_--;
  if (pages_[frame_id].GetPinCount() <= 0) {
    replacer_->SetEvictable(frame_id, true);
  }
  pages_[frame_id].is_dirty_ |= is_dirty;
  return true;
}

auto BufferPoolManagerInstance::FlushPgImp(page_id_t page_id) -> bool {
  frame_id_t frame_id;
  bool is_exist = page_table_->Find(page_id, frame_id);
  if (!is_exist) {
    return false;
  }
  pages_[frame_id].is_dirty_ = false;
  disk_manager_->WritePage(page_id, pages_[frame_id].GetData());
  return true;
}
void BufferPoolManagerInstance::FlushAllPgsImp() {

  for (frame_id_t frame_id = 0; frame_id < static_cast<frame_id_t>(pool_size_); ++frame_id) {
    page_id_t page_id = pages_[frame_id].page_id_;
    if (page_id == INVALID_PAGE_ID) continue;
    pages_[frame_id].is_dirty_ = false;
    disk_manager_->WritePage(page_id, pages_[frame_id].GetData());
  }
}
/**
   * TODO(P1): Add implementation
   *
   * @brief Delete a page from the buffer pool. If page_id is not in the buffer pool, do nothing and return true. If the
   * page is pinned and cannot be deleted, return false immediately.
   *
   * After deleting the page from the page table, stop tracking the frame in the replacer and add the frame
   * back to the free list. Also, reset the page's memory and metadata. Finally, you should call DeallocatePage() to
   * imitate freeing the page on the disk.
   *
   * @param page_id id of page to be deleted
   * @return false if the page exists but could not be deleted, true if the page didn't exist or deletion succeeded
   */
auto BufferPoolManagerInstance::DeletePgImp(page_id_t page_id) -> bool { 
  frame_id_t frame_id;
  bool is_exist = page_table_->Find(page_id, frame_id);
  if (!is_exist) {
    return true;
  }
  assert(page_id == pages_[frame_id].GetPageId());
  if (pages_[frame_id].GetPinCount() > 0) {
    return false;
  }
  if (pages_[frame_id].IsDirty()) {
    pages_[frame_id].is_dirty_ = false;
    disk_manager_->WritePage(pages_[frame_id].GetPageId(), pages_[frame_id].GetData());
  }
  // page_table删除
  page_table_->Remove(pages_[frame_id].GetPageId());
  pages_[frame_id].page_id_ = INVALID_PAGE_ID;
  // pages_[frame_id].ResetMemory();
  free_list_.push_back(frame_id);
  return true;
}
auto BufferPoolManagerInstance::AllocatePage() -> page_id_t { return next_page_id_++; }

}  // namespace bustub
