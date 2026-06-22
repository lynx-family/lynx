import { useState, useCallback, useRef } from '@lynx-js/react';
import { root } from '@lynx-js/react';
import './index.css';

const PRIMARY_VIDEO_URL = 'https://www.w3schools.com/html/mov_bbb.mp4';
const SECONDARY_VIDEO_URL =
  'https://interactive-examples.mdn.mozilla.net/media/cc0-videos/flower.mp4';
const INVALID_VIDEO_URL = 'https://www.w3schools.com/html/not-found-video.mp4';

type EventName =
  | 'firstframe'
  | 'playing'
  | 'paused'
  | 'stopped'
  | 'timeupdate'
  | 'ended'
  | 'looped'
  | 'error'
  | 'buffering';

type EventCounts = Record<EventName, number>;

const createEventCounts = (): EventCounts => ({
  firstframe: 0,
  playing: 0,
  paused: 0,
  stopped: 0,
  timeupdate: 0,
  ended: 0,
  looped: 0,
  error: 0,
  buffering: 0,
});

export function VideoDemo() {
  const [src, setSrc] = useState(PRIMARY_VIDEO_URL);
  const [srcLabel, setSrcLabel] = useState('primary');
  const [status, setStatus] = useState('ready');
  const [currentTime, setCurrentTime] = useState(0);
  const [duration, setDuration] = useState(0);
  const [eventLog, setEventLog] = useState<string[]>([]);
  const [callbackLog, setCallbackLog] = useState<string[]>([]);
  const [signalLog, setSignalLog] = useState<string[]>([]);
  const [eventCounts, setEventCounts] = useState<EventCounts>(
    createEventCounts()
  );
  const [loop, setLoop] = useState(false);
  const [volume, setVolume] = useState(1);
  const [muted, setMuted] = useState(false);
  const [speed, setSpeed] = useState(1);
  const [objectFit, setObjectFit] = useState('contain');
  const [mode, setMode] = useState('queue');
  const [timeUpdateInterval, setTimeUpdateInterval] = useState(0.5);
  const [loopedSeen, setLoopedSeen] = useState(false);
  const [firstFrameDuration, setFirstFrameDuration] = useState(0);
  const [lastError, setLastError] = useState('none');
  const [lastBuffering, setLastBuffering] = useState(0);
  const [minTimeUpdateDelta, setMinTimeUpdateDelta] = useState(0);
  const lastTimeUpdateMs = useRef(0);

  const appendLog = useCallback(
    (
      setter: (updater: (prev: string[]) => string[]) => void,
      entry: string
    ) => {
      setter((prev) => [...prev.slice(-19), entry]);
    },
    []
  );

  const pushEvent = useCallback(
    (name: EventName, detail?: string) => {
      setEventCounts((prev) => ({
        ...prev,
        [name]: prev[name] + 1,
      }));
      appendLog(setEventLog, `${name}${detail ? ':' + detail : ''}`);
      if (name !== 'timeupdate' && name !== 'buffering') {
        appendLog(setSignalLog, `event:${name}`);
      }
    },
    [appendLog]
  );

  const pushCallback = useCallback(
    (name: string, detail?: string) => {
      appendLog(setCallbackLog, `${name}${detail ? ':' + detail : ''}`);
      appendLog(setSignalLog, `callback:${name}${detail ? ':' + detail : ''}`);
    },
    [appendLog]
  );

  const clearSignals = useCallback(() => {
    setEventCounts(createEventCounts());
    setEventLog([]);
    setCallbackLog([]);
    setSignalLog([]);
    setLoopedSeen(false);
    setLastError('none');
    setLastBuffering(0);
    setMinTimeUpdateDelta(0);
    lastTimeUpdateMs.current = 0;
  }, []);

  const handleFirstFrame = (e: any) => {
    const eventDuration = e.detail.duration || 0;
    setDuration(eventDuration);
    setFirstFrameDuration(eventDuration);
    pushEvent('firstframe', eventDuration.toFixed(1));
    setStatus('ready');
  };

  const handlePlaying = () => {
    pushEvent('playing');
    setStatus('playing');
  };

  const handlePaused = () => {
    pushEvent('paused');
    setStatus('paused');
  };

  const handleStopped = () => {
    pushEvent('stopped');
    setStatus('stopped');
    setCurrentTime(0);
  };

  const handleTimeUpdate = (e: any) => {
    const current = e.detail.current || 0;
    const eventDuration = e.detail.duration || 0;
    const now = Date.now();
    if (lastTimeUpdateMs.current > 0) {
      const delta = now - lastTimeUpdateMs.current;
      setMinTimeUpdateDelta((prev) =>
        prev === 0 ? delta : Math.min(prev, delta)
      );
    }
    lastTimeUpdateMs.current = now;
    setCurrentTime(current);
    setDuration(eventDuration);
    pushEvent('timeupdate', current.toFixed(1));
  };

  const handleEnded = () => {
    pushEvent('ended');
    setStatus('ended');
  };

  const handleLooped = () => {
    setLoopedSeen(true);
    pushEvent('looped');
  };

  const handleError = (e: any) => {
    const code = e.detail.errorCode;
    const msg = e.detail.errorMsg || 'unknown';
    setLastError(`${code}:${msg}`);
    pushEvent('error', `${code}`);
    setStatus('error');
  };

  const handleBuffering = (e: any) => {
    const buffering = e.detail.buffering || 0;
    setLastBuffering(buffering);
    pushEvent('buffering', buffering.toFixed(1));
  };

  const formatCallbackResult = (result: any) => {
    if (result == null) {
      return 'empty';
    }
    const payload =
      typeof result === 'object' && result !== null && result.data
        ? result.data
        : result;
    if (payload == null) {
      return 'empty';
    }
    if (typeof payload !== 'object') {
      return `${payload}`;
    }
    const parts = [];
    if (typeof payload.success !== 'undefined') {
      parts.push(`success=${payload.success}`);
    }
    if (typeof payload.errorCode !== 'undefined') {
      parts.push(`errorCode=${payload.errorCode}`);
    }
    if (typeof payload.msg !== 'undefined') {
      parts.push(`msg=${payload.msg}`);
    }
    if (parts.length > 0) {
      return parts.join(',');
    }
    try {
      return JSON.stringify(payload);
    } catch {
      return `${payload}`;
    }
  };

  const invokeMethod = (method: string, params?: object | null) => {
    lynx
      .createSelectorQuery()
      .select('#test-video')
      .invoke({
        method,
        params: params === undefined ? {} : params,
        success: (res: any) =>
          pushCallback(`${method}_ok`, formatCallbackResult(res)),
        fail: (err: any) =>
          pushCallback(`${method}_fail`, formatCallbackResult(err)),
      })
      .exec();
  };

  const invokeBurst = (methods: Array<[string, object?]>) => {
    clearSignals();
    setTimeout(() => {
      methods.forEach(([method, params]) => invokeMethod(method, params));
    }, 0);
  };

  const selectSource = (label: string, url: string) => {
    clearSignals();
    setSrcLabel(label);
    setSrc(url);
    setStatus(label === 'empty' ? 'empty' : 'loading');
    setCurrentTime(0);
    setDuration(0);
    setFirstFrameDuration(0);
  };

  const playThenSelectSource = (label: string, url: string) => {
    clearSignals();
    setTimeout(() => {
      invokeMethod('play');
      setSrcLabel(label);
      setSrc(url);
      setStatus(label === 'empty' ? 'empty' : 'loading');
      setCurrentTime(0);
      setDuration(0);
      setFirstFrameDuration(0);
    }, 0);
  };

  const playPauseThenSelectSource = (label: string, url: string) => {
    clearSignals();
    setTimeout(() => {
      invokeMethod('play');
      invokeMethod('pause');
      setSrcLabel(label);
      setSrc(url);
      setStatus(label === 'empty' ? 'empty' : 'loading');
      setCurrentTime(0);
      setDuration(0);
      setFirstFrameDuration(0);
    }, 0);
  };

  const seekThenSelectSource = (label: string, url: string) => {
    clearSignals();
    setTimeout(() => {
      invokeMethod('seek', { position: 2 });
      setSrcLabel(label);
      setSrc(url);
      setStatus(label === 'empty' ? 'empty' : 'loading');
      setCurrentTime(0);
      setDuration(0);
      setFirstFrameDuration(0);
    }, 0);
  };

  const automationActions: Record<string, () => void> = {
    'btn-clear-signals': clearSignals,
    'btn-play': () => invokeMethod('play'),
    'btn-play-null-params': () => invokeMethod('play', null),
    'btn-pause': () => invokeMethod('pause'),
    'btn-stop': () => invokeMethod('stop'),
    'btn-seek-start': () => invokeMethod('seek', { position: 0 }),
    'btn-seek': () => invokeMethod('seek', { position: 2 }),
    'btn-seek-near-end': () => invokeMethod('seek', { position: 9.5 }),
    'btn-seek-out-of-range': () => invokeMethod('seek', { position: 99 }),
    'btn-seek-missing-position': () => invokeMethod('seek'),
    'btn-seek-null-params': () => invokeMethod('seek', null),
    'btn-seek-string-position': () => invokeMethod('seek', { position: 'bad' }),
    'btn-seek-nan-position': () =>
      invokeMethod('seek', { position: Number.NaN }),
    'btn-seek-infinite-position': () =>
      invokeMethod('seek', { position: Number.POSITIVE_INFINITY }),
    'btn-seek-huge-position': () =>
      invokeMethod('seek', { position: Number.MAX_VALUE }),
    'btn-src-primary': () => selectSource('primary', PRIMARY_VIDEO_URL),
    'btn-src-secondary': () => selectSource('secondary', SECONDARY_VIDEO_URL),
    'btn-src-invalid': () => selectSource('invalid', INVALID_VIDEO_URL),
    'btn-src-empty': () => selectSource('empty', ''),
    'btn-play-then-src-empty': () => playThenSelectSource('empty', ''),
    'btn-play-pause-then-src-empty': () =>
      playPauseThenSelectSource('empty', ''),
    'btn-seek-then-src-empty': () => seekThenSelectSource('empty', ''),
    'btn-loop-on': () => setLoop(true),
    'btn-loop-off': () => setLoop(false),
    'btn-muted-on': () => setMuted(true),
    'btn-muted-off': () => setMuted(false),
    'btn-volume-low': () => setVolume(0.3),
    'btn-volume-high': () => setVolume(1),
    'btn-speed-half': () => setSpeed(0.5),
    'btn-speed-normal': () => setSpeed(1),
    'btn-speed-double': () => setSpeed(2),
    'btn-object-contain': () => setObjectFit('contain'),
    'btn-object-cover': () => setObjectFit('cover'),
    'btn-object-fill': () => setObjectFit('fill'),
    'btn-timeupdate-slow': () => setTimeUpdateInterval(1),
    'btn-timeupdate-zero': () => setTimeUpdateInterval(0),
    'btn-timeupdate-fast': () => setTimeUpdateInterval(0.001),
    'btn-mode-queue': () => setMode('queue'),
    'btn-mode-direct': () => setMode('direct'),
    'btn-mode-latest': () => setMode('latest'),
    'btn-burst-play-stop': () => invokeBurst([['play'], ['stop']]),
    'btn-burst-play-pause-stop': () =>
      invokeBurst([['play'], ['pause'], ['stop']]),
    'btn-method-unknown': () => invokeMethod('unknownVideoMethod'),
  };

  (globalThis as any).__videoE2EAction = (tag: string) => {
    const action = automationActions[tag];
    if (!action) {
      return false;
    }
    action();
    return true;
  };

  const attrState =
    `src=${srcLabel};loop=${loop};volume=${volume.toFixed(1)};` +
    `muted=${muted};speed=${speed.toFixed(1)};fit=${objectFit};` +
    `mode=${mode};requestedInterval=${timeUpdateInterval.toFixed(1)}`;

  return (
    <scroll-view className="root" scroll-y>
      <text className="title">Video E2E Demo</text>

      <video
        id="test-video"
        lynx-test-tag="test-video"
        className="video"
        src={src}
        loop={loop}
        volume={volume}
        muted={muted}
        speed={speed}
        object-fit={objectFit}
        mode={mode}
        timeupdate-interval={timeUpdateInterval}
        bindfirstframe={handleFirstFrame}
        bindplaying={handlePlaying}
        bindpaused={handlePaused}
        bindstopped={handleStopped}
        bindtimeupdate={handleTimeUpdate}
        bindended={handleEnded}
        bindlooped={handleLooped}
        binderror={handleError}
        bindbuffering={handleBuffering}
      />

      <text lynx-test-tag="status-text" className="status">
        {status}
      </text>

      <text lynx-test-tag="attr-state" className="status">
        {attrState}
      </text>

      <text lynx-test-tag="interval-state" className="time">
        {timeUpdateInterval.toFixed(3)}
      </text>

      <text lynx-test-tag="time-text" className="time">
        {currentTime.toFixed(1)} / {duration.toFixed(1)}
      </text>

      <text lynx-test-tag="firstframe-duration" className="time">
        {firstFrameDuration.toFixed(1)}
      </text>

      <text lynx-test-tag="last-error" className="time">
        {lastError}
      </text>

      <text lynx-test-tag="last-buffering" className="time">
        {lastBuffering.toFixed(1)}
      </text>

      <text lynx-test-tag="min-timeupdate-delta" className="time">
        {minTimeUpdateDelta}
      </text>

      <view className="controls">
        <view
          lynx-test-tag="btn-clear-signals"
          className="button"
          bindtap={clearSignals}
        >
          <text>Clear</text>
        </view>
        <view
          lynx-test-tag="btn-play"
          className="button"
          bindtap={() => invokeMethod('play')}
        >
          <text>Play</text>
        </view>
        <view
          lynx-test-tag="btn-play-null-params"
          className="button"
          bindtap={() => invokeMethod('play', null)}
        >
          <text>Play Null</text>
        </view>
        <view
          lynx-test-tag="btn-pause"
          className="button"
          bindtap={() => invokeMethod('pause')}
        >
          <text>Pause</text>
        </view>
        <view
          lynx-test-tag="btn-stop"
          className="button"
          bindtap={() => invokeMethod('stop')}
        >
          <text>Stop</text>
        </view>
      </view>

      <view className="controls">
        <view
          lynx-test-tag="btn-seek-start"
          className="button"
          bindtap={() => invokeMethod('seek', { position: 0 })}
        >
          <text>Seek 0s</text>
        </view>
        <view
          lynx-test-tag="btn-seek"
          className="button"
          bindtap={() => invokeMethod('seek', { position: 2 })}
        >
          <text>Seek 2s</text>
        </view>
        <view
          lynx-test-tag="btn-seek-near-end"
          className="button"
          bindtap={() => invokeMethod('seek', { position: 9.5 })}
        >
          <text>Near End</text>
        </view>
        <view
          lynx-test-tag="btn-seek-out-of-range"
          className="button"
          bindtap={() => invokeMethod('seek', { position: 99 })}
        >
          <text>Seek Bad</text>
        </view>
        <view
          lynx-test-tag="btn-seek-missing-position"
          className="button"
          bindtap={() => invokeMethod('seek')}
        >
          <text>Seek Missing</text>
        </view>
        <view
          lynx-test-tag="btn-seek-null-params"
          className="button"
          bindtap={() => invokeMethod('seek', null)}
        >
          <text>Seek Null</text>
        </view>
        <view
          lynx-test-tag="btn-seek-string-position"
          className="button"
          bindtap={() => invokeMethod('seek', { position: 'bad' })}
        >
          <text>Seek Text</text>
        </view>
        <view
          lynx-test-tag="btn-seek-nan-position"
          className="button"
          bindtap={() => invokeMethod('seek', { position: Number.NaN })}
        >
          <text>Seek NaN</text>
        </view>
        <view
          lynx-test-tag="btn-seek-infinite-position"
          className="button"
          bindtap={() =>
            invokeMethod('seek', { position: Number.POSITIVE_INFINITY })
          }
        >
          <text>Seek Inf</text>
        </view>
        <view
          lynx-test-tag="btn-seek-huge-position"
          className="button"
          bindtap={() => invokeMethod('seek', { position: Number.MAX_VALUE })}
        >
          <text>Seek Huge</text>
        </view>
      </view>

      <view className="controls">
        <view
          lynx-test-tag="btn-src-primary"
          className="button"
          bindtap={() => selectSource('primary', PRIMARY_VIDEO_URL)}
        >
          <text>Src A</text>
        </view>
        <view
          lynx-test-tag="btn-src-secondary"
          className="button"
          bindtap={() => selectSource('secondary', SECONDARY_VIDEO_URL)}
        >
          <text>Src B</text>
        </view>
        <view
          lynx-test-tag="btn-src-invalid"
          className="button"
          bindtap={() => selectSource('invalid', INVALID_VIDEO_URL)}
        >
          <text>Src Bad</text>
        </view>
        <view
          lynx-test-tag="btn-src-empty"
          className="button"
          bindtap={() => selectSource('empty', '')}
        >
          <text>Src Empty</text>
        </view>
        <view
          lynx-test-tag="btn-play-then-src-empty"
          className="button"
          bindtap={() => playThenSelectSource('empty', '')}
        >
          <text>Play Empty</text>
        </view>
        <view
          lynx-test-tag="btn-play-pause-then-src-empty"
          className="button"
          bindtap={() => playPauseThenSelectSource('empty', '')}
        >
          <text>Play Pause Empty</text>
        </view>
        <view
          lynx-test-tag="btn-seek-then-src-empty"
          className="button"
          bindtap={() => seekThenSelectSource('empty', '')}
        >
          <text>Seek Empty</text>
        </view>
      </view>

      <view className="controls">
        <view
          lynx-test-tag="btn-loop-on"
          className="button"
          bindtap={() => setLoop(true)}
        >
          <text>Loop On</text>
        </view>
        <view
          lynx-test-tag="btn-loop-off"
          className="button"
          bindtap={() => setLoop(false)}
        >
          <text>Loop Off</text>
        </view>
        <view
          lynx-test-tag="btn-muted-on"
          className="button"
          bindtap={() => setMuted(true)}
        >
          <text>Mute</text>
        </view>
        <view
          lynx-test-tag="btn-muted-off"
          className="button"
          bindtap={() => setMuted(false)}
        >
          <text>Unmute</text>
        </view>
      </view>

      <view className="controls">
        <view
          lynx-test-tag="btn-volume-low"
          className="button"
          bindtap={() => setVolume(0.3)}
        >
          <text>Vol 0.3</text>
        </view>
        <view
          lynx-test-tag="btn-volume-high"
          className="button"
          bindtap={() => setVolume(1)}
        >
          <text>Vol 1</text>
        </view>
        <view
          lynx-test-tag="btn-speed-half"
          className="button"
          bindtap={() => setSpeed(0.5)}
        >
          <text>0.5x</text>
        </view>
        <view
          lynx-test-tag="btn-speed-normal"
          className="button"
          bindtap={() => setSpeed(1)}
        >
          <text>1x</text>
        </view>
        <view
          lynx-test-tag="btn-speed-double"
          className="button"
          bindtap={() => setSpeed(2)}
        >
          <text>2x</text>
        </view>
      </view>

      <view className="controls">
        <view
          lynx-test-tag="btn-object-contain"
          className="button"
          bindtap={() => setObjectFit('contain')}
        >
          <text>Contain</text>
        </view>
        <view
          lynx-test-tag="btn-object-cover"
          className="button"
          bindtap={() => setObjectFit('cover')}
        >
          <text>Cover</text>
        </view>
        <view
          lynx-test-tag="btn-object-fill"
          className="button"
          bindtap={() => setObjectFit('fill')}
        >
          <text>Fill</text>
        </view>
        <view
          lynx-test-tag="btn-timeupdate-slow"
          className="button"
          bindtap={() => setTimeUpdateInterval(1)}
        >
          <text>1s Tick</text>
        </view>
        <view
          lynx-test-tag="btn-timeupdate-zero"
          className="button"
          bindtap={() => setTimeUpdateInterval(0)}
        >
          <text>Invalid 0s</text>
        </view>
        <view
          lynx-test-tag="btn-timeupdate-fast"
          className="button"
          bindtap={() => setTimeUpdateInterval(0.001)}
        >
          <text>Fast Tick</text>
        </view>
      </view>

      <view className="controls">
        <view
          lynx-test-tag="btn-mode-queue"
          className="button"
          bindtap={() => setMode('queue')}
        >
          <text>Queue</text>
        </view>
        <view
          lynx-test-tag="btn-mode-direct"
          className="button"
          bindtap={() => setMode('direct')}
        >
          <text>Direct</text>
        </view>
        <view
          lynx-test-tag="btn-mode-latest"
          className="button"
          bindtap={() => setMode('latest')}
        >
          <text>Latest</text>
        </view>
        <view
          lynx-test-tag="btn-burst-play-stop"
          className="button"
          bindtap={() => invokeBurst([['play'], ['stop']])}
        >
          <text>Play Stop</text>
        </view>
      </view>

      <view className="controls">
        <view
          lynx-test-tag="btn-burst-play-pause-stop"
          className="button"
          bindtap={() => invokeBurst([['play'], ['pause'], ['stop']])}
        >
          <text>Play Pause Stop</text>
        </view>
        <view
          lynx-test-tag="btn-method-unknown"
          className="button"
          bindtap={() => invokeMethod('unknownVideoMethod')}
        >
          <text>Unknown</text>
        </view>
      </view>

      <text lynx-test-tag="event-counts" className="log">
        {`firstframe=${eventCounts.firstframe};playing=${eventCounts.playing};` +
          `paused=${eventCounts.paused};stopped=${eventCounts.stopped};` +
          `timeupdate=${eventCounts.timeupdate};ended=${eventCounts.ended};` +
          `looped=${eventCounts.looped};error=${eventCounts.error};` +
          `buffering=${eventCounts.buffering}`}
      </text>

      <text lynx-test-tag="event-log" className="log">
        {eventLog.join(' | ')}
      </text>

      <text lynx-test-tag="callback-log" className="log">
        {callbackLog.join(' | ')}
      </text>

      <text lynx-test-tag="signal-log" className="log">
        {signalLog.join(' | ')}
      </text>

      <text lynx-test-tag="looped-state" className="result">
        {loopedSeen ? 'looped' : 'pending'}
      </text>

      <text lynx-test-tag="auto-result" className="result">
        {status === 'playing' ||
        status === 'paused' ||
        status === 'ended' ||
        status === 'stopped' ||
        status === 'error'
          ? 'success'
          : 'pending'}
      </text>
    </scroll-view>
  );
}

root.render(<VideoDemo />);
