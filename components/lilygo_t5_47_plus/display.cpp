#include "display.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#ifdef USE_ESP32_FRAMEWORK_ARDUINO

#include <esp32-hal-gpio.h>
#include <esp_heap_caps.h>
#include <esp_sleep.h>

namespace esphome {
namespace lilygo_t5_47_plus {

static const char *const TAG = "lilygo_t5_47_plus";

void LilygoT5_47PlusDisplay::setup() {
  ESP_LOGV(TAG, "Initialize called");

  ESP_LOGV(TAG, "Free heap: %d bytes", heap_caps_get_free_size(MALLOC_CAP_8BIT));
  ESP_LOGV(TAG, "Free PSRAM: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  ESP_LOGV(TAG, "Largest free PSRAM block: %d bytes", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));

  epd_init();
  uint32_t buffer_size = this->get_buffer_length_();
  ESP_LOGD(TAG, "Buffer size: %u bytes", buffer_size);

  this->buffer_ = (uint8_t *) heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (this->buffer_ == nullptr) {
    ESP_LOGE(TAG, "Could not allocate display buffer (%u bytes). Is PSRAM enabled?", buffer_size);
    this->mark_failed();
    return;
  }

  memset(this->buffer_, 0xFF, buffer_size);

  this->old_buffer_ = (uint8_t *) heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (this->old_buffer_ == nullptr) {
    ESP_LOGE(TAG, "Could not allocate old display buffer (%u bytes). Is PSRAM enabled?", buffer_size);
    this->mark_failed();
    return;
  }
  memset(this->old_buffer_, 0xFF, buffer_size);

  this->diff_buffer_ = (uint8_t *) heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (this->diff_buffer_ == nullptr) {
    ESP_LOGE(TAG, "Could not allocate diff display buffer (%u bytes). Is PSRAM enabled?", buffer_size);
    this->mark_failed();
    return;
  }
}

float LilygoT5_47PlusDisplay::get_setup_priority() const { return setup_priority::PROCESSOR; }
size_t LilygoT5_47PlusDisplay::get_buffer_length_() {
  return this->get_width_internal() * this->get_height_internal() / 2;
}

void LilygoT5_47PlusDisplay::update() {
  uint32_t t0 = esphome::millis();
  this->do_update_();
  uint32_t t1 = esphome::millis();
  this->display();
  uint32_t t2 = esphome::millis();
  ESP_LOGV(TAG, "update(): do_update_=%ums, display=%ums, total=%ums", t1 - t0, t2 - t1, t2 - t0);
}

void HOT LilygoT5_47PlusDisplay::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x >= this->get_width_internal() || y >= this->get_height_internal() || x < 0 || y < 0)
    return;
  // ESPHome's logical COLOR_ON is a marked pixel. On E-Paper that means black,
  // while COLOR_OFF is the unmarked white paper background.
  uint8_t luminance =
      (color.red * 2126 / 10000) + (color.green * 7152 / 10000) + (color.blue * 722 / 10000);
  uint8_t gs = 255 - luminance;
  epd_draw_pixel(x, y, gs, this->buffer_);
}

void LilygoT5_47PlusDisplay::fill(Color color) {
  if (this->buffer_ == nullptr)
    return;
  // Match draw_absolute_pixel_internal(): logical OFF is white paper and ON is
  // black ink.
  uint8_t luminance =
      (color.red * 2126 / 10000) + (color.green * 7152 / 10000) + (color.blue * 722 / 10000);
  uint8_t gs = 255 - luminance;
  // 4-bit per pixel: pack same value into both nibbles
  uint8_t fill_byte = (gs & 0xF0) | (gs >> 4);
  memset(this->buffer_, fill_byte, this->get_buffer_length_());
}

void LilygoT5_47PlusDisplay::dump_config() {
  LOG_DISPLAY("", "LilygoT5_47PlusDisplay", this);
  LOG_UPDATE_INTERVAL(this);
  // Log why the ESP32-S3 woke up — helps verify deep sleep is working
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  switch (cause) {
    case ESP_SLEEP_WAKEUP_UNDEFINED:
      ESP_LOGCONFIG(TAG, "  Wakeup cause: power-on / normal reset");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      ESP_LOGCONFIG(TAG, "  Wakeup cause: timer (deep sleep)");
      break;
    case ESP_SLEEP_WAKEUP_EXT0:
      ESP_LOGCONFIG(TAG, "  Wakeup cause: ext0 pin");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      ESP_LOGCONFIG(TAG, "  Wakeup cause: ext1 pin(s)");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      ESP_LOGCONFIG(TAG, "  Wakeup cause: touch pad");
      break;
    default:
      ESP_LOGCONFIG(TAG, "  Wakeup cause: other (%d)", (int) cause);
      break;
  }
}

void LilygoT5_47PlusDisplay::display() {
  uint32_t t0 = esphome::millis();
  ESP_LOGD(TAG, "Refreshing EPD...");

  epd_poweron();
  uint32_t t1 = esphome::millis();

  bool full = this->first_draw_ ||
              (this->full_update_every_ > 0 && this->partial_count_ + 1 >= this->full_update_every_);
  uint32_t t2, t3;
  if (full) {
    epd_clear();
    t2 = esphome::millis();
    epd_draw_grayscale_image(epd_full_screen(), this->buffer_);
    t3 = esphome::millis();
    this->partial_count_ = 0;
    this->first_draw_ = false;
  } else {
    // Pixel-diff partial update: build erase/draw masks holding only CHANGED
    // pixels (unchanged nibbles forced to white = no ink applied), so
    // untouched screen areas do not flicker at all.
    uint32_t len = this->get_buffer_length_();
    for (uint32_t i = 0; i < len; i++) {
      uint8_t o = this->old_buffer_[i], n = this->buffer_[i];
      uint8_t olo = o & 0x0F, ohi = o >> 4, nlo = n & 0x0F, nhi = n >> 4;
      uint8_t e = (uint8_t)(((olo == nlo) ? 0x0F : olo) | ((ohi == nhi) ? 0xF0 : (ohi << 4)));
      uint8_t d = (uint8_t)(((olo == nlo) ? 0x0F : nlo) | ((ohi == nhi) ? 0xF0 : (nhi << 4)));
      this->old_buffer_[i] = e;   // erase mask: old ink only where pixel changed
      this->diff_buffer_[i] = d;  // draw mask: new ink only where pixel changed
    }
    epd_draw_image(epd_full_screen(), this->old_buffer_, WHITE_ON_WHITE);
    t2 = esphome::millis();
    epd_draw_image(epd_full_screen(), this->diff_buffer_, BLACK_ON_WHITE);
    t3 = esphome::millis();
    this->partial_count_++;
  }
  memcpy(this->old_buffer_, this->buffer_, this->get_buffer_length_());

  epd_poweroff();
  uint32_t t4 = esphome::millis();

  ESP_LOGD(TAG, "EPD refresh done (%s): poweron=%ums clear/erase=%ums draw=%ums poweroff=%ums total=%ums",
           full ? "full" : "partial", t1 - t0, t2 - t1, t3 - t2, t4 - t3, t4 - t0);
}

void LilygoT5_47PlusDisplay::on_safe_shutdown() {
  // Fully power off the EPD supply before entering deep sleep.
  // The e-paper retains its image without power, so this is safe.
  epd_poweroff_all();
  ESP_LOGD(TAG, "EPD powered off for shutdown");
}

}  // namespace lilygo_t5_47_plus
}  // namespace esphome

#endif  // USE_ESP32_FRAMEWORK_ARDUINO
