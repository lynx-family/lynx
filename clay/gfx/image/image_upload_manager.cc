// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/image_upload_manager.h"

#include <utility>

namespace clay {

ImageUploadManager& ImageUploadManager::GetInstance() {
  static ImageUploadManager instance;
  return instance;
}

void ImageUploadManager::AddImageUploadTask(uint64_t queue_id,
                                            OneShotCallback<>&& task,
                                            int image_id) {
  std::unique_lock<std::mutex> lock(image_upload_tasks_mutex_);

  TaskList& task_list = image_upload_tasks_[queue_id];
  task_list.emplace_back(image_id, std::move(task));

  TaskIter task_iter = --task_list.end();
  image_to_task_entries_[image_id].emplace_back(queue_id, task_iter);
}

void ImageUploadManager::RemoveImageUploadTaskByQueueId(uint64_t queue_id) {
  std::unique_lock<std::mutex> lock(image_upload_tasks_mutex_);

  auto queue_entry = image_upload_tasks_.find(queue_id);
  if (queue_entry == image_upload_tasks_.end()) {
    return;
  }

  TaskList& task_list = queue_entry->second;
  // Remove the indexes in image_to_task_entries_
  for (auto task_iter = task_list.begin(); task_iter != task_list.end();
       ++task_iter) {
    int image_id = task_iter->first;
    RemoveSingleTaskFromIndex(image_id, queue_id, task_iter);
  }

  image_upload_tasks_.erase(queue_entry);
}

void ImageUploadManager::RemoveImageUploadTaskByImageId(int image_id) {
  std::unique_lock<std::mutex> lock(image_upload_tasks_mutex_);

  auto image_entry = image_to_task_entries_.find(image_id);
  if (image_entry == image_to_task_entries_.end()) {
    return;
  }

  ImageTaskEntries& all_task_entries = image_entry->second;
  for (const auto& [queue_id, task_iter] : all_task_entries) {
    auto queue_entry = image_upload_tasks_.find(queue_id);
    if (queue_entry == image_upload_tasks_.end()) {
      continue;
    }

    TaskList& task_list = queue_entry->second;
    task_list.erase(task_iter);
  }

  image_to_task_entries_.erase(image_id);
}

// @param queue_id: page id that tasks belong to.
// @return: number of tasks that are left.
uint64_t ImageUploadManager::ProcessSingleTask(uint64_t queue_id) {
  std::unique_lock<std::mutex> lock(image_upload_tasks_mutex_);

  auto queue_entry = image_upload_tasks_.find(queue_id);
  if (queue_entry == image_upload_tasks_.end()) {
    return 0;
  }
  // Upload no more than one task at each call.
  auto& tasks = queue_entry->second;
  if (tasks.empty()) {
    return 0;
  }
  uint64_t tasks_left = 0;
  do {
    TaskIter task_iter = tasks.begin();
    int image_id = task_iter->first;

    auto task = std::move(task_iter->second);

    // Clean the indexes in image_to_task_entries before pop the task.
    RemoveSingleTaskFromIndex(image_id, queue_id, task_iter);
    tasks.pop_front();
    tasks_left = tasks.size();

    if (task.UnDone()) {
      lock.unlock();
      task();
      return tasks_left;
    }
  } while (!tasks.empty());
  return 0;
}

void ImageUploadManager::RemoveSingleTaskFromIndex(int image_id,
                                                   uint64_t queue_id,
                                                   TaskIter task_iter) {
  auto image_entry = image_to_task_entries_.find(image_id);
  if (image_entry == image_to_task_entries_.end()) {
    return;
  }

  ImageTaskEntries& entries = image_entry->second;
  for (auto iter = entries.begin(); iter != entries.end(); ++iter) {
    if (iter->first == queue_id && iter->second == task_iter) {
      entries.erase(iter);
      break;
    }
  }
  if (entries.empty()) {
    image_to_task_entries_.erase(image_id);
  }
}

}  // namespace clay
