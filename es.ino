#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <DHT.h>;
#include <lmic.h>
#include <Adafruit_Sensor.h>
#include <DHT_U.h>

#define DHTPIN 4     // Digital pin connected to the DHT sensor 

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

// See guide for details on sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;
//!\\ Declare and Initialize DHT here

// Schedule the transmission every 
unsigned int TX_INTERVAL = 20; //seconds

static const u1_t PROGMEM APPEUI[8] = { // LSB format
0x54, 0x12, 0x45, 0x36, 0x62, 0x37, 0x26, 0x67
};
void os_getArtEui (u1_t* buf) {memcpy_P(buf, APPEUI, 8);}

static const u1_t PROGMEM DEVEUI[8] = { // LSB format
0x77, 0x63, 0x52, 0x63, 0x73, 0x62, 0x53, 0x34

};
void os_getDevEui (u1_t* buf) {memcpy_P(buf, DEVEUI, 8);}

static const u1_t PROGMEM APPKEY[16] = { // MSB format
0xAF, 0x94, 0x2D, 0x79, 0x09, 0xF7, 0x84, 0x78, 0xD6, 0xCA, 0x18, 0xF2, 0x48, 0x74, 0x96, 0x2F
};
void os_getDevKey (u1_t* buf) {memcpy_P(buf, APPKEY, 16);}

// These pins are used to connect with RFM96 module (on UCA board only)
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LMIC_UNUSED_PIN,
    .dio = {2, 7, 8},
};

static osjob_t sendjob;
static float Vcc;
//!\\ Temp and Hum variables here

// ---------------------------------------------- //
//        Read Voltage supplied for Arduino       //
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result;
  return result; // Return Vcc in mV
}
// ---------------------------------------------- //

// ---------------------------------------------- //
//  Update new values from Sensor for the next TX //
void updateParameters() {
  Serial.println("\n___Updating value. . .");
  Vcc = (int)(readVcc()); // returns Vcc in mVolt
  Serial.print("Vcc = "); Serial.print(Vcc/1000); Serial.println(" V");
  delay(2000);

  //!\\ Read and print Temp and Hum here
  
}
// ---------------------------------------------- //

// ---------------------------------------------- //
//              LoRaWAN events (LMiC)
void onEvent (ev_t ev) {
  Serial.print(os_getTime()); Serial.print(": ");
  switch (ev) {
    case EV_SCAN_TIMEOUT: Serial.println(F("EV_SCAN_TIMEOUT")); break;
    case EV_BEACON_FOUND: Serial.println(F("EV_BEACON_FOUND")); break;
    case EV_BEACON_MISSED: Serial.println(F("EV_BEACON_MISSED")); break;
    case EV_BEACON_TRACKED: Serial.println(F("EV_BEACON_TRACKED")); break;
    case EV_JOINING: Serial.println(F("EV_JOINING")); break;      
    case EV_JOINED: Serial.println(F("EV_JOINED")); LMIC_setLinkCheckMode(0); break;
    case EV_JOIN_FAILED: Serial.println(F("EV_JOIN_FAILED")); LMiC_Startup(); break;
    case EV_REJOIN_FAILED: Serial.println(F("EV_REJOIN_FAILED")); LMiC_Startup(); break; 
    case EV_LOST_TSYNC: Serial.println(F("EV_LOST_TSYNC")); break;
    case EV_RESET: Serial.println(F("EV_RESET")); break;
    case EV_RXCOMPLETE: Serial.println(F("EV_RXCOMPLETE")); break;
    case EV_LINK_DEAD: Serial.println(F("EV_LINK_DEAD")); break;
    case EV_LINK_ALIVE: Serial.println(F("EV_LINK_ALIVE")); break;
    
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE"));
      if (LMIC.txrxFlags & TXRX_ACK) Serial.println(F("Received ack"));
      if (LMIC.dataLen) { Serial.print(F("Received ")); Serial.print(LMIC.dataLen); Serial.println(F(" bytes.")); }
      delay(50);
      os_setTimedCallback(&sendjob, os_getTime() + ms2osticks(TX_INTERVAL*1000), do_send);
      break;

    default: Serial.print(F("Unknown event: ")); Serial.println((unsigned) ev);
    break;
  }
}
// ---------------------------------------------- //


// ---------------------------------------------- //
//      Send the encrypted data packet to TTN     //
void do_send(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {Serial.println(F("OP_TXRXPEND, not sending"));}
  else {
    updateParameters();

    // Prepare "Hello World" payload
    const char *helloWorld = "Hello World";
    size_t payloadSize = strlen(helloWorld) + 1;
    uint8_t data[payloadSize];
    memcpy(data, helloWorld, payloadSize);

    // Send the payload
    LMIC_setTxData2(1, data, payloadSize, 0);
    Serial.println(F("Packet queued !!!"));
  }
  // Next TX is scheduled after TX_COMPLETE event.
}
// ---------------------------------------------- //


// ---------------------------------------------- //
//      Called from setup to activate Device      //
void LMiC_Startup() {
  os_init();
  LMIC_reset(); LMIC_setLinkCheckMode(1); LMIC_setAdrMode(1); // Reset the MAC state
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100); // Increase window time for clock accuracy problem
  LMIC_startJoining(); // Join the network, sending will be started after the event "Joined"
}
// ---------------------------------------------- //

// ---------------------------------------------- //
//                   Device setup                   //
void setup() {
  Serial.begin(9600);

  //TEMP FUNCTION

    dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;

  //LORA FUNCTION
  Serial.println("\n*-____________________-*");
  Serial.println("  .::LoRa (Re)start::.");
  delay(100);

  //!\\ pinMode setup and Init DHT
  

  // Update value for the first packet to TTN
  Serial.println("Get value for the first packet.");
  updateParameters();
  
  LMiC_Startup();       // LMiC init
  do_send(&sendjob);    // OTAA start
}
// ---------------------------------------------- //

//*     .::Fixed LOOP - Do NOT interfered::.     *//
void loop() {
    // Delay between measurements.
  delay(delayMS);
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
  }
  
  os_runloop_once();
  
  
  }
