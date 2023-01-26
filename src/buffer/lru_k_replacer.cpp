//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"

namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::lock_guard<std::mutex> guard(latch_);
  if (Size() == 0U) {
    return false;
  }
  frame_id_t evict_frame_id = -1;

  for (auto &frame : frame_list_) {
    if (!frame.second.IsEvictable()) {
      continue;
    }
    if (evict_frame_id == -1) {
      evict_frame_id = frame.first;
      continue;
    }
    if (frame_list_[evict_frame_id].Count() < k_ && frame.second.Count() < k_) {
      if (frame_list_[evict_frame_id].Time() > frame.second.Time()) {
        evict_frame_id = frame.first;
      }
    } else {
      if (frame_list_[evict_frame_id].Count() >= k_ && frame.second.Count() >= k_) {
        if (frame_list_[evict_frame_id].Time() > frame.second.Time()) {
          evict_frame_id = frame.first;
        }
      } else {
        if (frame_list_[evict_frame_id].Count() >= k_ && frame.second.Count() < k_) {
          evict_frame_id = frame.first;
        }
      }
    }
  }
  *frame_id = evict_frame_id;
  frame_list_.erase(evict_frame_id);
  curr_size_--;
  return true;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id) {
  std::lock_guard<std::mutex> guard(latch_);
  if (frame_id > static_cast<int>(replacer_size_)) {
    throw std::exception();
  }
  ++current_timestamp_;
  if (frame_list_.count(frame_id) != 0U) {
    frame_list_[frame_id].Access();
    if (frame_list_[frame_id].Count() >= k_) {
      frame_list_[frame_id].SetTime(current_timestamp_);
    }
  } else {
    frame_list_.emplace(frame_id, FrameMeta(current_timestamp_));
  }
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  std::lock_guard<std::mutex> guard(latch_);
  if (frame_id > static_cast<int>(replacer_size_)) {
    throw std::exception();
  }
  if (frame_list_.count(frame_id) != 0U) {
    auto &frame = frame_list_[frame_id];
    if (frame.IsEvictable() && !set_evictable) {
      curr_size_--;
    }
    if (!frame.IsEvictable() && set_evictable) {
      curr_size_++;
    }
    frame.SetEvictable(set_evictable);
  }
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  std::lock_guard<std::mutex> guard(latch_);
  if (frame_id > static_cast<int>(replacer_size_)) {
    throw std::exception();
  }
  if (frame_list_.count(frame_id) != 0U) {
    if (frame_list_[frame_id].IsEvictable()) {
      curr_size_--;
    }
    frame_list_.erase(frame_id);
  }
}

auto LRUKReplacer::Size() -> size_t { return curr_size_; }

}  // namespace bustub
