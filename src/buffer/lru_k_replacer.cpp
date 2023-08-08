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
#include "iostream"
namespace bustub {

LRUKReplacer::FrameInfo::FrameInfo(frame_id_t frame_id) : frame_id_(frame_id) {
  times_ = 0;
  evictable_ = true;
}

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  latch_.lock();
  for (auto it = history_list_.begin(); it != history_list_.end();) {
    if ((*it)->IsEvictable()) {
      *frame_id = (*it)->GetId();
      it = history_list_.erase(it);
      history_map_.erase(*frame_id);
      latch_.unlock();
      return true;
    }
    it++;
  }
  for (auto it = cache_list_.begin(); it != cache_list_.end();) {
    if ((*it)->IsEvictable()) {
      *frame_id = (*it)->GetId();
      it = cache_list_.erase(it);
      cache_map_.erase(*frame_id);
      latch_.unlock();
      return true;
    }
    it++;
  }
  latch_.unlock();
  return false;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id) {
  BUSTUB_ASSERT(frame_id <= (int)replacer_size_, "Invalid frame id");
  latch_.lock();
  if (cache_map_.count(frame_id) != 0U) {
    auto it = cache_map_[frame_id];
    cache_list_.emplace_back(std::move(*it));
    cache_list_.erase(it);
    cache_map_[frame_id] = (--cache_list_.end());
    latch_.unlock();
    return;
  }

  if (history_map_.count(frame_id) != 0U) {
    auto it = history_map_[frame_id];
    (*it)->IncreaseTimes();
    if ((*it)->GetTimes() >= k_) {
      cache_list_.emplace_back(std::move((*it)));
      cache_map_[frame_id] = --cache_list_.end();
      history_map_.erase(frame_id);
      history_list_.erase(it);
    }
    latch_.unlock();
    return;
  }

  std::unique_ptr<FrameInfo> frame_ptr = std::make_unique<FrameInfo>(frame_id);
  frame_ptr->IncreaseTimes();
  history_list_.emplace_back(std::move(frame_ptr));
  history_map_[frame_id] = (--history_list_.end());
  latch_.unlock();
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  latch_.lock();
  if (history_map_.count(frame_id) != 0U) {
    (*history_map_[frame_id])->SetEvictable(set_evictable);
    latch_.unlock();
    return;
  }
  if (cache_map_.count(frame_id) != 0U) {
    (*cache_map_[frame_id])->SetEvictable(set_evictable);
    latch_.unlock();
    return;
  }
  latch_.unlock();
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  latch_.lock();
  if (history_map_.count(frame_id) != 0U) {
    auto it = history_map_[frame_id];
    BUSTUB_ASSERT((*it)->IsEvictable(), "Remove unEvictable frame id.");
    history_list_.erase(it);
    history_map_.erase(frame_id);
    latch_.unlock();
    return;
  }
  if (cache_map_.count(frame_id) != 0U) {
    auto it = cache_map_[frame_id];
    BUSTUB_ASSERT((*it)->IsEvictable(), "Remove unEvictable frame id.");
    cache_list_.erase(it);
    cache_map_.erase(frame_id);
    latch_.unlock();
    return;
  }
  latch_.unlock();
}

auto LRUKReplacer::Size() -> size_t {
  latch_.lock();
  size_t num = 0;
  for (auto &ele : cache_list_) {
    if (ele->IsEvictable()) {
      num++;
    }
  }
  for (auto &ele : history_list_) {
    if (ele->IsEvictable()) {
      num++;
    }
  }
  latch_.unlock();
  return num;
}

}  // namespace bustub
