#include "BleKeyboard.h"

#if defined(USE_NIMBLE)
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLEHIDDevice.h>
#else
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#endif // USE_NIMBLE
#include "HIDTypes.h"
#include <driver/adc.h>
#include "sdkconfig.h"


#if defined(CONFIG_ARDUHAL_ESP_LOG)
  #include "esp32-hal-log.h"
  #define LOG_TAG ""
#else
  #include "esp_log.h"
  static const char* LOG_TAG = "BLEDevice";
#endif


// Report IDs:
#define KEYBOARD_ID 0x01
#define MEDIA_KEYS_ID 0x02


static const uint8_t _hidReportDescriptor[] = {
  USAGE_PAGE(1),      0x01,          // USAGE_PAGE (Generic Desktop Ctrls)
  USAGE(1),           0x06,          // USAGE (Keyboard)
  COLLECTION(1),      0x01,          // COLLECTION (Application)
  // ------------------------------------------------- Keyboard
  REPORT_ID(1),       KEYBOARD_ID,   //   REPORT_ID (1)
  USAGE_PAGE(1),      0x07,          //   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0xE0,          //   USAGE_MINIMUM (0xE0)
  USAGE_MAXIMUM(1),   0xE7,          //   USAGE_MAXIMUM (0xE7)
  LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,          //   Logical Maximum (1)
  REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  REPORT_COUNT(1),    0x08,          //   REPORT_COUNT (8)
  HIDINPUT(1),        0x02,          //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  REPORT_COUNT(1),    0x01,          //   REPORT_COUNT (1) ; 1 byte (Reserved)
  REPORT_SIZE(1),     0x08,          //   REPORT_SIZE (8)
  HIDINPUT(1),        0x01,          //   INPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  REPORT_COUNT(1),    0x05,          //   REPORT_COUNT (5) ; 5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
  REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  USAGE_PAGE(1),      0x08,          //   USAGE_PAGE (LEDs)
  USAGE_MINIMUM(1),   0x01,          //   USAGE_MINIMUM (0x01) ; Num Lock
  USAGE_MAXIMUM(1),   0x05,          //   USAGE_MAXIMUM (0x05) ; Kana
  HIDOUTPUT(1),       0x02,          //   OUTPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),    0x01,          //   REPORT_COUNT (1) ; 3 bits (Padding)
  REPORT_SIZE(1),     0x03,          //   REPORT_SIZE (3)
  HIDOUTPUT(1),       0x01,          //   OUTPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),    0x06,          //   REPORT_COUNT (6) ; 6 bytes (Keys)
  REPORT_SIZE(1),     0x08,          //   REPORT_SIZE(8)
  LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM(0)
  LOGICAL_MAXIMUM(1), 0x65,          //   LOGICAL_MAXIMUM(0x65) ; 101 keys
  USAGE_PAGE(1),      0x07,          //   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0x00,          //   USAGE_MINIMUM (0)
  USAGE_MAXIMUM(1),   0x65,          //   USAGE_MAXIMUM (0x65)
  HIDINPUT(1),        0x00,          //   INPUT (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0),                 // END_COLLECTION
  // ------------------------------------------------- Media Keys
  USAGE_PAGE(1),      0x0C,          // USAGE_PAGE (Consumer)
  USAGE(1),           0x01,          // USAGE (Consumer Control)
  COLLECTION(1),      0x01,          // COLLECTION (Application)
  REPORT_ID(1),       MEDIA_KEYS_ID, //   REPORT_ID (3)
  USAGE_PAGE(1),      0x0C,          //   USAGE_PAGE (Consumer)
  LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,          //   LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  REPORT_COUNT(1),    0x10,          //   REPORT_COUNT (16)
  USAGE(1),           0xB5,          //   USAGE (Scan Next Track)     ; bit 0: 1
  USAGE(1),           0xB6,          //   USAGE (Scan Previous Track) ; bit 1: 2
  USAGE(1),           0xB7,          //   USAGE (Stop)                ; bit 2: 4
  USAGE(1),           0xCD,          //   USAGE (Play/Pause)          ; bit 3: 8
  USAGE(1),           0xE2,          //   USAGE (Mute)                ; bit 4: 16
  USAGE(1),           0xE9,          //   USAGE (Volume Increment)    ; bit 5: 32
  USAGE(1),           0xEA,          //   USAGE (Volume Decrement)    ; bit 6: 64
  USAGE(2),           0x23, 0x02,    //   Usage (WWW Home)            ; bit 7: 128
  USAGE(2),           0x94, 0x01,    //   Usage (My Computer) ; bit 0: 1
  USAGE(2),           0x92, 0x01,    //   Usage (Calculator)  ; bit 1: 2
  USAGE(2),           0x2A, 0x02,    //   Usage (WWW fav)     ; bit 2: 4
  USAGE(2),           0x21, 0x02,    //   Usage (WWW search)  ; bit 3: 8
  USAGE(2),           0x26, 0x02,    //   Usage (WWW stop)    ; bit 4: 16
  USAGE(2),           0x24, 0x02,    //   Usage (WWW back)    ; bit 5: 32
  USAGE(2),           0x83, 0x01,    //   Usage (Media sel)   ; bit 6: 64
  USAGE(2),           0x8A, 0x01,    //   Usage (Mail)        ; bit 7: 128
  HIDINPUT(1),        0x02,          //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0)                  // END_COLLECTION
};

