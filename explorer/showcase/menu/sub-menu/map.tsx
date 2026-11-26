// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { root, useState } from '@lynx-js/react';

const stops = [
  {
    name: 'Ferry Building',
    coordinate: { latitude: 37.7955, longitude: -122.3937 },
  },
  { name: 'Pier 14', coordinate: { latitude: 37.7914, longitude: -122.3904 } },
];

function MapExample() {
  const [stopIndex, setStopIndex] = useState(0);
  const [selected, setSelected] = useState(false);
  const stop = stops[stopIndex];
  return (
    <view
      style={{
        height: '100%',
        display: 'flex',
        flexDirection: 'column',
        backgroundColor: '#fff',
      }}
    >
      <text style={{ fontSize: '24px', fontWeight: 'bold', margin: '16px' }}>
        San Francisco waterfront
      </text>
      <map
        style={{ width: '100%', flex: 1 }}
        center={stop.coordinate}
        scale={15}
      >
        <map-marker
          position={stop.coordinate}
          anchor={[0.5, 0.5]}
          style={{
            width: '112px',
            height: '38px',
            borderRadius: '19px',
            backgroundColor: '#16584c',
            alignItems: 'center',
            justifyContent: 'center',
          }}
          bindclickmarker={() => setSelected(true)}
        >
          <text style={{ color: '#fff', fontSize: '14px', fontWeight: 'bold' }}>
            {stop.name}
          </text>
        </map-marker>
      </map>
      <text style={{ fontSize: '15px', margin: '16px' }}>
        {selected ? `Selected ${stop.name}` : 'Tap the marker to select it.'}
      </text>
      <view
        style={{
          backgroundColor: '#16584c',
          padding: '14px',
          alignItems: 'center',
        }}
        bindtap={() => {
          setSelected(false);
          setStopIndex(1 - stopIndex);
        }}
      >
        <text style={{ color: '#fff', fontSize: '16px' }}>
          Show {stops[1 - stopIndex].name}
        </text>
      </view>
    </view>
  );
}

root.render(<MapExample />);
