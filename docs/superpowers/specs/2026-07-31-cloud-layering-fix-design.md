# Cloud Layering Fix — Flicker-Free

## Problem

Clouds (`cloud1`, `cloud2`) drift leftward across the display. When a cloud overlaps a clock block (`hourBlock`, `minuteBlock`), the cloud's `fillRect` with `SKY_COLOR` erases the block. The current fix redraws blocks after clouds when they overlap, but this causes flickering because the block is erased (by the clear) before being redrawn — a visible flash on the no-buffer display.

## Solution

Cloud clips both its clear and its sprite draw around overlapping blocks. The cloud appears to go *behind* the blocks, and the blocks are never touched.

**Flow:**

```
Cloud::update():
  1. Clear old position (clipped around blocks)
  2. Move to new position
  3. Draw cloud sprite (clipped around blocks)
```

## Design

### 1. Clip Region Computation

The clipping helper takes the cloud's bounding box and a block's bounding box, and returns up to 4 rectangles that cover the cloud region *minus* the block region.

**Input:** Cloud bbox `(cx, cy, cw, ch)`, Block bbox `(bx, by, bw, bh)`

**Output:** Up to 4 `ClipRegion` entries:

```
If no overlap: 1 region = full cloud bbox
If overlap: 4 regions:
  top:    (cx, cy, cw, overlap_y - cy)
  bottom: (cx, overlap_y + overlap_h, cw, ch - overlap_h - (overlap_y - cy))
  left:   (cx, overlap_y, overlap_x - cx, overlap_h)
  right:  (overlap_x + overlap_w, overlap_y, cw - overlap_w - (overlap_x - cx), overlap_h)
```

**Sprite source offset:** Each clip region maps to a sub-rectangle of the sprite bitmap. The source offset `(sx, sy)` is `(region.x - cx, region.y - cy)`.

**Two-block clipping:** When both blocks overlap the cloud, we clip against block1 first, then clip each resulting region against block2. This can produce up to 16 regions in theory, but in practice blocks are fixed at (13,8) and (32,8) with 19×19 size, so overlap with both is rare and the result is manageable.

### 2. Cloud::update() becomes

```cpp
void Cloud::update() {
  clearOldPosition(hourBlock, minuteBlock);
  _x -= _speed;
  drawClipped(hourBlock, minuteBlock);
}
```

### 3. Clip helper (private method)

```cpp
void Cloud::getClipRegions(int bx, int by, int bw, int bh,
                           ClipRegion regions[], int& count) {
  // Compute intersection
  int ix = max(_x, bx);
  int iy = max(_y, by);
  int ix2 = min(_x + _width, bx + bw);
  int iy2 = min(_y + _height, by + bh);

  if (ix >= ix2 || iy >= iy2) {
    // No overlap — full region
    regions[0] = {_x, _y, _width, _height, 0, 0};
    count = 1;
    return;
  }

  // 4 rectangles excluding the intersection
  count = 0;
  // top
  if (iy > _y)
    regions[count++] = {_x, _y, _width, iy - _y, 0, 0};
  // bottom
  if (iy2 < _y + _height)
    regions[count++] = {_x, iy2, _width, (_y + _height) - iy2, 0, iy2 - _y};
  // left
  if (ix > _x)
    regions[count++] = {_x, iy, ix - _x, iy2 - iy, 0, iy - _y};
  // right
  if (ix2 < _x + _width)
    regions[count++] = {ix2, iy, (_x + _width) - ix2, iy2 - iy, ix2 - _x, iy - _y};
}
```

### 4. drawClipped()

For each clip region, draw the cloud sprite using `drawRGBBitmap` with adjusted source and destination coordinates:

```cpp
void Cloud::drawClipped(Block& block1, Block& block2) {
  ClipRegion regions[16];
  int count = 0;

  // Clip against block1
  getClipRegions(block1._x, block1._y, block1._width, block1._height,
                 regions, count);

  // Clip each region against block2
  ClipRegion final[16];
  int finalCount = 0;
  for (int i = 0; i < count; i++) {
    ClipRegion sub[4];
    int subCount = 0;
    getClipRegionAgainst(regions[i], block2, sub, subCount);
    for (int j = 0; j < subCount; j++)
      final[finalCount++] = sub[j];
  }

  // Draw each final region
  for (int i = 0; i < finalCount; i++) {
    ClipRegion& r = final[i];
    Locator::getDisplay()->drawRGBBitmap(
      r.x, r.y, MASKED + r.sy * _width + r.sx, r.w, r.h);
  }
}
```

### 5. clearOldPosition()

Same clipping logic, but uses `fillRect(SKY_COLOR)` instead of `drawRGBBitmap`.

## Edge Cases

- **Cloud fully inside block:** All 4 clip regions have zero area — nothing is cleared or drawn. Correct behavior.
- **Block fully inside cloud:** All 4 clip regions exist. Cloud is drawn around the block.
- **Both blocks overlap:** Clipped against block1 first, then each result against block2. Max 16 final regions, but in practice far fewer (blocks are side-by-side at x=13 and x=32, cloud is ~20px wide).
- **Cloud off-screen:** `_x` or `_y` negative, or beyond display bounds. The clipping math handles this naturally since `fillRect` and `drawRGBBitmap` clip to display bounds.

## Performance

- **Worst case:** 2 blocks × 4 regions each = 8 `fillRect` calls for clear, 8 `drawRGBBitmap` calls for draw. Compared to current: 1 `fillRect` + 1 `drawRGBBitmap` (plus conditional block redraw). Slightly more work, but all are small rectangles.
- **Typical case:** Cloud overlaps 0-1 blocks. 1-4 `fillRect` + 1-4 `drawRGBBitmap`. Very fast.
- **No per-pixel work:** All operations are rectangle-based, leveraging hardware acceleration.

## Removal of Current Fix

The existing overlap check and `block.redraw()` calls in `Clockface::update()` are removed — no longer needed since clouds never erase blocks.

## Files to Modify

| File | Change |
|------|--------|
| `gfx/cloud.h` | Add `ClipRegion` struct, `clearOldPosition()`, `drawClipped()`, clip helpers |
| `gfx/cloud.cpp` | Implement clipping logic |
| `gfx/block.h` | Add position/size getters (if not already present) |
| `gfx/block.cpp` | Implement getters |
| `Clockface.cpp` | Remove overlap check and block redraw logic |

## Verification

1. Compile: `cd ~/git/clockwise/firmware/esphome && uv run esphome compile clockwise.yaml`
2. Visual check: clouds drift behind the clock blocks, blocks never disappear or flicker
