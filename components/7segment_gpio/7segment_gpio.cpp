#include "7segment_gpio.h"
#include <assert.h>

namespace esphome {
namespace lcd_digits {

namespace {
constexpr uint8_t UNKNOWN_CHAR = 0xff;
/**
 * @brief symbol map
 *
 */
constexpr uint8_t ASCII_TO_RAW[95] = {
    0b00000000,   // ' ', ord 0x20
    0b10110000,   // '!', ord 0x21
    0b00100010,   // '"', ord 0x22
    UNKNOWN_CHAR, // '#', ord 0x23
    UNKNOWN_CHAR, // '$', ord 0x24
    0b01001001,   // '%', ord 0x25
    UNKNOWN_CHAR, // '&', ord 0x26
    0b00000010,   // ''', ord 0x27
    0b01001110,   // '(', ord 0x28
    0b01111000,   // ')', ord 0x29
    0b01000000,   // '*', ord 0x2A
    UNKNOWN_CHAR, // '+', ord 0x2B
    0b00010000,   // ',', ord 0x2C
    0b00000001,   // '-', ord 0x2D
    0b10000000,   // '.', ord 0x2E
    UNKNOWN_CHAR, // '/', ord 0x2F
    0b01111110,   // '0', ord 0x30
    0b00110000,   // '1', ord 0x31
    0b01101101,   // '2', ord 0x32
    0b01111001,   // '3', ord 0x33
    0b00110011,   // '4', ord 0x34
    0b01011011,   // '5', ord 0x35
    0b01011111,   // '6', ord 0x36
    0b01110000,   // '7', ord 0x37
    0b01111111,   // '8', ord 0x38
    0b01111011,   // '9', ord 0x39
    0b01001000,   // ':', ord 0x3A
    0b01011000,   // ';', ord 0x3B
    0b01000011,   // '<', ord 0x3C
    0b00001001,   // '=', ord 0x3D
    0b01100001,   // '>', ord 0x3E
    0b01100101,   // '?', ord 0x3F
    0b01101111,   // '@', ord 0x40
    0b01110111,   // 'A', ord 0x41
    0b01111111,   // 'B', ord 0x42
    0b01001110,   // 'C', ord 0x43
    0b01111001,   // 'D', ord 0x44
    0b01001111,   // 'E', ord 0x45
    0b01000111,   // 'F', ord 0x46
    0b01011110,   // 'G', ord 0x47
    0b00110111,   // 'H', ord 0x48
    0b00000110,   // 'I', ord 0x49
    0b00111000,   // 'J', ord 0x4A
    0b01010111,   // 'K', ord 0x4B
    0b00001110,   // 'L', ord 0x4C
    0b01101011,   // 'M', ord 0x4D
    0b01110110,   // 'N', ord 0x4E
    0b01111110,   // 'O', ord 0x4F
    0b01100111,   // 'P', ord 0x50
    0b01101011,   // 'Q', ord 0x51
    0b01101111,   // 'R', ord 0x52
    0b01011011,   // 'S', ord 0x53
    0b01000110,   // 'T', ord 0x54
    0b00111110,   // 'U', ord 0x55
    0b00111010,   // 'V', ord 0x56
    0b01011100,   // 'W', ord 0x57
    0b01001001,   // 'X', ord 0x58
    0b00101011,   // 'Y', ord 0x59
    0b01101101,   // 'Z', ord 0x5A
    0b01001110,   // '[', ord 0x5B
    UNKNOWN_CHAR, // '\', ord 0x5C
    0b01111000,   // ']', ord 0x5D
    UNKNOWN_CHAR, // '^', ord 0x5E
    0b00001000,   // '_', ord 0x5F
    0b00100000,   // '`', ord 0x60
    0b00011001,   // 'a', ord 0x61
    0b00011111,   // 'b', ord 0x62
    0b00001101,   // 'c', ord 0x63
    0b00111101,   // 'd', ord 0x64
    0b00001100,   // 'e', ord 0x65
    0b00000111,   // 'f', ord 0x66
    0b01001101,   // 'g', ord 0x67
    0b00010111,   // 'h', ord 0x68
    0b01001100,   // 'i', ord 0x69
    0b01011000,   // 'j', ord 0x6A
    0b01011000,   // 'k', ord 0x6B
    0b00000110,   // 'l', ord 0x6C
    0b01010101,   // 'm', ord 0x6D
    0b00010101,   // 'n', ord 0x6E
    0b00011101,   // 'o', ord 0x6F
    0b01100111,   // 'p', ord 0x70
    0b01110011,   // 'q', ord 0x71
    0b00000101,   // 'r', ord 0x72
    0b00011000,   // 's', ord 0x73
    0b00001111,   // 't', ord 0x74
    0b00011100,   // 'u', ord 0x75
    0b00011000,   // 'v', ord 0x76
    0b00101010,   // 'w', ord 0x77
    0b00001001,   // 'x', ord 0x78
    0b00111011,   // 'y', ord 0x79
    0b00001001,   // 'z', ord 0x7A
    0b00110001,   // '{', ord 0x7B
    0b00000110,   // '|', ord 0x7C
    0b00000111,   // '}', ord 0x7D
    0b01100011,   // '~', ord 0x7E (degree symbol)
};

LcdDigitsData *g_interrupt_data = nullptr;
static void IRAM_ATTR HOT s_timer_intr() {

  g_interrupt_data->timer_interrupt();
}
static void IRAM_ATTR HOT s_timer_setup() {
  timer1_isr_init();
}
}
static void IRAM_ATTR HOT s_timer_enable() {
  timer1_attachInterrupt(s_timer_intr);     
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
  timer1_write(250);
}

void IRAM_ATTR HOT LcdDigitsData::timer_interrupt() {
  // return;
  if (cycles_to_skip > 0) {
    cycles_to_skip--;
    return;
  }

  auto invert_if_not = [](bool value, bool condition) {
    return condition ? value : !value;
  };

  auto digit_level = [&](bool on) {
    const auto digit_on_level = display_type == CommonAnode;
    return invert_if_not(digit_on_level, on);
  };

  auto segment_level = [&](bool on) {
    const auto segment_on_level = display_type != CommonAnode;
    return invert_if_not(segment_on_level, on);
  };

  uint8_t bit_count = 0;
  if (iterate_digits) {

    // turn off digit
    if (auto digit_pin = digit_pins[current_frame])
      digit_pin->digital_write(digit_level(false));

    // switch to next digit
    current_frame = (current_frame + 1) % digit_pins.size();

    auto raw_digit = buffer_[current_frame];
    for (const auto &segment_pin : segment_pins) {
      const bool segment_on = raw_digit & 0x01;
      raw_digit >>= 1;
      bit_count += segment_on ? 1 : 0;
      segment_pin->digital_write(segment_level(segment_on));
    }

    if (auto digit_pin = digit_pins[current_frame])
      digit_pin->digital_write(digit_level(true));
  } else {
    segment_pins[current_frame]->digital_write(segment_level(false));

    // switch to next segment
    current_frame = (current_frame + 1) % segment_pins.size();

    auto const *raw_digit = buffer_;
    for (const auto &digit_pin : digit_pins) {
      const bool digit_on = (*raw_digit) & (0x01 << current_frame);
      bit_count += digit_on ? 1 : 0;
      if (digit_pin)
        digit_pin->digital_write(digit_level(digit_on));
      raw_digit++;
    }
    segment_pins[current_frame]->digital_write(segment_level(true));
  }

  if (colon_pin)
    colon_pin->digital_write(segment_level(colon_on && current_frame == 0));

  if (degree_pin)
    degree_pin->digital_write(segment_level(degree_on && current_frame == 0));

  cycles_to_skip = intensity_delay + (compensate_brightness ? bit_count : 0);
}

void LcdDigitsComponent::enable_timer() {
  ESP_LOGV(TAG, "enabling timer");
  s_timer_enable();
}

void LcdDigitsComponent::set_segment_pins(std::vector<GPIOPin *> segment_pins) {
  ESP_LOGV(TAG, "Setting up segment pins");
  InterruptLock lock;
  interrupt_data_.segment_pins = std::move(segment_pins);
  interrupt_data_.segment_pins.resize(
      std::min(size_t(8), interrupt_data_.segment_pins.size()));
}

void LcdDigitsComponent::set_degree_pin(GPIOPin *arg) {
  ESP_LOGV(TAG, "Setting up degree pin");
  InterruptLock lock;
  interrupt_data_.degree_pin = arg;
}

void LcdDigitsComponent::set_colon_pin(GPIOPin *arg) {
  ESP_LOGV(TAG, "Setting up colon");
  InterruptLock lock;
  interrupt_data_.colon_pin = arg;
}

void LcdDigitsComponent::set_digit_pins(std::vector<GPIOPin *> digit_pins) {
  ESP_LOGV(TAG, "Setting up digit pins");
  InterruptLock lock;
  interrupt_data_.digit_pins = std::move(digit_pins);
  interrupt_data_.digit_pins.resize(
      std::min(size_t(max_digit_count), interrupt_data_.digit_pins.size()));
}

void LcdDigitsComponent::set_writer(lcd_digits_writer_t &&writer) {
  ESP_LOGV(TAG, "Setting up writer");
  writer_ = std::move(writer);
}
void LcdDigitsComponent::set_display_type(DisplayType arg) {
  ESP_LOGV(TAG, "set display type: %d", arg);
  InterruptLock lock;
  interrupt_data_.display_type = arg;
}
void LcdDigitsComponent::set_compensate_brightness(bool arg) {
  ESP_LOGV(TAG, "Setting up brightness to %d", arg);
  InterruptLock lock;
  interrupt_data_.compensate_brightness = arg;
}
void LcdDigitsComponent::set_iterate_digits(bool arg) {
  ESP_LOGV(TAG, "Setting up iterate digits to %d", arg);
  InterruptLock lock;
  interrupt_data_.iterate_digits = arg;
}
void LcdDigitsComponent::set_intensity(uint8_t arg) {
  ESP_LOGV(TAG, "Setting up intensity to %d", arg);
  InterruptLock lock;
  interrupt_data_.intensity_delay = (15 - arg);
};

void LcdDigitsComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "LCD Digits:");
  for (auto *pin : interrupt_data_.digit_pins) {
    if (pin) {
      LOG_PIN("  Digit Pin: ", pin);
    }
  }
  for (auto *pin : interrupt_data_.segment_pins) {
    LOG_PIN("  Segment Pin: ", pin);
  }
  LOG_PIN("  Semicolon Pin: ", interrupt_data_.colon_pin);
  LOG_PIN("  Semicolon Pin: ", interrupt_data_.degree_pin);
}