BleKeyboard::BleKeyboard(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel) 
    : hid(0)
    , deviceName(std::string(deviceName).substr(0, 15))
    , deviceManufacturer(std::string(deviceManufacturer).substr(0,15))
    , batteryLevel(batteryLevel) {}              

void BleKeyboard::begin(void)
{
  BLEDevice::init(deviceName);
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);

  hid = new BLEHIDDevice(pServer);
  inputKeyboard = hid->inputReport(KEYBOARD_ID);  // <-- input REPORTID from report map
  outputKeyboard = hid->outputReport(KEYBOARD_ID);
  inputMediaKeys = hid->inputReport(MEDIA_KEYS_ID);

  outputKeyboard->setCallbacks(this);

  hid->manufacturer()->setValue(deviceManufacturer);

  hid->pnp(0x02, vid, pid, version);
  hid->hidInfo(0x00, 0x01);


#if defined(USE_NIMBLE)

  BLEDevice::setSecurityAuth(true, true, true);

#else

  BLESecurity* pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);

#endif // USE_NIMBLE

  hid->reportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
  hid->startServices();

  onStarted(pServer);

  advertising = pServer->getAdvertising();
  advertising->setAppearance(HID_KEYBOARD);
  advertising->addServiceUUID(hid->hidService()->getUUID());
  advertising->setScanResponse(false);
  advertising->start();
  hid->setBatteryLevel(batteryLevel);

  ESP_LOGD(LOG_TAG, "Advertising started!");
  initTranslate();
}

void BleKeyboard::end(void)
{
}

