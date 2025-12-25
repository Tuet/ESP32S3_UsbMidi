
#define USB_MANUFACTURER "YOUR_NAME"
#define USB_PRODUCT "YOUR_PRODUCT"

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

#include <WS2812Write.h>
#define RGB_PIN 21

// USB MIDI object
Adafruit_USBD_MIDI usb_midi;

// Create a new instance of the Arduino MIDI Library,
// and attach usb_midi as the transport.
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

// Variable that holds the current position in the sequence.
uint32_t position = 0;

// Store example melody as an array of note values
byte note_sequence[] = {
    74, 78, 81, 86, 90, 93, 98, 102, 57, 61, 66, 69, 73, 78, 81, 85, 88, 92, 97, 100, 97, 92, 88, 85, 81, 78,
    74, 69, 66, 62, 57, 62, 66, 69, 74, 78, 81, 86, 90, 93, 97, 102, 97, 93, 90, 85, 81, 78, 73, 68, 64, 61,
    56, 61, 64, 68, 74, 78, 81, 86, 90, 93, 98, 102
};


void setup() {
  ws2812Write(RGB_PIN, 0x100000); 
  
  // Manual begin() is required on core without built-in support e.g. mbed rp2040
  if (!TinyUSBDevice.isInitialized()) {
    TinyUSBDevice.begin(0);
  }

  // Beware: Most OS remembers the Descriptors once they see them first with the given ID
  // To enforce refresh you might try to set a new VID here
  TinyUSBDevice.setManufacturerDescriptor("ManufactDesci");
  TinyUSBDevice.setProductDescriptor("ProdDescri");
  TinyUSBDevice.setID(0x303a, 0x8000); // 0x8000 is Espressif Test VID, use it only for testing purposes
  usb_midi.setStringDescriptor("TinyUSB MIDI1"); // Seems not used!
  usb_midi.setCableName(1, "ESP32S3 Midi"); // Seems to be added after ProductDescriptor in some programs like Ableton

  Serial.begin(115200);


  // Initialize MIDI, and listen to all MIDI channels
  // This will also call usb_midi's begin()
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // If already enumerated, additional class driverr begin() e.g msc, hid, midi won't take effect until re-enumeration
  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }
  

  // Attach the handleNoteOn function to the MIDI Library. It will
  // be called whenever the Bluefruit receives MIDI Note On messages.
  MIDI.setHandleNoteOn(handleNoteOn);

  // Do the same for MIDI Note Off messages.
  MIDI.setHandleNoteOff(handleNoteOff);
}

void loop() {
  #ifdef TINYUSB_NEED_POLLING_TASK
  // Manual call tud_task since it isn't called by Core's background
  TinyUSBDevice.task();
  #endif

  // not enumerated()/mounted() yet: nothing to do
  if (!TinyUSBDevice.mounted()) {
    ws2812Write(RGB_PIN, 0x000010); delay(1);
    return;
  }

  static uint32_t start_ms = 0;
  if (millis() - start_ms > 266) {
    ws2812Write(RGB_PIN, (position&1)?0x001000:0x000000); 
    start_ms += 266;
    Serial.print("♫");

    // Setup variables for the current and previous
    // positions in the note sequence.
    int previous = position - 1;

    // If we currently are at position 0, set the
    // previous position to the last note in the sequence.
    if (previous < 0) {
      previous = sizeof(note_sequence) - 1;
    }

    // Send Note On for current position at full velocity (127) on channel 1.
    MIDI.sendNoteOn(note_sequence[position], 127, 1);

    // Send Note Off for previous note.
    MIDI.sendNoteOff(note_sequence[previous], 0, 1);

    // Increment position
    position++;

    // If we are at the end of the sequence, start over.
    if (position >= sizeof(note_sequence)) {
      position = 0;
    }
  }

  // read any new MIDI messages
  MIDI.read();
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  // Log when a note is pressed.
  
  Serial.print("Note on: channel = ");
  Serial.print(channel);

  Serial.print(" pitch = ");
  Serial.print(pitch);

  Serial.print(" velocity = ");
  Serial.println(velocity);
  
  ws2812Write(RGB_PIN, (uint32_t)pitch << 8 + velocity); delayMicroseconds(90);
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  // Log when a note is released.
  
  Serial.print("Note off: channel = ");
  Serial.print(channel);

  Serial.print(" pitch = ");
  Serial.print(pitch);

  Serial.print(" velocity = ");
  Serial.println(velocity);
  
}