#include "cloud.h"
#include <Locator.h>
#include "assets.h"

Cloud::Cloud(const unsigned short* sprite, const uint8_t* mask, int width, int height,
             int startX, int startY, int interval) {
  _sprite = sprite;
  _mask = mask;
  _width = width;
  _height = height;
  _x = startX;
  _y = startY;
  _interval = interval;
  _lastMillis = 0;
}

void Cloud::getClipRegions(int bx, int by, int bw, int bh,
                           ClipRegion regions[], int& count) {
  int cx = _x, cy = _y, cw = _width, ch = _height;
  int ix = max(cx, bx);
  int iy = max(cy, by);
  int ix2 = min(cx + cw, bx + bw);
  int iy2 = min(cy + ch, by + bh);

  if (ix >= ix2 || iy >= iy2) {
    regions[0] = {cx, cy, cw, ch, 0, 0};
    count = 1;
    return;
  }

  count = 0;
  if (iy > cy)
    regions[count++] = {cx, cy, cw, iy - cy, 0, 0};
  if (iy2 < cy + ch)
    regions[count++] = {cx, iy2, cw, (cy + ch) - iy2, 0, iy2 - cy};
  if (ix > cx)
    regions[count++] = {cx, iy, ix - cx, iy2 - iy, 0, iy - cy};
  if (ix2 < cx + cw)
    regions[count++] = {ix2, iy, (cx + cw) - ix2, iy2 - iy, ix2 - cx, iy - cy};
}

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

void Cloud::init() {
  Locator::getEventBus()->subscribe(this);
  Locator::getDisplay()->drawRGBBitmap(_x, _y, _sprite, _mask, _width, _height);
}

void Cloud::update() {
  unsigned long now = millis();
  if (now - _lastMillis >= _interval) {
    // Clear old position with sky color
    Locator::getDisplay()->fillRect(_x, _y, _width, _height, SKY_COLOR);
    // Move one pixel left
    _x -= 1;
    // Wrap around when fully off the left edge
    wrap();
    // Redraw at new position with transparency mask
    Locator::getDisplay()->drawRGBBitmap(_x, _y, _sprite, _mask, _width, _height);
    _lastMillis = now;
  }
}

void Cloud::wrap() {
  if (_x + _width < 0) {
    _x = DISPLAY_WIDTH;
  }
}

void Cloud::execute(EventType event, Sprite* caller) {
  // No-op — no collision handling needed per spec
}

const char* Cloud::name() {
  return "CLOUD";
}

int Cloud::getX() { return _x; }
int Cloud::getY() { return _y; }
int Cloud::getWidth() { return _width; }
int Cloud::getHeight() { return _height; }