void BleKeyboard::initTranslate(){

translate["VK_a"] = 0x61;
translate["VK_b"] = 0x62;
translate["VK_c"] = 0x63;
translate["VK_d"] = 0x64;
translate["VK_e"] = 0x65;
translate["VK_f"] = 0x66;
translate["VK_g"] = 0x67;
translate["VK_h"] = 0x68;
translate["VK_i"] = 0x69;
translate["VK_j"] = 0x6a;
translate["VK_k"] = 0x6b;
translate["VK_l"] = 0x6c;
translate["VK_m"] = 0x6d;
translate["VK_n"] = 0x6e;
translate["VK_o"] = 0x6f;
translate["VK_p"] = 0x70;
translate["VK_q"] = 0x71;
translate["VK_r"] = 0x72;
translate["VK_s"] = 0x73;
translate["VK_t"] = 0x74;
translate["VK_u"] = 0x75;
translate["VK_v"] = 0x76;
translate["VK_w"] = 0x77;
translate["VK_x"] = 0x78;
translate["VK_y"] = 0x79;
translate["VK_z"] = 0x7a;

translate["VK_A"] = 0x61;
translate["VK_B"] = 0x62;
translate["VK_C"] = 0x63;
translate["VK_D"] = 0x64;
translate["VK_E"] = 0x65;
translate["VK_F"] = 0x66;
translate["VK_G"] = 0x67;
translate["VK_H"] = 0x68;
translate["VK_I"] = 0x69;
translate["VK_J"] = 0x6a;
translate["VK_K"] = 0x6b;
translate["VK_L"] = 0x6c;
translate["VK_M"] = 0x6d;
translate["VK_N"] = 0x6e;
translate["VK_O"] = 0x6f;
translate["VK_P"] = 0x70;
translate["VK_Q"] = 0x71;
translate["VK_R"] = 0x72;
translate["VK_S"] = 0x73;
translate["VK_T"] = 0x74;
translate["VK_U"] = 0x75;
translate["VK_V"] = 0x76;
translate["VK_W"] = 0x77;
translate["VK_X"] = 0x78;
translate["VK_Y"] = 0x79;
translate["VK_Z"] = 0x7a;

translate["VK_0"] = 0x30;
translate["VK_1"] = 0x31;
translate["VK_2"] = 0x32;
translate["VK_3"] = 0x33;
translate["VK_4"] = 0x34;
translate["VK_5"] = 0x35;
translate["VK_6"] = 0x36;
translate["VK_7"] = 0x37;
translate["VK_8"] = 0x38;
translate["VK_9"] = 0x39;

translate["VK_RIGHTPAREN"] = 0x29;
translate["VK_EXCLAIM"] = 0x21;
translate["VK_AT"] = 0x40;
translate["VK_HASH"] = 0x23;
translate["VK_DOLLAR"] = 0x24;
translate["VK_PERCENT"] = 0x25;
translate["VK_CARET"] = 0x5E;
translate["VK_AMPERSAND"] = 0x26;
translate["VK_KP_MULTIPLY"] = 0x2A;
translate["VK_LEFTPAREN"] = 0x28;

translate["VK_SPACE"] = 0x20;
translate["VK_GRAVEACCENT"] = 0x60;
translate["VK_TILDE"] = 0x7E;
translate["VK_MINUS"] = 0x2D;
translate["VK_UNDERSCORE"] = 0x5F;
translate["VK_EQUALS"] = 0x3D;
translate["VK_PLUS"] = 0x2B;
translate["VK_BACKSLASH"] = 0x5C;
translate["VK_VERTICALBAR"] = 0x7C;
translate["VK_LEFTBRACKET"] = 0x5B;
translate["VK_LEFTBRACE"] = 0x7B;
translate["VK_RIGHTBRACKET"] = 0x5D;
translate["VK_RIGHTBRACE"] = 0x7D;
translate["VK_SEMICOLON"] = 0x3B;
translate["VK_COLON"] = 0x3A;
translate["VK_QUOTE"] = 0x27;
translate["VK_QUOTEDBL"] = 0x22;
translate["VK_COMMA"] = 0x2C;
translate["VK_LESS"] = 0x3C;
translate["VK_PERIOD"] = 0x2E;
translate["VK_GREATER"] = 0x3E;
translate["VK_SLASH"] = 0x2F;
translate["VK_QUESTION"] = 0x3F;

translate["VK_LCTRL"] = 0x80;
translate["VK_LSHIFT"] = 0x81;
translate["VK_LALT"] = 0x82;
translate["VK_LGUI"] = 0x83;
translate["VK_RCTRL"] = 0x84;
translate["VK_RSHIFT"] = 0x85;
translate["VK_RALT"] = 0x86;
translate["VK_RGUI"] = 0x87;
translate["VK_SCROLLLOCK"] = 0xCF;
translate["VK_NUMLOCK"] = 0xDB;
translate["VK_APPLICATION"] = 0xED;

translate["VK_UP"] = 0xDA;
translate["VK_DOWN"] = 0xD9;
translate["VK_LEFT"] = 0xD8;
translate["VK_RIGHT"] = 0xD7;
translate["VK_BACKSPACE"] = 0xB2;
translate["VK_TAB"] = 0xB3;
translate["VK_RETURN"] = 0xB0;
translate["VK_ESCAPE"] = 0xB1;
translate["VK_INSERT"] = 0xD1;
translate["VK_PRINTSCREEN"] = 0xCE;
translate["VK_DELETE"] = 0xD4;
translate["VK_PAGEUP"] = 0xD3;
translate["VK_PAGEDOWN"] = 0xD6;
translate["VK_HOME"] = 0xD2;
translate["VK_END"] = 0xD5;
translate["VK_CAPSLOCK"] = 0xC1;
translate["VK_F1"] = 0xC2;
translate["VK_F2"] = 0xC3;
translate["VK_F3"] = 0xC4;
translate["VK_F4"] = 0xC5;
translate["VK_F5"] = 0xC6;
translate["VK_F6"] = 0xC7;
translate["VK_F7"] = 0xC8;
translate["VK_F8"] = 0xC9;
translate["VK_F9"] = 0xCA;
translate["VK_F10"] = 0xCB;
translate["VK_F11"] = 0xCC;
translate["VK_F12"] = 0xCD;
translate["VK_F13"] = 0xF0;
translate["VK_F14"] = 0xF1;
translate["VK_F15"] = 0xF2;
translate["VK_F16"] = 0xF3;
translate["VK_F17"] = 0xF4;
translate["VK_F18"] = 0xF5;
translate["VK_F19"] = 0xF6;
translate["VK_F20"] = 0xF7;
translate["VK_F21"] = 0xF8;
translate["VK_F22"] = 0xF9;
translate["VK_F23"] = 0xFA;
translate["VK_F24"] = 0xFB;

translate["VK_KP_MINUS"] = 0xDE;
translate["VK_KP_PLUS"] = 0xDF;
translate["VK_KP_MULTIPLY"] = 0xDD;
translate["VK_KP_DIVIDE"] = 0xDC;
translate["VK_KP_ENTER"] = 0xE0;

translate["VK_KP_0"] = 0xEA;
translate["VK_KP_1"] = 0xE1;
translate["VK_KP_2"] = 0xE2;
translate["VK_KP_3"] = 0xE3;
translate["VK_KP_4"] = 0xE4;
translate["VK_KP_5"] = 0xE5;
translate["VK_KP_6"] = 0xE6;
translate["VK_KP_7"] = 0xE7;
translate["VK_KP_8"] = 0xE8;
translate["VK_KP_9"] = 0xE9;
translate["VK_KP_PERIOD"] = 0xEB;

translate["VK_KP_INSERT"] = 0xEA;
translate["VK_KP_END"] = 0xE1;
translate["VK_KP_DOWN"] = 0xE2;
translate["VK_KP_PAGEDOWN"] = 0xE3;
translate["VK_KP_LEFT"] = 0xE4;
translate["VK_KP_CENTER"] = 0xE5;
translate["VK_KP_RIGHT"] = 0xE6;
translate["VK_KP_HOME"] = 0xE7;
translate["VK_KP_UP"] = 0xE8;
translate["VK_KP_PAGEUP"] = 0xE9;
translate["VK_KP_DELETE"] = 0xEB;
}

