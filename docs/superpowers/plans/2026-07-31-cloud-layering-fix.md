# Cloud Layering Fix — Flicker-Free Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix cloud-block flickering by clipping cloud clear and sprite draw around overlapping blocks.

**Architecture:** Cloud clips both its `fillRect` clear and `drawRGBBitmap` draw around overlapping blocks using AABB intersection math. Blocks are never erased, so no flickering.

**Tech Stack:** C++, Adafruit GFX, ESP32 (ESPHome)

## Global Constraints

- Display: 64×64 RGB565 via Adafruit GFX (`Locator::getDisplay()`)
- Compile from: `cd ~/git/clockwise/firmware/esphome && uv run esphome compile clockwise.yaml`
- Clock blocks: 19×19 pixels at positions (13, 8) and (32, 8)
- Cloud sprites: 20×15 pixels, moving leftward
- SKY_COLOR: 0x000E (defined in `gfx/assets.h`)
- DISPLAY_WIDTH: 64, DISPLAY_HEIGHT: 64 (defined in `cw-gfx-engine/Game.h`)

---

## File Structure

| File | Change | Responsibility |
|------|--------|----------------|
| `gfx/cloud.h` | Modify | Add `ClipRegion` struct, `clearOldPosition()`, `drawClipped()`, clip helpers |
| `gfx/cloud.cpp` | Modify | Implement clipping logic |
| `Clockface.cpp` | Modify | Remove overlap check and block redraw logic |

---

### Task 1: Add ClipRegion struct and clip helpers to Cloud

**Files:**
- Modify: `gfx/cloud.h` — add `ClipRegion` struct and private helper declarations
- Modify: `gfx/cloud.cpp` — add `getClipRegions()` and `getClipRegionAgainst()` implementations

**Interfaces:**
- Consumes: nothing (standalone addition)
- Produces: `ClipRegion` struct, `getClipRegions()`, `getClipRegionAgainst()`

- [ ] **Step 1: Add ClipRegion struct to cloud.h**

Add before the `Cloud` class declaration:

```cpp
struct ClipRegion {
  int x, y, w, h;  // destination on screen
  int sx, sy;       // source offset into sprite bitmap
};
```

- [ ] **Step 2: Add private helper declarations to cloud.h**

Inside the `Cloud` class `private` section, add after `void wrap();`:

```cpp
void getClipRegions(int bx, int by, int bw, int bh,
                    ClipRegion regions[], int& count);
void getClipRegionAgainst(ClipRegion& input, int bx, int by, int bw, int bh,
                          ClipRegion out[], int& outCount);
```

- [ ] **Step 3: Implement getClipRegions() in cloud.cpp**

Add before `Cloud::update()`:

```cpp
void Cloud::getClipRegions(int bx, int by, int bw, int bh,
                           ClipRegion regions[], int& count) {
  int ix = max(_x, bx);
  int iy = max(_y, by);
  int ix2 = min(_x + _width, bx + bw);
  int iy2 = min(_y + _height, by + bh);

  if (ix >= ix2 || iy >= iy2) {
    regions[0] = {_x, _y, _width, _height, 0, 0};
    count = 1;
    return;
  }

  count = 0;
  if (iy > _y)
    regions[count++] = {_x, _y, _width, iy - _y, 0, 0};
  if (iy2 < _y + _height)
    regions[count++] = {_x, iy2, _width, (_y + _height) - iy2, 0, iy2 - _y};
  if (ix > _x)
    regions[count++] = {_x, iy, ix - _x, iy2 - iy, 0, iy - _y};
  if (ix2 < _x + _width)
    regions[count++] = {ix2, iy, (_x + _width) - ix2, iy2 - iy, ix2 - _x, iy - _y};
}
```

- [ ] **Step 4: Implement getClipRegionAgainst() in cloud.cpp**

Add after `getClipRegions()`:

```cpp
void Cloud::getClipRegionAgainst(ClipRegion& input, int bx, int by, int bw, int bh,
                                 ClipRegion out[], int& outCount) {
  int ix = max(input.x, bx);
  int iy = max(input.y, by);
  int ix2 = min(input.x + input.w, bx + bw);
  int iy2 = min(input.y + input.h, by + bh);

  if (ix >= ix2 || iy >= iy2) {
    out[0] = input;
    outCount = 1;
    return;
  }

  outCount = 0;
  if (iy > input.y)
    out[outCount++] = {input.x, input.y, input.w, iy - input.y, input.sx, input.sy};
  if (iy2 < input.y + input.h)
    out[outCount++] = {input.x, iy2, input.w, (input.y + input.h) - iy2,
                       input.sx, input.sy + (iy2 - input.y)};
  if (ix > input.x)
    out[outCount++] = {input.x, iy, ix - input.x, iy2 - iy,
                       input.sx, input.sy + (iy - input.y)};
  if (ix2 < input.x + input.w)
    out[outCount++] = {ix2, iy, (input.x + input.w) - ix2, iy2 - iy,
                       input.sx + (ix2 - input.x), input.sy + (iy - input.y)};
}
```

- [ ] **Step 5: Compile and verify**

Run: `cd ~/git/clockwise/firmware/esphome && uv run esphome compile clockwise.yaml`
Expected: Compile succeeds with no errors.

- [ ] **Step 6: Commit**

