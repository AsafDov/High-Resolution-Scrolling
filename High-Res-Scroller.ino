#include "Adafruit_TinyUSB.h"
#include "AS5600.h" // Add an AS5600 library
#include "Wire.h"

AS5600 as5600(&Wire1);
int16_t lastPosition = 0;
int16_t threshold = 0;
int16_t sensitivity = 8;
int16_t v=10;
int dir = -1;
const int pinButton = 0; 
const bool activeState = LOW; 
static int32_t accum = 0;

#define REPORT_ID_MOUSE      1
#define REPORT_ID_MULTIPLIER 2

uint8_t const desc_hid_report[] = {
  0x05, 0x01, 0x09, 0x02, 0xA1, 0x01,
    0x85, REPORT_ID_MOUSE,
    0x09, 0x01, 0xA1, 0x00,
      // Botones
      0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02,
      0x95, 0x01, 0x75, 0x05, 0x81, 0x03,
      // X, Y (16 bits)
      0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x16, 0x01, 0x80, 0x26, 0xFF, 0x7F, 0x75, 0x10, 0x95, 0x02, 0x81, 0x06,
      
      // RUEDA CON ALTA RESOLUCIÓN
      0x05, 0x01, 0x09, 0x38,
      0x15, 0x81, // Logical Minimum (-127)
      0x25, 0x7F, // Logical Maximum (127)
      0x75, 0x08, // Report Size (8 bits) - Suficiente para scroll suave
      0x95, 0x01, // Report Count (1)
      0x81, 0x06, // Input (Data,Var,Rel)

      // Multiplicador (Feature)
      0x05, 0x01, 0x09, 0x48,
      0x85, REPORT_ID_MULTIPLIER,
      0x15, 0x00, 0x25, 0x01, 0x35, 0x01, 0x45, 0x0C, 0x75, 0x02, 0x95, 0x01, 0xB1, 0x02,
      0x75, 0x06, 0xB1, 0x03, // Padding
    0xC0,
  0xC0
};

// Estructura actualizada a 8 bits para la rueda
typedef struct __attribute__((packed)) {
  uint8_t buttons;
  int16_t x;
  int16_t y;
  int8_t  wheel; // Cambiado a 8 bits
} hi_res_mouse_report_t;

Adafruit_USBD_HID usb_hid;

uint16_t my_get_report_callback (uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  if (report_type == HID_REPORT_TYPE_FEATURE && report_id == REPORT_ID_MULTIPLIER) {
    if (reqlen >= 1) { buffer[0] = 1; return 1; }
  }
  return 0;
}

void my_set_report_callback (uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  (void) report_id; (void) report_type; (void) buffer; (void) bufsize;
}

void setup() {
  Wire1.begin();
  as5600.begin(); // Initialize I2C and sensor
  lastPosition = as5600.readAngle(); // Get initial position

  pinMode(pinButton, INPUT);
  usb_hid.setPollInterval(1);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.setReportCallback(my_get_report_callback, my_set_report_callback);
  usb_hid.begin();

  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(1000); 
    TinyUSBDevice.attach();
  }
}

void loop() {
  #ifdef TINYUSB_NEED_POLLING_TASK
  TinyUSBDevice.task();
  #endif

  // if (!usb_hid.ready()) return;

// 1. Get current position
  int16_t currentPosition = as5600.readAngle();

  // 2. Calculate Delta (Difference)
  int16_t delta = currentPosition - lastPosition;

  // 3. Handle the 0-4096 wrap-around (assuming 4096 range)
  if (delta > 2048) delta -= 4096;
  else if (delta < -2048) delta += 4096;
  lastPosition = currentPosition;
  accum += delta;

  int8_t scrollValue = 0;
  while (accum >= sensitivity) { scrollValue++; accum -= sensitivity; }
  while (accum <= -sensitivity) { scrollValue--; accum += sensitivity; }

  if (scrollValue != 0) {
    if (usb_hid.ready()) {
      hi_res_mouse_report_t report = {0};
      report.wheel = dir*scrollValue; 
    
          // If send fails, PUT IT BACK so it retries next loop
      if (!usb_hid.sendReport(REPORT_ID_MOUSE, &report, sizeof(report))) {
        accum += (int32_t)scrollValue * sensitivity;
      }
    } else {
      // Not ready: keep it for later (do nothing)
      accum += (int32_t)scrollValue * sensitivity;
    }
    // IMPORTANT: Only update lastPosition by the amount we actually scrolled.
    // This keeps the "leftover" degrees for the next loop.
    // lastPosition += (scrollValue * sensitivity); 
    // if (lastPosition >= 4096) lastPosition -= 4096;
    // else if (lastPosition < 0) lastPosition += 4096;
}

  delay(1);
}
