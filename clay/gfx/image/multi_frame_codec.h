// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_MULTI_FRAME_CODEC_H_
#define CLAY_GFX_IMAGE_MULTI_FRAME_CODEC_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "base/include/fml/task_runner.h"
#include "clay/gfx/image/codec.h"
#include "clay/gfx/image/frame_info.h"
#include "third_party/skia/src/codec/SkCodecImageGenerator.h"  // nogncheck

namespace clay {

class MultiFrameCodec : public Codec {
 public:
  explicit MultiFrameCodec(std::shared_ptr<SkCodecImageGenerator> generator);

  ~MultiFrameCodec() override;

  // |Codec|
  int FrameCount() const override;

  // |Codec|
  int FrameDuration(int index) const override;

  // |Codec|
  void NextFrame(const CodecCallback& callback) override;

 private:
  // Captures the state shared between the IO and UI task runners.
  //
  // Decoding occurs on the IO task runner. Since it is possible for the UI
  // object to be collected independently of the IO task runner work, it is not
  // safe for this state to live directly on the MultiFrameCodec.
  // Instead, the MultiFrameCodec creates this object when it is constructed,
  // shares it with the IO task runner's decoding work, and sets the live_
  // member to false when it is destructed.
  struct State {
    explicit State(std::shared_ptr<SkCodecImageGenerator> generator);

    const std::shared_ptr<SkCodecImageGenerator> generator_;
    const int frame_count_;

    // The non-const members and functions below here are only read or written
    // to on the IO thread. They are not safe to access or write on the UI
    // thread.
    int next_frame_index_;

    // The last decoded frame that's required to decode any subsequent frames.
    std::unique_ptr<SkBitmap> last_required_frame_;

    // The index of the last decoded required frame.
    int last_required_frame_index_ = -1;

    fml::RefPtr<GraphicsImage> GetNextFrameImage();

    void GetNextFrameAndInvokeCallback(const CodecCallback& callback);
  };

  // Shared across the UI and IO task runners.
  std::shared_ptr<State> state_;

  FML_FRIEND_MAKE_REF_COUNTED(MultiFrameCodec);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(MultiFrameCodec);
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_MULTI_FRAME_CODEC_H_
