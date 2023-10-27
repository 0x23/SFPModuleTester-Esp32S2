//*****************************************************************************

#include "esp_system.h"
#include <Wire.h>
#include "driver/dedic_gpio.h"

//*****************************************************************************

#define I2C_ADDRESS_MODDEF 0x50 // Module definition 
#define I2C_ADDRESS_DOM 0x51    // Digital Optical Monitoring

/**
 * Pin Assignment (for Wemos-S2 mini - ESP32-S2)
 *
 * WARNING: Do not use the 3.3V supply from the MCU board if it cant supply at least 500mA current (Wemos S2 mini can not!). 
 *          Also use Caps to filter 3.3V supply for SFP module.
 * WARNING: Tx+/Tx- pins are a differential pair. Most SFP modules require a differential voltage of 0.2V-0.5V on these pins (see Datasheet of your SFP module). 
 *          For low speeds (up to about 50kHz) you can use a voltage divider. The correct way is to use a single ended to differntial converter. 
 *          Do not use 3.3V from IO pins directly !
 *
 * | Function      | SFP Module Pin| Esp32-S2 Pin |
 * | ------------- | ------------- | ------------ |
 * | Tx Disable    |   3           |   2          |
 * | MOD-DEF (SDA) |   4           |   7	        |
 * | MOD-DEF (SCL) |   5           |   6          |
 * | Signal Loss   |   8           |   3          |
 * | Tx+           | 18 w. divider |   16         |
 * | Tx-           | 19 w. divider |   17         |
 * |               |               |              |
 * | VeeT          |   1/20/17     |   GND        |
 * | VeeR          |   9/10/11/14  |   GND        |
 * | VccT          |   16          |   (+3.3V)    |
 * | VccR          |   17          |   (+3.3V)    |
 */ 

const int PIN_IC2_SDA = 7;
const int PIN_IC2_SCL = 6;
const int PIN_TX_DISABLE = 2;
const int PIN_SIGNAL_LOSS = 3;
const int PIN_TX1 = 16;
const int PIN_TX2 = 17;

const int PIN_LED = 15;

//*** GLOBALS *****************************************************************

dedic_gpio_bundle_handle_t tx_pin_bundle;
volatile int next_tx_output = 1;

//*** FUNCTIONS ***************************************************************

void blink(int count, int puls_duration_ms) {
  for(int i=0; i<count; i++) {
      digitalWrite(PIN_LED, true);
      delay(puls_duration_ms);
      digitalWrite(PIN_LED, false);
      delay(puls_duration_ms);
  }
}

/**
 * creates a gpio pin bundle to simultanously write pins in a single cpu clock cycle
 * @param pin_numbers list of pin numbers, the given pins will be configured as output
 * @param pin_count  length of the pin list
 * @return the created gpio bundle. destroy with dedic_gpio_del_bundle()
 */
dedic_gpio_bundle_handle_t create_gpio_bundle_out(int* pin_numbers, int pin_count) {
  for(int i=0; i<pin_count; i++)
      pinMode(pin_numbers[i], OUTPUT);

  dedic_gpio_bundle_handle_t bundle = NULL;
  dedic_gpio_bundle_config_t bundle_config = {
      .gpio_array = pin_numbers,
      .array_size = pin_count,
      .flags = {
          .out_en = 1,
      },
  };
  ESP_ERROR_CHECK(dedic_gpio_new_bundle(&bundle_config, &bundle));

  return bundle;
}

/**
 * reads a mudule register over I2C mod-def interface. 
 */
uint8_t read_module_register(uint8_t device_adress, uint8_t register_address) {
  // Request data from the device
  Wire.beginTransmission(device_adress);
  Wire.write(register_address);
  Wire.endTransmission();

  // Read data from the device
  int bytesRead = Wire.requestFrom(device_adress, (uint8_t)1);
  if (bytesRead != 1)
    return -1;

  int32_t data = Wire.read();
  return data;
}

/**
 * interrupt for toggeling tx pins
 */
void IRAM_ATTR tx_pin_timer() {
  dedic_gpio_bundle_write(tx_pin_bundle, 255, next_tx_output);
  next_tx_output = 3-next_tx_output;  // toggle between 1 and 2
}

/**
 * setup gpio bundle and toggle timer for Tx testing
 */
void setup_tx() {
  // configure pins for differential output to TX 
  int pin_list[] = {PIN_TX1, PIN_TX2};
  tx_pin_bundle = create_gpio_bundle_out(pin_list, 2);

  // initialize the timer and attach interrupt
  hw_timer_t* timer = timerBegin(0, 80, true); // Timer 0, divider 80 => 1µs
  timerAttachInterrupt(timer, &tx_pin_timer, true);
  timerAlarmWrite(timer, 200/2, true);        // 5khz
  timerAlarmEnable(timer);

  // enable transmitter
  digitalWrite(PIN_TX_DISABLE, false);
}

/**
 * prints all 127 bytes of the module info to serial interface
 */
void print_mod_def_info(int i2c_address) {
  Serial.println("Reading MOD-DEF registers:");
  for(uint8_t i=0; i<128; i++) {
    int v = read_module_register(i2c_address, i);
    Serial.print("  ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(v, HEX);
  }
}

//*** MAIN ********************************************************************

void setup() {
  pinMode(PIN_IC2_SDA, INPUT_PULLUP);
  pinMode(PIN_IC2_SCL, INPUT_PULLUP);
  pinMode(PIN_SIGNAL_LOSS, INPUT_PULLUP);  
  pinMode(PIN_TX_DISABLE, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_TX_DISABLE, true);

  blink(3, 100);

  Wire.begin(PIN_IC2_SDA, PIN_IC2_SCL);
  Serial.begin(115200);
  delay(2000);

  print_mod_def_info(I2C_ADDRESS_MODDEF);

  setup_tx();
}

void loop() {
  // light on boar LED whe signal loss is false
  delay(10);
  bool signal_loss = digitalRead(PIN_SIGNAL_LOSS);
  digitalWrite(PIN_LED, signal_loss==false);
}
