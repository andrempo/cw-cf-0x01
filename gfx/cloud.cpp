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
