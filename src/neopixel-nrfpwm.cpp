/*-------------------------------------------------------------------------
  Driver to generate the Neopixel waveform on the nRF52840 using the PWM peripheral and DMA.

  On the Argon, Boron and Xenon, only these pins can be used for Neopixel:
  - D2, D3, A4, A5
  - D4, D6, D7, D8
  - A0, A1, A2, A3
  
  In addition, only one pin per group can be used at a time. So it's OK to have one Adafruit_NeoPixel
  instance on pin D2 and another one on pin A2, but it's not possible to have one on pin A0 and another
  one on pin A1.

  For each Neopixel LED the driver outputs 3 or 4 bytes of data (1 byte per red/green/blue
  color + optional white) so it outputs 24 or 32 bits per LED. For each bit, the waveform
  lasts 1.25 microseconds. If the waveform is high for 300 nanoseconds, then low for 950 ns
  it is a 0 bit. If it is high for 850 ns and low for 400 ns, it is a 1 bit.

  This driver uses the ability of the nRF microcontroller to play back an array of PWM
  duty cycles without CPU intervention. It converts the pixel array into an array of PWM duty
  cycles so it uses 16 times more memory than the bit bang driver (since each pixel has 8 bits
  per pixel and the duty cycle register has 2 bytes).

  For the original idea of using PWM and DMA for driving Neopixel / WS2812 LEDs, see the
  OctoWS2811 library by Paul Stoffregen
  https://www.pjrc.com/teensy/td_libs_OctoWS2811.html#tech

  --------------------------------------------------------------------*/

/* ======================= neopixel-nrfpwm.cpp ======================= */
#include "neopixel.h"

#ifdef USE_DRIVER_NRFPWM

#include "neopixel-nrfpwm.h"
#include "nrf_gpio.h"
#include "pinmap_impl.h"

NeoPixel_NrfPwm* NeoPixel_NrfPwm::instances[NRF_PWM_CHANNEL_COUNT] = {
  NULL,
  NULL,
  NULL,
  NULL
};

const nrfx_pwm_t NeoPixel_NrfPwm::pwms[NRF5X_PWM_COUNT] = {
    NRFX_PWM_INSTANCE(0),
    NRFX_PWM_INSTANCE(1),
    NRFX_PWM_INSTANCE(2),
    NRFX_PWM_INSTANCE(3)
};

#define NRFX_PWM_DUTY_INVERTED 0x8000

// 800 kHz with 16 MHz clock
#define PERIOD_800KHZ 20
// 400 kHz with 16 MHz clock
#define PERIOD_400KHZ 40

#define BIT0_800KHZ (NRFX_PWM_DUTY_INVERTED | 5)
#define BIT1_800KHZ (NRFX_PWM_DUTY_INVERTED | 14)

#define BIT0_400KHZ (NRFX_PWM_DUTY_INVERTED | 10)
#define BIT1_400KHZ (NRFX_PWM_DUTY_INVERTED | 28)

NeoPixel_NrfPwm::NeoPixel_NrfPwm(uint8_t p, uint8_t t) :
  type(t), endTime(0), active(false), bits(NULL)
{
  setPin(p);
}

NeoPixel_NrfPwm::~NeoPixel_NrfPwm() {
  updateLength(0);
}

void NeoPixel_NrfPwm::updateLength(uint16_t numBytes) {
  waitForInactive();
  if (bits) {
    free(bits);
    bits = NULL;
  }

  if (numBytes == 0) {
    return;
  }

  size_t bitsSize = numBytes * 8 * sizeof(uint16_t);
  bits = (uint16_t*)malloc(bitsSize);
  if (bits) {
    memset(bits, 0, bitsSize);
  }
}

