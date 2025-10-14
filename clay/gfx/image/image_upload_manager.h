// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_UPLOAD_MANAGER_H_
#define CLAY_GFX_IMAGE_IMAGE_UPLOAD_MANAGER_H_

#include <list>
#include <mutex>
#include <unordered_map>
#include <utility>

#include "clay/shell/common/one_shot_callback.h"

namespace clay {
class Image;

class ImageUploadManager {
 public:
  static ImageUploadManager& GetInstance();

  void AddImageUploadTask(uint64_t queue_id, OneShotCallback<>&& task,
                          int image_id);
  void RemoveImageUploadTaskByQueueId(uint64_t queue_id);
  void RemoveImageUploadTaskByImageId(int image_id);

  uint64_t ProcessSingleTask(uint64_t queue_id);

 private:
  using Task = std::pair<int, OneShotCallback<>>;
  using TaskList = std::list<Task>;
  using TaskIter = TaskList::iterator;
  using ImageTaskEntries = std::list<std::pair<uint64_t, TaskIter>>;

  void RemoveSingleTaskFromIndex(int image_id, uint64_t queue_id,
                                 TaskIter task_iter);

  ImageUploadManager() = default;
  ~ImageUploadManager() = default;

  // key: queue id, which is page view's unique id.
  // value: image upload tasks that belong to the same page view.
  std::unordered_map<uint64_t, TaskList> image_upload_tasks_;
  // key: image id.
  // value: a list of queue id and task iterator.
  std::unordered_map<int, ImageTaskEntries> image_to_task_entries_;

  std::mutex image_upload_tasks_mutex_;
};
}  // namespace clay

#endif  // CLAY_GFX_IMAGE_IMAGE_UPLOAD_MANAGER_H_