void LcdDigitsComponent::update() {
  if (writer_.has_value())
    (*writer_)(*this);
  ESP_LOGV(TAG, "Updating interrupt data");
  InterruptLock lock;
  static_cast<LcdData &>(interrupt_data_) = display_data_;
}

uint8_t LcdDigitsComponent::print(const char *str) {
  ESP_LOGV(TAG, "Printing %s", str);
  return print(0, str);
}

void LcdDigitsComponent::set_degree_on(bool arg) {
  ESP_LOGV(TAG, "Setting degree on %d", arg);
  display_data_.degree_on = arg;
}
void LcdDigitsComponent::set_colon_on(bool arg) {
  ESP_LOGV(TAG, "Setting colon on %d", arg);
  display_data_.colon_on = arg;
}

void LcdDigitsComponent::setup() {
  ESP_LOGV(TAG, "Setting up");
  g_interrupt_data = &interrupt_data_;

  const auto setup_output_pin = [&](GPIOPin *pin, bool initial) {
    pin->setup();
    pin->pin_mode(esphome::gpio::Flags::FLAG_OUTPUT);
    pin->digital_write(initial);
  };

  for (const auto &pin : interrupt_data_.digit_pins) {
    if (pin) {
      setup_output_pin(pin, true);
    }
  }

  for (const auto &pin : interrupt_data_.segment_pins)
    setup_output_pin(pin, false);

  if (interrupt_data_.colon_pin)
    setup_output_pin(interrupt_data_.colon_pin, false);

  if (interrupt_data_.degree_pin)
    setup_output_pin(interrupt_data_.degree_pin, false);

  ESP_LOGV(TAG, "Timer1 Setup");
  s_timer_setup();
  ESP_LOGV(TAG, "Timer1 Setup done");
}

