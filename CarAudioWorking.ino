
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_RA8875.h"

#define SCK_PIN   18  // Default SCK
#define MOSI_PIN  23  // Default MOSI
#define MISO_PIN  22  // Remapped MISO to GPIO22 due to IOS audio quality weirdness
#define RA8875_CS 5
#define RA8875_RESET 4

bool callPressed = false;
bool prevSongPressed = false;
bool playPausePressed = false;
bool nextSongPressed = false;
bool volumeDownPressed = false;
bool volumeUpPressed = false;

String songTitle = "";
String songArtist = "";
String songAlbum = "";

Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS, RA8875_RESET);

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

bool isPlaying = false;


////////////////////////////////// Metadata String Handling //////////////////////////////////

void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  Serial.printf("==> AVRC metadata rsp: attribute id 0x%x, %s\n", id, text);

  switch (id) {
    case ESP_AVRC_MD_ATTR_TITLE:
      songTitle = String((char*)text);
      break;
    case ESP_AVRC_MD_ATTR_ARTIST:
      songArtist = String((char*)text);
      break;
    case ESP_AVRC_MD_ATTR_ALBUM:
      songAlbum = String((char*)text);
      break;
  }

  updateDisplay();
}


void playbackStateCallback(esp_a2d_audio_state_t state, void* param) {
  if (state == ESP_A2D_AUDIO_STATE_STARTED) {
    Serial.println("Playing");
    isPlaying = true;
  } else if (state == ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND || state == ESP_A2D_AUDIO_STATE_STOPPED) {
    Serial.println("Paused");
    isPlaying = false;
  }
}

////////////////////////////////// Update Display //////////////////////////////////

void updateDisplay() {
  tft.fillScreen(RA8875_BLACK);
  tft.textMode(); 
  tft.textEnlarge(2.5);
  
  // Display metadata on the screen
  tft.textColor(RA8875_GREEN, RA8875_BLACK);  // Green text on black background to match other interfaces

  tft.textSetCursor(10, 50);   // Line 1: Title

  tft.textWrite(songTitle.c_str());

  tft.textSetCursor(10, 210);  // Line 2: Artist

  tft.textWrite(songArtist.c_str());

  tft.textSetCursor(10, 370);  // Line 3: Album

  tft.textWrite(songAlbum.c_str());

  // hide cursor
  tft.textSetCursor(10, 360);
  tft.textColor(RA8875_BLACK, RA8875_BLACK);
  tft.textWrite("");
  delay(1000);
}

////////////////////////////////// Setup //////////////////////////////////

void setup() {
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, RA8875_CS); //enable SPI or else is still uses default maps
  auto cfg = i2s.defaultConfig();
  cfg.pin_bck = 26; //External DAC BCK to DAC2 of ESP32
  cfg.pin_ws = 25; //External DAC RCK to DAC1 of ESP32
  cfg.pin_data = 19; //DAC audio data out to pin 19 (MISO) this is due to some weirdness with only IOS devices sounding awful on pin22, something with cross muxing idk, pin 19 works great though
  i2s.begin(cfg);
  Serial.begin(115200);

  // Initialize display
  Serial.println("RA8875 start");
  if (!tft.begin(RA8875_800x480)) {
    Serial.println("RA8875 Not Found!");
    while (1);
  }
  
  tft.displayOn(true);
  tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
  tft.PWM1out(255);
  tft.fillScreen(RA8875_BLACK);
  tft.textMode();
  tft.cursorBlink(32);

  // Initialize Bluetooth
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.set_on_audio_state_changed(playbackStateCallback);
  a2dp_sink.set_avrc_rn_play_pos_callback(avrc_rn_play_pos_callback);
  a2dp_sink.set_auto_reconnect(true);
  a2dp_sink.start("Device Name");

  updateDisplay();
}

////////////////////////////////// Loop //////////////////////////////////

void loop() {
  delay(1000); // Add a delay to prevent rapid updates
}