```bash
git add gfx/cloud.h gfx/cloud.cpp
git commit -m "feat: add ClipRegion struct and clip helpers to Cloud"
```

---

### Task 2: Implement clearOldPosition() and drawClipped()

**Files:**
- Modify: `gfx/cloud.h` — add `clearOldPosition()` and `drawClipped()` declarations
- Modify: `gfx/cloud.cpp` — add implementations, update `update()`

**Interfaces:**
- Consumes: `ClipRegion`, `getClipRegions()`, `getClipRegionAgainst()` (Task 1), `Block` getters
- Produces: `clearOldPosition()`, `drawClipped()`, updated `update()`

- [ ] **Step 1: Add public method declarations to cloud.h**

Inside the `Cloud` class `public` section, add after `void update();`:

```cpp
void clearOldPosition(int b1x, int b1y, int b1w, int b1h,
                      int b2x, int b2y, int b2w, int b2h);
void drawClipped(int b1x, int b1y, int b1w, int b1h,
                 int b2x, int b2y, int b2w, int b2h);
```

- [ ] **Step 2: Implement clearOldPosition() in cloud.cpp**

Add before `Cloud::update()`:

```cpp
void Cloud::clearOldPosition(int b1x, int b1y, int b1w, int b1h,
                             int b2x, int b2y, int b2w, int b2h) {
  ClipRegion regions[16];
  int count = 0;
  getClipRegions(b1x, b1y, b1w, b1h, regions, count);

  ClipRegion final[16];
  int finalCount = 0;
  for (int i = 0; i < count; i++) {
    ClipRegion sub[4];
    int subCount = 0;
    getClipRegionAgainst(regions[i], b2x, b2y, b2w, b2h, sub, subCount);
    for (int j = 0; j < subCount; j++)
      final[finalCount++] = sub[j];
  }

  for (int i = 0; i < finalCount; i++) {
    ClipRegion& r = final[i];
    if (r.w > 0 && r.h > 0)
      Locator::getDisplay()->fillRect(r.x, r.y, r.w, r.h, SKY_COLOR);
  }
}
```

- [ ] **Step 3: Implement drawClipped() in cloud.cpp**

Add after `clearOldPosition()`:

```cpp
void Cloud::drawClipped(int b1x, int b1y, int b1w, int b1h,
                        int b2x, int b2y, int b2w, int b2h) {
  ClipRegion regions[16];
  int count = 0;
  getClipRegions(b1x, b1y, b1w, b1h, regions, count);

  ClipRegion final[16];
  int finalCount = 0;
  for (int i = 0; i < count; i++) {
    ClipRegion sub[4];
    int subCount = 0;
    getClipRegionAgainst(regions[i], b2x, b2y, b2w, b2h, sub, subCount);
    for (int j = 0; j < subCount; j++)
      final[finalCount++] = sub[j];
  }

  for (int i = 0; i < finalCount; i++) {
    ClipRegion& r = final[i];
    if (r.w > 0 && r.h > 0)
      Locator::getDisplay()->drawRGBBitmap(r.x, r.y,
        _sprite + r.sy * _width + r.sx, _mask + r.sy * _width + r.sx,
        r.w, r.h);
  }
}
```

- [ ] **Step 4: Update Cloud::update() to use new methods**

Replace the existing `Cloud::update()`:

```cpp
void Cloud::update() {
  unsigned long now = millis();
  if (now - _lastMillis >= _interval) {
    clearOldPosition(13, 8, 19, 19, 32, 8, 19, 19);
    _x -= 1;
    wrap();
    drawClipped(13, 8, 19, 19, 32, 8, 19, 19);
    _lastMillis = now;
  }
}
```

- [ ] **Step 5: Compile and verify**

Run: `cd ~/git/clockwise/firmware/esphome && uv run esphome compile clockwise.yaml`
Expected: Compile succeeds with no errors.

- [ ] **Step 6: Commit**

```bash
git add gfx/cloud.h gfx/cloud.cpp
git commit -m "feat: implement cloud-aware clear and clipped sprite draw"
```

---

### Task 3: Remove overlap check from Clockface

**Files:**
- Modify: `Clockface.cpp` — remove overlap check and block redraw logic

**Interfaces:**
- Consumes: Cloud now handles its own clipping (Task 2)
- Produces: simplified `Clockface::update()`

- [ ] **Step 1: Remove overlap check from Clockface::update()**

Replace the current `Clockface::update()` with:

```cpp
void Clockface::update() {
  cloud1.update();
  cloud2.update();

  hourBlock.update();
  minuteBlock.update();

  mario.update();

  if (_dateTime->getSecond() == 0 && millis() - lastMillis > 1000) {
    mario.jump();
    updateTime();
    lastMillis = millis();
  }
}
```

- [ ] **Step 2: Remove cloudsOverlapBlock() function**

Delete the `cloudsOverlapBlock()` function and its forward declaration (lines 5 and 100-105).

- [ ] **Step 3: Compile and verify**

Run: `cd ~/git/clockwise/firmware/esphome && uv run esphome compile clockwise.yaml`
Expected: Compile succeeds with no errors.

- [ ] **Step 4: Commit**

```bash
git add Clockface.cpp
git commit -m "refactor: remove overlap check from Clockface, cloud handles its own clipping"
```