uint8_t LcdDigitsComponent::print(uint8_t start_pos, const char *in_str) {
  ESP_LOGV(TAG, "Printing pos: %d %s", start_pos, in_str);
  const auto &digits_count = interrupt_data_.digit_pins.size();
  if (start_pos >= digits_count)
    return 0;

  uint8_t pos = start_pos;
  auto &&buffer = display_data_.buffer_;
  for (auto str = in_str; *str != '\0'; str++) {
    uint8_t data = UNKNOWN_CHAR;
    if (*str >= ' ' && *str <= '~')
      data = ASCII_TO_RAW[*str - ' '];

    if (data == UNKNOWN_CHAR) {
      ESP_LOGV(TAG,
               "Encountered character '%c' with no representation while "
               "translating string!",
               *str);
    }

    if (*str == '.') {
      if (pos != start_pos)
        pos--;
      buffer[digits_count - pos - 1] |= 0b10000000;
    } else {
      if (pos >= digits_count) {
        ESP_LOGE(TAG, "String '%s' is too long for the display!", in_str);
        break;
      }
      buffer[digits_count - pos - 1] = data;
    }
    pos++;
  }
  
  ESP_LOGV(TAG, "Done %s", in_str);
  return pos - start_pos;
}

uint8_t LcdDigitsComponent::printf(uint8_t pos, const char *format, ...) {
  ESP_LOGV(TAG, "printf pos: %d %s", pos, format);
  va_list arg;
  va_start(arg, format);
  char buffer[8];
  int ret = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);
  if (ret > 0)
    return print(pos, buffer);
  return 0;
}

void LcdDigitsComponent::set_raw(uint64_t raw) {
  ESP_LOGV(TAG, "Setting raw %ud", raw);
  for (auto &d : display_data_.buffer_) {
    d = raw & 0xff;
    raw >>= 8;
  }
}

} // namespace lcd_digits

} // namespace esphome
