# SCGLayer in the fragment renderer

The iOS fragment container renderer records fills, borders, gradients, box shadows,
text, and static images as CoreGraphics commands. Consecutive commands share
SCGLayer tiles. Native child views and animated images separate paint groups so
that their display-list order, playback controls, and events remain intact.
The page root uses the same drawing path. Custom native renderer hosts retain
their existing drawing path.

SCGLayer is adapted from [SuperCoreGraphicLayer](https://github.com/zhongyr/SuperCoreGraphicLayer)
at commit `05dce83`. Its original MIT license is included in this directory.
The integration is internal to the renderer and adds no Pod dependency.

## Availability and distribution

This is an **experimental private-API implementation**. It weakly imports
`CGIOSurfaceContextCreateWithOptions` and `CGIOSurfaceContextCreate`, and assigns
an IOSurface to `CALayer.contents`. Weak importing prevents a missing constructor
from causing a load-time link failure; it does not make these APIs public or
remove App Store review, OS compatibility, or distribution-policy risks.

Missing symbols select the existing layer renderer. A failed context allocation
uses public bitmap CoreGraphics drawing. Neither successful symbol lookup nor a
successful draw proves GPU execution or presentation completion. `CGContextFlush`
is submission, not a GPU completion fence.

## Resource behavior

Each tile owns at most three surfaces and never writes to the current front
surface or a surface still in use by Core Animation. Busy buffers retain the
previous frame and schedule another display pass. Tiles are reused across
consecutive display lists; removed groups and explicit reset release their pools.
Image completion invalidates drawing and rebuilds group boundaries when an image
switches between static and animated playback.

Occupied tiles are at most 2048 physical pixels per dimension. A display list
may allocate at most 16,777,216 pixels of tile area (up to roughly 192 MiB for
three BGRA buffers, excluding decoded images, alignment, and compositor resources). Larger or
invalid geometry uses the existing layer renderer for that update. Empty space
between distant fragments does not get its own surface. Resizing can temporarily
retain old compositor-owned surfaces as well as the new pool.

Drawing commands are replayed after each display-list update. This trades
Core Animation layer/rasterization work for application-side CoreGraphics work
and surface memory. Performance depends on scene size, changes, device, and OS;
this integration does not assert a universal speedup over the existing renderer.