void NeoPixel_NrfPwm::begin() {
  // Set up hardware PWM peripheral to output Neopixel waveform
  NRF5x_Pin_Info*  PIN_MAP = HAL_Pin_Map();
  uint8_t          pwm_num = PIN_MAP[pin].pwm_instance;
  uint8_t          pwm_channel = PIN_MAP[pin].pwm_channel;

  if (pwm_num == PWM_INSTANCE_NONE) {
    DEBUG("Pin not supported for Neopixel");
    return;
  }

  nrf_pwm_clk_t pwm_clock = NRF_PWM_CLK_16MHz;
  uint16_t period_hwu = (freqkHz() == 800) ? PERIOD_800KHZ : PERIOD_400KHZ;

  uint8_t nrf_pins[NRF_PWM_CHANNEL_COUNT] = {
    NRFX_PWM_PIN_NOT_USED,
    NRFX_PWM_PIN_NOT_USED,
    NRFX_PWM_PIN_NOT_USED,
    NRFX_PWM_PIN_NOT_USED,
  };
  nrf_pins[pwm_channel] = NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);

  // ensure pin is in default mode
  nrf_gpio_cfg_default(nrf_pins[pwm_channel]);

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

  nrfx_pwm_handler_t stopHandlers[] = {
    stopHandler0,
    stopHandler1,
    stopHandler2,
    stopHandler3,    
  };

  nrfx_pwm_init(&pwms[pwm_num], &config, stopHandlers[pwm_num]);
}

void NeoPixel_NrfPwm::end() {
  waitForInactive();

  NRF5x_Pin_Info*  PIN_MAP = HAL_Pin_Map();
  uint8_t          pwm_num = PIN_MAP[pin].pwm_instance;

  if (pwm_num == PWM_INSTANCE_NONE) {
    DEBUG("Pin not supported for Neopixel");
    return;
  }

  nrfx_pwm_uninit(&pwms[pwm_num]);
}

// Set the output pin number
void NeoPixel_NrfPwm::setPin(uint8_t p) {
  pin = p;
}

void NeoPixel_NrfPwm::show(uint8_t* pixels, uint16_t numBytes, uint32_t waitTime) {
  if(!pixels || !bits) return;

  waitForInactive();

  pixelsToBits(pixels, numBytes);

  while((micros() - endTime) < waitTime);

  active = true;

  NRF5x_Pin_Info*  PIN_MAP = HAL_Pin_Map();
  uint8_t          pwm_num = PIN_MAP[pin].pwm_instance;

  if (pwm_num == PWM_INSTANCE_NONE) {
    return;
  }

  instances[pwm_num] = this;

  nrf_pwm_sequence_t const seq = {
    .values     = { .p_common = bits },
    .length     = (uint16_t)(8 * numBytes),
    .repeats    = 0,
    .end_delay  = 0
  };

  nrfx_pwm_simple_playback(&pwms[pwm_num], &seq, 1, NRFX_PWM_FLAG_STOP);
}

void NeoPixel_NrfPwm::waveformDone() {
  active = false;
  endTime = micros();
}

void NeoPixel_NrfPwm::stopHandler0(nrfx_pwm_evt_type_t eventType) {
  if (eventType == NRFX_PWM_EVT_STOPPED) {
    if (NeoPixel_NrfPwm::instances[0]) {
      NeoPixel_NrfPwm::instances[0]->waveformDone();
    }
  }
}

void NeoPixel_NrfPwm::stopHandler1(nrfx_pwm_evt_type_t eventType) {
  if (eventType == NRFX_PWM_EVT_STOPPED) {
    if (NeoPixel_NrfPwm::instances[1]) {
      NeoPixel_NrfPwm::instances[1]->waveformDone();
    }
  }
}

void NeoPixel_NrfPwm::stopHandler2(nrfx_pwm_evt_type_t eventType) {
  if (eventType == NRFX_PWM_EVT_STOPPED) {
    if (NeoPixel_NrfPwm::instances[2]) {
      NeoPixel_NrfPwm::instances[2]->waveformDone();
    }
  }
}

void NeoPixel_NrfPwm::stopHandler3(nrfx_pwm_evt_type_t eventType) {
  if (eventType == NRFX_PWM_EVT_STOPPED) {
    if (NeoPixel_NrfPwm::instances[3]) {
      NeoPixel_NrfPwm::instances[3]->waveformDone();
    }
  }
}

void NeoPixel_NrfPwm::pixelsToBits(uint8_t* pixels, uint16_t numBytes) {
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

void NeoPixel_NrfPwm::waitForInactive() {
    while (active) delay(1);
}

uint16_t NeoPixel_NrfPwm::freqkHz() const {
  return (type == WS2811 || type == TM1803) ? 400 : 800;
}

#endif // USE_DRIVER_NRFPWM
