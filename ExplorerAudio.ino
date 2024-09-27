#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_RA8875.h"

#define SCK_PIN   18  // Default SCK
#define MOSI_PIN  23  // Default MOSI
#define MISO_PIN  22  // Remapped MISO to GPIO22
#define RA8875_CS 5
#define RA8875_RESET 4

Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS, RA8875_RESET);
uint16_t tx, ty;

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

bool connected = true;

void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  Serial.printf("==> AVRC metadata rsp: attribute id 0x%x, %s\n", id, text);
  if (id == ESP_AVRC_MD_ATTR_PLAYING_TIME) {
    uint32_t playtime = String((char*)text).toInt();
    Serial.printf("==> Playing time is %d ms (%d seconds)\n", playtime, (int)round(playtime/1000.0));
  }
}

void setup() {
  auto cfg = i2s.defaultConfig();
  cfg.pin_bck = 26;
  cfg.pin_ws = 25;
  cfg.pin_data = 19;
  i2s.begin(cfg);
  Serial.begin(115200);

  //Serial.println("RA8875 start");
  //if (!tft.begin(RA8875_800x480)) {
  //  Serial.println("RA8875 Not Found!");
  //while (1);
  //}

  tft.displayOn(true);
  tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
  tft.PWM1out(255);
  tft.fillScreen(RA8875_BLACK);
  tft.textMode();
  tft.cursorBlink(32);

  tft.textSetCursor(10, 10);

  /* Render some text! */
  char string[15] = "Hello, World! ";
  tft.textTransparent(RA8875_WHITE);
  tft.textWrite(string);
  tft.textColor(RA8875_WHITE, RA8875_RED);
  tft.textWrite(string);
  tft.textTransparent(RA8875_CYAN);
  tft.textWrite(string);
  tft.textTransparent(RA8875_GREEN);
  tft.textWrite(string);
  tft.textColor(RA8875_YELLOW, RA8875_CYAN);
  tft.textWrite(string);
  tft.textColor(RA8875_BLACK, RA8875_MAGENTA);
  tft.textWrite(string);

  /* Change the cursor location and color ... */
  tft.textSetCursor(100, 100);
  tft.textTransparent(RA8875_RED);
  /* If necessary, enlarge the font */
  tft.textEnlarge(1);
  /* ... and render some more text! */
  tft.textWrite(string);
  tft.textSetCursor(100, 150);
  tft.textEnlarge(2);
  tft.textWrite(string);
/////////////////////////////////////////////////////
  a2dp_sink.set_avrc_metadata_attribute_mask(ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_PLAYING_TIME );
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);

  a2dp_sink.set_auto_reconnect(true);
  a2dp_sink.start("Explorer Audio");
}

void loop() {
  delay(60000);  // do nothing
}