bool BleKeyboard::isConnected(void) {
  return this->connected;
}

void BleKeyboard::setBatteryLevel(uint8_t level) {
  this->batteryLevel = level;
  if (hid != 0)
    this->hid->setBatteryLevel(this->batteryLevel);
}

//must be called before begin in order to set the name
void BleKeyboard::setName(std::string deviceName) {
  this->deviceName = deviceName;
}

/**
 * @brief Sets the waiting time (in milliseconds) between multiple keystrokes in NimBLE mode.
 * 
 * @param ms Time in milliseconds
 */
void BleKeyboard::setDelay(uint32_t ms) {
  this->_delay_ms = ms;
}

void BleKeyboard::set_vendor_id(uint16_t vid) { 
	this->vid = vid; 
}

void BleKeyboard::set_product_id(uint16_t pid) { 
	this->pid = pid; 
}

void BleKeyboard::set_version(uint16_t version) { 
	this->version = version; 
}

void BleKeyboard::sendReport(KeyReport* keys)
{
  if (this->isConnected())
  {
    this->inputKeyboard->setValue((uint8_t*)keys, sizeof(KeyReport));
    this->inputKeyboard->notify();
#if defined(USE_NIMBLE)        
    // vTaskDelay(delayTicks);
    this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
  }	
}

void BleKeyboard::sendReport(MediaKeyReport* keys)
{
  if (this->isConnected())
  {
    this->inputMediaKeys->setValue((uint8_t*)keys, sizeof(MediaKeyReport));
    this->inputMediaKeys->notify();
#if defined(USE_NIMBLE)        
    //vTaskDelay(delayTicks);
    this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
  }	
}

extern
const uint8_t _asciimap[128] PROGMEM;

