#pragma once

#include <Arduino.h>
#include <Game.h>
#include <EventTask.h>

struct ClipRegion {
  int x, y, w, h;  // destination on screen
  int sx, sy;       // source offset into sprite bitmap
};

class Cloud: public Sprite, public EventTask {
  private:
    const unsigned short* _sprite;
    const uint8_t* _mask;
    int _interval;       // tick interval in milliseconds
    unsigned long _lastMillis;
    void wrap();
    void getClipRegions(int bx, int by, int bw, int bh,
                        ClipRegion regions[], int& count);
    void getClipRegionAgainst(ClipRegion& input, int bx, int by, int bw, int bh,
                              ClipRegion out[], int& outCount);

  public:
    Cloud(const unsigned short* sprite, const uint8_t* mask, int width, int height,
          int startX, int startY, int interval);
    void init();
    void update();
    void clearOldPosition(int b1x, int b1y, int b1w, int b1h,
                          int b2x, int b2y, int b2w, int b2h);
    void drawClipped(int b1x, int b1y, int b1w, int b1h,
                     int b2x, int b2y, int b2w, int b2h);
    void execute(EventType event, Sprite* caller);
    const char* name();
    int getX();
    int getY();
    int getWidth();
    int getHeight();
};
