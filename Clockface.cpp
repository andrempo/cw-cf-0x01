
#include "Clockface.h"

// Forward declaration
bool cloudsOverlapBlock(Cloud& cloud, Block& block);

EventBus eventBus;

const char* FORMAT_TWO_DIGITS = "%02d";

// Graphical elements
Tile ground(GROUND, 8, 8); 

Object bush(BUSH, 21, 9);
Cloud cloud1(CLOUD_NEW, CLOUD_MASK, 20, 15, 0, 21, 1000);
Cloud cloud2(CLOUD_NEW, CLOUD_MASK, 20, 15, 51, 7, 2000);
Object hill(HILL, 20, 22);


Mario mario(23, 40);
Block hourBlock(13, 8);
Block minuteBlock(32, 8);

unsigned long lastMillis = 0;

Clockface::Clockface(Adafruit_GFX* display) {
  _display = display;

  Locator::provide(display);
  Locator::provide(&eventBus);
}

void Clockface::setup(CWDateTime *dateTime) {
  _dateTime = dateTime;

  Locator::getDisplay()->setFont(&Super_Mario_Bros__24pt7b);
  Locator::getDisplay()->fillRect(0, 0, 64, 64, SKY_COLOR);

  ground.fillRow(DISPLAY_HEIGHT - ground._height);

  bush.draw(43, 47);
  hill.draw(0, 34);
  cloud1.init();
  cloud2.init();

  updateTime();


  hourBlock.init();
  minuteBlock.init();
  mario.init();
}

void Clockface::update() {
  // Move clouds first (clears old position with sky color, draws at new)
  cloud1.update();
  cloud2.update();

  // Blocks draw/update on top (they clear+redraw themselves every frame)
  hourBlock.update();
  minuteBlock.update();

  // If a cloud moved over a block, redraw that block on top
  if (cloudsOverlapBlock(cloud1, hourBlock)) {
    Locator::getDisplay()->drawRGBBitmap(hourBlock.getX(), hourBlock.getY(), BLOCK, 19, 19);
  }
  if (cloudsOverlapBlock(cloud1, minuteBlock)) {
    Locator::getDisplay()->drawRGBBitmap(minuteBlock.getX(), minuteBlock.getY(), BLOCK, 19, 19);
  }
  if (cloudsOverlapBlock(cloud2, hourBlock)) {
    Locator::getDisplay()->drawRGBBitmap(hourBlock.getX(), hourBlock.getY(), BLOCK, 19, 19);
  }
  if (cloudsOverlapBlock(cloud2, minuteBlock)) {
    Locator::getDisplay()->drawRGBBitmap(minuteBlock.getX(), minuteBlock.getY(), BLOCK, 19, 19);
  }

  mario.update();

  if (_dateTime->getSecond() == 0 && millis() - lastMillis > 1000) {
    mario.jump();
    updateTime();
    lastMillis = millis();

    //Serial.println(_dateTime->getFormattedTime());
  }
}

void Clockface::updateTime() {
  hourBlock.setText(String(_dateTime->getHour()));
  minuteBlock.setText(String(_dateTime->getMinute(FORMAT_TWO_DIGITS)));
}

void Clockface::externalEvent(int type) {
  if (type == 0) {  //TODO create an enum
    mario.jump();
    updateTime();
  }
}

bool cloudsOverlapBlock(Cloud& cloud, Block& block) {
  return cloud.getX() < block.getX() + block.getWidth() &&
         cloud.getX() + cloud.getWidth() > block.getX() &&
         cloud.getY() < block.getY() + block.getHeight() &&
         cloud.getY() + cloud.getHeight() > block.getY();
}
