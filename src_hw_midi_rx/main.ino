
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

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_HW);

// Classical Midi-Rx connected to UART, pin 1=rx, pin2=tx
// See circuit for Midi-Rx

void setup() {
  ws2812Write(RGB_PIN, 0x100000); 

  pinMode(1, INPUT_PULLUP);             // Start HW-Serial and set HW MIDI pins
  Serial1.begin(31250,SERIAL_8N1,1,2);
  MIDI_HW.begin(MIDI_CHANNEL_OMNI);

  
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

uint32_t last_MidiRx = 0;
bool last_MidiRxCleared = false;

void loop() {
  #ifdef TINYUSB_NEED_POLLING_TASK
  // Manual call tud_task since it isn't called by Core's background
  TinyUSBDevice.task();
  #endif

  // not enumerated()/mounted() yet: nothing to do
  if (!TinyUSBDevice.mounted()) {
    ws2812Write(RGB_PIN, 0x200020); delay(1);
    return;
  }

  if((millis() - last_MidiRx) > 10) {
    if(!last_MidiRxCleared) {
      ws2812Write(RGB_PIN, 0x000100);
      last_MidiRxCleared = true;
    }
    last_MidiRx = millis();
  }

  //if(Serial1.available()) {
  //  uint8_t r = Serial1.read();
  //  Serial.printf("1:%02x ", (int)r);
  //  
  //}

  if(Serial.available()) {
    uint8_t r = Serial.read();
    Serial.printf("0:%02x ", (int)r);
    Serial1.write(r);
  }

  // read any new MIDI messages
  if (MIDI.read())
  {
      // Thru on A has already pushed the input message to out A.
      // Forward the message to out B as well.
      midi::MidiType t = MIDI.getType();
      midi::DataByte d1 = MIDI.getData1();
      midi::DataByte d2 = MIDI.getData2();
      midi::Channel c = MIDI.getChannel();

      MIDI_HW.send(t,d1,d2,c);

      if((t != 0xf8) && (t != 0xfe)) {
        Serial.printf("SW t: 0x%02x, d1:0x%02x, d2:0x%02x, c:%u\r\n", (int)t,  (int)d1, (int)d2, (int)c);   
        ws2812Write(RGB_PIN, 0x101010);
      } else if(t == 0xf8) {
        ws2812Write(RGB_PIN, 0x040400);    
      } else if(t == 0xfe) {
        ws2812Write(RGB_PIN, 0x100000);    
      } 
      
      last_MidiRx = millis();
      last_MidiRxCleared = false;
  }

  if (MIDI_HW.read())
  {
    // Thru on A has already pushed the input message to out A.
    // Forward the message to out B as well.
    midi::MidiType t = MIDI_HW.getType();
    midi::DataByte d1 = MIDI_HW.getData1();
    midi::DataByte d2 = MIDI_HW.getData2();
    midi::Channel c = MIDI_HW.getChannel();

    MIDI.send(t,d1,d2,c);

    if((t != 0xf8) && (t != 0xfe)) {
      Serial.printf("HW t: 0x%02x, d1:0x%02x, d2:0x%02x, c:%u\r\n", (int)t,  (int)d1, (int)d2, (int)c);   
      ws2812Write(RGB_PIN, 0x101010);
    } else if(t == 0xf8) {
      ws2812Write(RGB_PIN, 0x040400);    
    } else if(t == 0xfe) {
      ws2812Write(RGB_PIN, 0x100000);    
    } 
    
    last_MidiRx = millis();
    last_MidiRxCleared = false;    
  }
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  // Log when a note is pressed.
  
  Serial.print("Note on: channel = ");
  Serial.print(channel);

  Serial.print(" pitch = ");
  Serial.print(pitch);

  Serial.print(" velocity = ");
  Serial.println(velocity);
  
  // ws2812Write(RGB_PIN, (uint32_t)pitch << 8 + velocity); delayMicroseconds(90);
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