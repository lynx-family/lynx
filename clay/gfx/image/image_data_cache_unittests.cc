// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/image_data_cache.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(ImageDataCacheTest, SetMaxCachedBytesTest) {
  size_t cache_size = 1024 * 1024;
  ImageDataCache::GetInstance().SetMaxCachedBytes(cache_size);
  EXPECT_EQ(ImageDataCache::GetInstance().max_cached_bytes_, cache_size);
}

TEST(ImageDataCacheTest, CacheImageDataTest) {
  // Clear old cache.
  ImageDataCache::GetInstance().ClearCache();

  std::string url = "url";
  std::string raw_data = "data";
  sk_sp<SkData> sk_data = SkData::MakeWithCString(raw_data.c_str());
  ImageDataCache::GetInstance().CacheImageData(url, sk_data);
  EXPECT_TRUE(ImageDataCache::GetInstance().active_cache_[url].second == 1);
  EXPECT_TRUE(sk_data->equals(
      ImageDataCache::GetInstance().active_cache_[url].first.get()));

  ImageDataCache::GetInstance().CacheImageData(url, sk_data);
  EXPECT_TRUE(ImageDataCache::GetInstance().active_cache_[url].second == 2);

  // get data from active cache.
  sk_sp<SkData> get_cached_data =
      ImageDataCache::GetInstance().GetImageData(url);
  EXPECT_TRUE(ImageDataCache::GetInstance().active_cache_[url].second == 2);

  ImageDataCache::GetInstance().ReleaseOrArchiveImageData(url);
  EXPECT_TRUE(ImageDataCache::GetInstance().active_cache_[url].second == 1);

  ImageDataCache::GetInstance().ReleaseOrArchiveImageData(url);
  EXPECT_TRUE(ImageDataCache::GetInstance().active_cache_.find(url) ==
              ImageDataCache::GetInstance().active_cache_.end());
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_map_.find(url) !=
              ImageDataCache::GetInstance().inactive_cache_map_.end());
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_map_.size() == 1);
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_list_.size() == 1);

  size_t data_size = sk_data->size();
  EXPECT_EQ(ImageDataCache::GetInstance().inactive_cache_bytes_, data_size);

  // get data from inactive cache.
  get_cached_data = ImageDataCache::GetInstance().GetImageData(url);
  size_t expected_inactive_cache_bytes_ = 0;
  EXPECT_EQ(ImageDataCache::GetInstance().inactive_cache_bytes_,
            expected_inactive_cache_bytes_);
  EXPECT_TRUE(ImageDataCache::GetInstance().active_cache_.find(url) ==
              ImageDataCache::GetInstance().active_cache_.end());
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_map_.find(url) ==
              ImageDataCache::GetInstance().inactive_cache_map_.end());
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_map_.size() == 0);
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_list_.size() == 0);
}

TEST(ImageDataCacheTest, RemoveImageDataIfExistTest) {
  // Clear old cache.
  ImageDataCache::GetInstance().ClearCache();
  std::string url = "url";
  std::string raw_data = "data";
  sk_sp<SkData> sk_data = SkData::MakeWithCString(raw_data.c_str());
  ImageDataCache::GetInstance().CacheImageData(url, sk_data);
  EXPECT_TRUE(ImageDataCache::GetInstance().active_cache_[url].second == 1);

  ImageDataCache::GetInstance().RemoveImageDataIfExist(url);
  EXPECT_TRUE(ImageDataCache::GetInstance().active_cache_.size() == 0);
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_map_.size() == 0);
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_list_.size() == 0);
}

TEST(ImageDataCacheTest, CleanToTest) {
  // Clear old cache.
  ImageDataCache::GetInstance().ClearCache();

  // max_cached_bytes_: 10, desired_cache_bytes_: 8
  ImageDataCache::GetInstance().SetMaxCachedBytes(10);

  std::string url1 = "url1";
  std::string raw_data1(2, 'a');
  // sk_data1 size: 3
  sk_sp<SkData> sk_data1 = SkData::MakeWithCString(raw_data1.c_str());
  ImageDataCache::GetInstance().CacheImageData(url1, sk_data1);
  ImageDataCache::GetInstance().ReleaseOrArchiveImageData(url1);
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_bytes_ == 3);

  std::string url2 = "url2";
  std::string raw_data2(2, 'a');
  // sk_data2 size: 3
  sk_sp<SkData> sk_data2 = SkData::MakeWithCString(raw_data2.c_str());
  ImageDataCache::GetInstance().CacheImageData(url2, sk_data2);
  ImageDataCache::GetInstance().ReleaseOrArchiveImageData(url2);
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_bytes_ == 6);

  std::string url3 = "url3";
  std::string raw_data3(1, 'a');
  // sk_data2 size: 2
  sk_sp<SkData> sk_data3 = SkData::MakeWithCString(raw_data3.c_str());
  ImageDataCache::GetInstance().CacheImageData(url3, sk_data3);
  ImageDataCache::GetInstance().ReleaseOrArchiveImageData(url3);
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_bytes_ == 8);

  std::string url4 = "url4";
  std::string raw_data4(2, 'a');
  // sk_data2 size: 3
  sk_sp<SkData> sk_data4 = SkData::MakeWithCString(raw_data4.c_str());
  ImageDataCache::GetInstance().CacheImageData(url4, sk_data4);
  // current inactive list: [url4, url3, url2, url], size: [3, 2, 3, 3], total
  // size: 11 will trigger CleanTo.
  ImageDataCache::GetInstance().ReleaseOrArchiveImageData(url4);
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_bytes_ == 8);
}

TEST(ImageDataCacheTest, ClearCacheTest) {
  // Clear old cache.
  ImageDataCache::GetInstance().ClearCache();

  std::string url1 = "url1";
  std::string raw_data1 = "data";
  sk_sp<SkData> sk_data1 = SkData::MakeWithCString(raw_data1.c_str());
  ImageDataCache::GetInstance().CacheImageData(url1, sk_data1);

  std::string url2 = "url2";
  std::string raw_data2 = "data";
  sk_sp<SkData> sk_data2 = SkData::MakeWithCString(raw_data2.c_str());
  ImageDataCache::GetInstance().CacheImageData(url2, sk_data2);

  ImageDataCache::GetInstance().ReleaseOrArchiveImageData(url1);
  EXPECT_TRUE(ImageDataCache::GetInstance().active_cache_.size() == 1);
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_map_.size() == 1);
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_list_.size() == 1);

  ImageDataCache::GetInstance().ClearCache();
  EXPECT_TRUE(ImageDataCache::GetInstance().active_cache_.size() == 0);
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_map_.size() == 0);
  EXPECT_TRUE(ImageDataCache::GetInstance().inactive_cache_list_.size() == 0);
}

}  // namespace clay
