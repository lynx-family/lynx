// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { assertType } from 'vitest';
import { IntrinsicElements, UIMethods, VideoBufferingEvent, VideoErrorEvent, VideoFirstFrameEvent, VideoProps, VideoTimeUpdateEvent, VideoUIMethods } from '../../types';

let a: unknown;

{
  <video src="https://example.com/video.mp4" loop={true} volume={0.5} muted={false} speed={1.25} object-fit="cover" mode="latest" timeupdate-interval={0.2} />;

  assertType<VideoProps>(a as IntrinsicElements['video']);
  assertType<string | undefined>(a as IntrinsicElements['video']['src']);
  assertType<boolean | undefined>(a as IntrinsicElements['video']['loop']);
  assertType<number | undefined>(a as IntrinsicElements['video']['volume']);
  assertType<boolean | undefined>(a as IntrinsicElements['video']['muted']);
  assertType<number | undefined>(a as IntrinsicElements['video']['speed']);
  assertType<'contain' | 'cover' | 'fill' | undefined>(a as IntrinsicElements['video']['object-fit']);
  assertType<'queue' | 'direct' | 'latest' | undefined>(a as IntrinsicElements['video']['mode']);
  assertType<number | undefined>(a as IntrinsicElements['video']['timeupdate-interval']);
}

{
  <video
    bindfirstframe={(e: VideoFirstFrameEvent) => {
      assertType<number>(e.detail.duration);
    }}
  />;

  <video
    bindtimeupdate={(e: VideoTimeUpdateEvent) => {
      assertType<number>(e.detail.current);
      assertType<number>(e.detail.duration);
    }}
  />;

  <video
    binderror={(e: VideoErrorEvent) => {
      assertType<number>(e.detail.errorCode);
      assertType<string>(e.detail.errorMsg);
    }}
  />;

  <video
    bindbuffering={(e: VideoBufferingEvent) => {
      assertType<number>(e.detail.buffering);
    }}
  />;

  <video bindplaying={() => {}} bindpaused={() => {}} bindstopped={() => {}} bindended={() => {}} bindlooped={() => {}} />;
}

function invoke<T extends keyof { video: VideoUIMethods }>(_param: { video: VideoUIMethods }[T]) {}

{
  invoke<'video'>({
    method: 'play',
    success: (res) => {
      assertType<boolean | undefined>(res.success);
    },
  });

  invoke<'video'>({
    method: 'pause',
    fail: (res) => {
      assertType<number | undefined>(res.errorCode);
      assertType<string | undefined>(res.msg);
      assertType<string | undefined>(res.errorMsg);
    },
  });

  invoke<'video'>({
    method: 'stop',
  });

  invoke<'video'>({
    method: 'seek',
    params: {
      position: 2,
    },
  });

  assertType<VideoUIMethods>(a as UIMethods['video']);
  assertType<'play' | 'pause' | 'stop' | 'seek'>(a as UIMethods['video']['method']);
}