#define SHIFT 0x80
const uint8_t _asciimap[128] =
{
	0x00,             // NUL
	0x00,             // SOH
	0x00,             // STX
	0x00,             // ETX
	0x00,             // EOT
	0x00,             // ENQ
	0x00,             // ACK
	0x00,             // BEL
	0x2a,			// BS	Backspace
	0x2b,			// TAB	Tab
	0x28,			// LF	Enter
	0x00,             // VT
	0x00,             // FF
	0x00,             // CR
	0x00,             // SO
	0x00,             // SI
	0x00,             // DEL
	0x00,             // DC1
	0x00,             // DC2
	0x00,             // DC3
	0x00,             // DC4
	0x00,             // NAK
	0x00,             // SYN
	0x00,             // ETB
	0x00,             // CAN
	0x00,             // EM
	0x00,             // SUB
	0x00,             // ESC
	0x00,             // FS
	0x00,             // GS
	0x00,             // RS
	0x00,             // US

	0x2c,		   //  ' '
	0x1e|SHIFT,	   // !
	0x34|SHIFT,	   // "
	0x20|SHIFT,    // #
	0x21|SHIFT,    // $
	0x22|SHIFT,    // %
	0x24|SHIFT,    // &
	0x34,          // '
	0x26|SHIFT,    // (
	0x27|SHIFT,    // )
	0x25|SHIFT,    // *
	0x2e|SHIFT,    // +
	0x36,          // ,
	0x2d,          // -
	0x37,          // .
	0x38,          // /
	0x27,          // 0
	0x1e,          // 1
	0x1f,          // 2
	0x20,          // 3
	0x21,          // 4
	0x22,          // 5
	0x23,          // 6
	0x24,          // 7
	0x25,          // 8
	0x26,          // 9
	0x33|SHIFT,      // :
	0x33,          // ;
	0x36|SHIFT,      // <
	0x2e,          // =
	0x37|SHIFT,      // >
	0x38|SHIFT,      // ?
	0x1f|SHIFT,      // @
	0x04|SHIFT,      // A
	0x05|SHIFT,      // B
	0x06|SHIFT,      // C
	0x07|SHIFT,      // D
	0x08|SHIFT,      // E
	0x09|SHIFT,      // F
	0x0a|SHIFT,      // G
	0x0b|SHIFT,      // H
	0x0c|SHIFT,      // I
	0x0d|SHIFT,      // J
	0x0e|SHIFT,      // K
	0x0f|SHIFT,      // L
	0x10|SHIFT,      // M
	0x11|SHIFT,      // N
	0x12|SHIFT,      // O
	0x13|SHIFT,      // P
	0x14|SHIFT,      // Q
	0x15|SHIFT,      // R
	0x16|SHIFT,      // S
	0x17|SHIFT,      // T
	0x18|SHIFT,      // U
	0x19|SHIFT,      // V
	0x1a|SHIFT,      // W
	0x1b|SHIFT,      // X
	0x1c|SHIFT,      // Y
	0x1d|SHIFT,      // Z
	0x2f,          // [
	0x31,          // bslash
	0x30,          // ]
	0x23|SHIFT,    // ^
	0x2d|SHIFT,    // _
	0x35,          // `
	0x04,          // a
	0x05,          // b
	0x06,          // c
	0x07,          // d
	0x08,          // e
	0x09,          // f
	0x0a,          // g
	0x0b,          // h
	0x0c,          // i
	0x0d,          // j
	0x0e,          // k
	0x0f,          // l
	0x10,          // m
	0x11,          // n
	0x12,          // o
	0x13,          // p
	0x14,          // q
	0x15,          // r
	0x16,          // s
	0x17,          // t
	0x18,          // u
	0x19,          // v
	0x1a,          // w
	0x1b,          // x
	0x1c,          // y
	0x1d,          // z
	0x2f|SHIFT,    // {
	0x31|SHIFT,    // |
	0x30|SHIFT,    // }
	0x35|SHIFT,    // ~
	0				// DEL
};


