// ----------------------------------------------------------------------------------
// Features
// ----------------------------------------------------------------------------------
#define CHECK_WATER_METER_AVAILABLE
#define LEAK_CHECK

// ----------------------------------------------------------------------------------
// PINs
// ----------------------------------------------------------------------------------
#define BLUETOOTH_ENABLE_PIN 2
// unused 3

#define VALVE1_PIN 4
#define VALVE2_PIN 5
#define VALVE3_PIN 6
#define VALVE4_PIN 7

#define RTC_INT_PIN 8
// unused 9
#define START_MANUAL_PIN 9
#define START_AUTOMATIC_PIN 10
#define MODE_PIN 11

#define WATER_METER_PIN 12

#define DEEP_SLEEP_SCHEDULER_AWAKE_INDICATION_PIN 13

#define COLOR_LED_GREEN_PIN A0
#define COLOR_LED_RED_PIN A1
#define COLOR_LED_BLUE_PIN A2

// uncomment if your color LED has a common + PIN
// comment out if your color LED has a common - PIN
//#define COLOR_LED_INVERTED

// unused A3, A4, A5

// potential PinChangePins on Leonardo: 8, 9, 10, 11

// ----------------------------------------------------------------------------------
// EEPROM
// ----------------------------------------------------------------------------------
#define EEPROM_VERSION 1
#define EEPROM_LENGTH_TO_USE 128
#define EEPROM_INDEX_COUNT 6

#define EEPROM_INDEX_WATCHDOG_RESET_COUNT 0
#define EEPROM_INDEX_SERIAL_SLEEP_TIMEOUT_MS 1
#define EEPROM_INDEX_ZONE1 2
#define EEPROM_INDEX_ZONE2 3
#define EEPROM_INDEX_ZONE3 4
#define EEPROM_INDEX_WATER_METER_THRESHOLD 5

