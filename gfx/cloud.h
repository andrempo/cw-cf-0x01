#pragma once

#include <Arduino.h>
#include <Game.h>
#include <EventTask.h>

class Cloud: public Sprite, public EventTask {
  private:
    const unsigned short* _sprite;
    const uint8_t* _mask;
    int _interval;       // tick interval in milliseconds
    unsigned long _lastMillis;
    void wrap();

  public:
    Cloud(const unsigned short* sprite, const uint8_t* mask, int width, int height,
          int startX, int startY, int interval);
    void init();
    void update();
    void execute(EventType event, Sprite* caller);
    const char* name();
};