uint8_t USBPutChar(uint8_t c);

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way
// USB HID works, the host acts like the key remains pressed until we
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t BleKeyboard::press(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers |= (1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = pgm_read_byte(_asciimap + k);
		if (!k) {
			setWriteError();
			return 0;
		}
		if (k & 0x80) {						// it's a capital letter or other character reached with shift
			_keyReport.modifiers |= 0x02;	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Add k to the key report only if it's not already present
	// and if there is an empty slot.
	if (_keyReport.keys[0] != k && _keyReport.keys[1] != k &&
		_keyReport.keys[2] != k && _keyReport.keys[3] != k &&
		_keyReport.keys[4] != k && _keyReport.keys[5] != k) {

		for (i=0; i<6; i++) {
			if (_keyReport.keys[i] == 0x00) {
				_keyReport.keys[i] = k;
				break;
			}
		}
		if (i == 6) {
			setWriteError();
			return 0;
		}
	}
	sendReport(&_keyReport);
	return 1;
}

size_t BleKeyboard::press(const MediaKeyReport k)
{
    uint16_t k_16 = k[1] | (k[0] << 8);
    uint16_t mediaKeyReport_16 = _mediaKeyReport[1] | (_mediaKeyReport[0] << 8);

    mediaKeyReport_16 |= k_16;
    _mediaKeyReport[0] = (uint8_t)((mediaKeyReport_16 & 0xFF00) >> 8);
    _mediaKeyReport[1] = (uint8_t)(mediaKeyReport_16 & 0x00FF);

	sendReport(&_mediaKeyReport);
	return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t BleKeyboard::release(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers &= ~(1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = pgm_read_byte(_asciimap + k);
		if (!k) {
			return 0;
		}
		if (k & 0x80) {							// it's a capital letter or other character reached with shift
			_keyReport.modifiers &= ~(0x02);	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Test the key report to see if k is present.  Clear it if it exists.
	// Check all positions in case the key is present more than once (which it shouldn't be)
	for (i=0; i<6; i++) {
		if (0 != k && _keyReport.keys[i] == k) {
			_keyReport.keys[i] = 0x00;
		}
	}

	sendReport(&_keyReport);
	return 1;
}

size_t BleKeyboard::release(const MediaKeyReport k)
{
    uint16_t k_16 = k[1] | (k[0] << 8);
    uint16_t mediaKeyReport_16 = _mediaKeyReport[1] | (_mediaKeyReport[0] << 8);
    mediaKeyReport_16 &= ~k_16;
    _mediaKeyReport[0] = (uint8_t)((mediaKeyReport_16 & 0xFF00) >> 8);
    _mediaKeyReport[1] = (uint8_t)(mediaKeyReport_16 & 0x00FF);

	sendReport(&_mediaKeyReport);
	return 1;
}

void BleKeyboard::releaseAll(void)
{
	_keyReport.keys[0] = 0;
	_keyReport.keys[1] = 0;
	_keyReport.keys[2] = 0;
	_keyReport.keys[3] = 0;
	_keyReport.keys[4] = 0;
	_keyReport.keys[5] = 0;
	_keyReport.modifiers = 0;
    _mediaKeyReport[0] = 0;
    _mediaKeyReport[1] = 0;
	sendReport(&_keyReport);
}

size_t BleKeyboard::write(uint8_t c)
{
	uint8_t p = press(c);  // Keydown
	release(c);            // Keyup
	return p;              // just return the result of press() since release() almost always returns 1
}

size_t BleKeyboard::write(const MediaKeyReport c)
{
	uint16_t p = press(c);  // Keydown
	release(c);            // Keyup
	return p;              // just return the result of press() since release() almost always returns 1
}

size_t BleKeyboard::write(const uint8_t *buffer, size_t size) {
	size_t n = 0;
	while (size--) {
		if (*buffer != '\r') {
			if (write(*buffer)) {
			  n++;
			} else {
			  break;
			}
		}
		buffer++;
	}
	return n;
}

void BleKeyboard::onConnect(BLEServer* pServer) {
  this->connected = true;

#if !defined(USE_NIMBLE)

  BLE2902* desc = (BLE2902*)this->inputKeyboard->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(true);
  desc = (BLE2902*)this->inputMediaKeys->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(true);

#endif // !USE_NIMBLE

}

void BleKeyboard::onDisconnect(BLEServer* pServer) {
  this->connected = false;

#if !defined(USE_NIMBLE)

  BLE2902* desc = (BLE2902*)this->inputKeyboard->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(false);
  desc = (BLE2902*)this->inputMediaKeys->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(false);

  advertising->start();

#endif // !USE_NIMBLE
}

void BleKeyboard::onWrite(BLECharacteristic* me) {
  uint8_t* value = (uint8_t*)(me->getValue().c_str());
  (void)value;
  ESP_LOGI(LOG_TAG, "special keys: %d", *value);
}

void BleKeyboard::delay_ms(uint64_t ms) {
  uint64_t m = esp_timer_get_time();
  if(ms){
    uint64_t e = (m + (ms * 1000));
    if(m > e){ //overflow
        while(esp_timer_get_time() > e) { }
    }
    while(esp_timer_get_time() < e) {}
  }
}