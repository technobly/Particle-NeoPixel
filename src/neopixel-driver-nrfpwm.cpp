#include "neopixel.h"

#ifdef USE_DRIVER_NRFPWM

/* Implementation of the Neopixel waveform generation for the nRF52840 using the PWM peripheral and DMA
 */

#include "neopixel-driver-blocking.h"

NeoPixel_Driver_NrfPwm* NeoPixel_Driver_NrfPwm::instance = NULL;

#include "nrf_gpio.h"
#include "pinmap_impl.h"

#define NRFX_PWM_DUTY_INVERTED 0x8000

// 800 kHz with 16 MHz clock
#define PERIOD_800KHZ 20
// 400 kHz with 16 MHz clock
#define PERIOD_400KHZ 40

#define BIT0_800KHZ (NRFX_PWM_DUTY_INVERTED | 5)
#define BIT1_800KHZ (NRFX_PWM_DUTY_INVERTED | 14)

#define BIT0_400KHZ (NRFX_PWM_DUTY_INVERTED | 10)
#define BIT1_400KHZ (NRFX_PWM_DUTY_INVERTED | 28)

uint8_t nrf_pins[NRF_PWM_CHANNEL_COUNT] = {
  NRFX_PWM_PIN_NOT_USED,
  NRFX_PWM_PIN_NOT_USED,
  NRFX_PWM_PIN_NOT_USED,
  NRFX_PWM_PIN_NOT_USED,
};

const nrfx_pwm_t pwms[] = {
    NRFX_PWM_INSTANCE(0),
    NRFX_PWM_INSTANCE(1),
    NRFX_PWM_INSTANCE(2),
    NRFX_PWM_INSTANCE(3)
};

NeoPixel_Driver_NrfPwm::NeoPixel_Driver_NrfPwm(uint8_t p, uint8_t t) :
  type(t), endTime(0), active(false), bits(NULL)
{
  setPin(p);
}

NeoPixel_Driver_NrfPwm::~NeoPixel_Driver_NrfPwm() {
  updateLength(0);
}

void NeoPixel_Driver_NrfPwm::updateLength(uint16_t numBytes) {
  waitForInactive();
  if (bits) {
    free(bits);
    bits = NULL;
  }

  if (numBytes == 0) {
    return;
  }

  bits = (uint16_t*)malloc(numBytes * 8);
  if (bits) {
    memset(bits, 0, numBytes * 8);
  }
}

void NeoPixel_Driver_NrfPwm::begin() {
  // Set up hardware PWM peripheral to output Neopixel waveform
  NRF5x_Pin_Info*  PIN_MAP = HAL_Pin_Map();
  uint8_t          pwm_num = PIN_MAP[pin].pwm_instance;
  uint8_t          pwm_channel = PIN_MAP[pin].pwm_channel;

  nrf_pwm_clk_t pwm_clock = NRF_PWM_CLK_16MHz;
  uint16_t period_hwu = (freqkHz() == 800) ? PERIOD_800KHZ : PERIOD_400KHZ;
  nrf_pins[pwm_channel] = NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);

  nrfx_pwm_config_t const config = {
      .output_pins =
      {
          nrf_pins[0],   // channel 0
          nrf_pins[1],   // channel 1
          nrf_pins[2],   // channel 2
          nrf_pins[3],   // channel 3
      },
      .irq_priority = APP_IRQ_PRIORITY_LOWEST,
      .base_clock   = pwm_clock,
      .count_mode   = NRF_PWM_MODE_UP,
      .top_value    = period_hwu,
      .load_mode    = NRF_PWM_LOAD_COMMON,
      .step_mode    = NRF_PWM_STEP_AUTO
  };

  nrfx_pwm_init(&pwms[pwm_num], &config, stopHandler);
  HAL_Set_Pin_Function(pin, PF_PWM);
}

void NeoPixel_Driver_NrfPwm::end() {
  waitForInactive();
  // TODO
}

// Set the output pin number
void NeoPixel_Driver_NrfPwm::setPin(uint8_t p) {
  pin = p;
}

void NeoPixel_Driver_NrfPwm::show(uint8_t* pixels, uint16_t numBytes) {
  if(!pixels || !bits) return;

  waitForInactive();

  pixelsToBits(pixels, numBytes);

  // Data latch = 24 or 50 microsecond pause in the output stream.  Rather than
  // put a delay at the end of the function, the ending time is noted and
  // the function will simply hold off (if needed) on issuing the
  // subsequent round of data until the latch time has elapsed.  This
  // allows the mainline code to start generating the next frame of data
  // rather than stalling for the latch.
  uint32_t wait_time; // wait time in microseconds.
  switch(type) {
    case TM1803: { // TM1803 = 24us reset pulse
        wait_time = 24L;
      } break;
    case SK6812RGBW: { // SK6812RGBW = 80us reset pulse
        wait_time = 80L;
      } break;
    case TM1829: { // TM1829 = 500us reset pulse
        wait_time = 500L;
      } break;
    case WS2812B: // WS2812, WS2812B & WS2813 = 300us reset pulse
    case WS2812B2: {
        wait_time = 300L;
      } break;
    case WS2811: // WS2811, WS2812B_FAST & WS2812B2_FAST = 50us reset pulse
    case WS2812B_FAST:
    case WS2812B2_FAST:
    default: {   // default = 50us reset pulse
        wait_time = 50L;
      } break;
  }
  while((micros() - endTime) < wait_time);

  active = true;
  instance = this;

  NRF5x_Pin_Info*  PIN_MAP = HAL_Pin_Map();
  uint8_t          pwm_num = PIN_MAP[pin].pwm_instance;

  nrf_pwm_sequence_t const seq = {
    .values     = { .p_common = bits },
    .length     = (uint16_t)(8 * numBytes),
    .repeats    = 0,
    .end_delay  = 0
  };

  nrfx_pwm_simple_playback(&pwms[pwm_num], &seq, 1, NRFX_PWM_FLAG_STOP);
}

void NeoPixel_Driver_NrfPwm::waveformDone() {
  active = false;
  endTime = micros();
}

void NeoPixel_Driver_NrfPwm::stopHandler(nrfx_pwm_evt_type_t eventType) {
  if (eventType == NRFX_PWM_EVT_STOPPED) {
    if (NeoPixel_Driver_NrfPwm::instance) {
      NeoPixel_Driver_NrfPwm::instance->waveformDone();
    }
  }
}

void NeoPixel_Driver_NrfPwm::pixelsToBits(uint8_t* pixels, uint16_t numBytes) {
  uint16_t bit0 = (freqkHz() == 800) ? BIT0_800KHZ : BIT0_400KHZ;
  uint16_t bit1 = (freqkHz() == 800) ? BIT1_800KHZ : BIT1_400KHZ;
  uint16_t* bitPtr = &bits[0];
  uint8_t* pixelPtr = &pixels[0];

  for (uint16_t i = 0; i < numBytes; i++) {
    uint8_t mask = 0x80;

    while (mask) {
      if (*pixelPtr & mask) {
        *bitPtr = bit1;
      } else {
        *bitPtr = bit0;
      }
      mask >>= 1;
      bitPtr++;
    }
    pixelPtr++;
  }
}

void NeoPixel_Driver_NrfPwm::waitForInactive() {
    while (active) delay(1);
}

uint16_t NeoPixel_Driver_NrfPwm::freqkHz() const {
  return (type == WS2811 || type == TM1803) ? 400 : 800;
}

#endif // USE_DRIVER_NRFPWM