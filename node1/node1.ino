#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <LowPower.h>
#include <printf.h>

#define CE_PIN 7
#define CSN_PIN 8

RF24 radio(CE_PIN, CSN_PIN);  // Initialize the radio, see block of Serial.println lines below for wiring guidance.

// *****************************************************************************************
// *** NOTE: Change the next two lines depending on which radio you are programming
// ***       This must match the values in the Raspberry Pi code.  If you have 2
// ***       devices then RPi needs at least 2 devices, if you have 4 devices then RPi needs 4
// ***       The max number of devices you can have per RPi is 6.
// ***       (unless you do something with the channels which would get tricky fast)
// *****************************************************************************************
// *** I used this library and examples for my project: 
// ***       http://tmrh20.github.io/RF24/
// *** Ultra Low Power instructions can be found here:
// ***       http://home-automation-community.com/arduino-low-power-how-to-run-atmega328p-for-a-year-on-coin-cell-battery/
// *****************************************************************************************
const int num =           1;
const byte address[6] = {"1Node"};

const int wakeUpPin = 2;            // Pin 2 is the open/closed sensor pin and also use it as the wake up pin to trigger interrupt
const int battPin = A0;             // used to get the battery status, dont use pin A0 for any other purpose

int volatile openClosed;            // used to hold current state of open/closed pin (1 or 0) (same as wakeUpPin)
long volatile millivolts;           // used to hold the battery remaining millivolts
unsigned volatile long msg = 0;     // above variables combine into a 6 digit value in this field which is what is transmitted

// initialize these vars so we will transmit the current state (open or closed) on boot up
int volatile initRun = 1;           // used simply to warm up the battery sensor and send on initial state
int volatile wakeCnt = 1;           // used to indicate an interrupt was received
int volatile sendMsg = 1;           // used to indicate that a transmission should be sent on this loop

void setup() {
  
  Serial.begin(115200);             // this is just to setup the console output window.  This should match the setting in the console
  printf_begin();                   // this is required to get the radio.printDetails function to work

  radio.begin();                    // start the radio as wired from line 10
  
  // These radio settings all have to match the RPi and since we are using 1 RPi for multi Arudinos then, by extension, the other 
  // Arduinos much also have the same settings. 
  radio.setPALevel(RF24_PA_MAX);    // My transmitter was far away so I boost to max to send.  This uses a bit more power but not enough to cause me problems
  radio.setDataRate(RF24_250KBPS);  // I choose the slowest data rate because I'm only sending 6 bytes and lower rates are more successful with interfence
  radio.setAutoAck(true);           // Im not doing anything fancy so AutoAck is good for me
  radio.setChannel(108);            // the 2.4GHz is a busy frequency so I picked a channel I thought would be less cluttered in my home.
  radio.openWritingPipe(address);   // these are the transmitters so we open the radio for writing
  radio.stopListening();            // these are the transmitters so we don't listen (this may not be necessary, Im not sure of the default)

  pinMode(wakeUpPin, INPUT_PULLUP); // define the pin that will be the switch to open / close (pulling up sets the default instead of letting it be arbitary)

  delay(50);                        // the delays are for the serial output to flush
  
  // There are many wiring diagrams for these components but here is one of the ones I used
  // https://howtomechatronics.com/tutorials/arduino/arduino-wireless-communication-nrf24l01-tutorial/
  Serial.println("Pinouts Arduino to NRF24L01+");
  Serial.println("VIN or RAW -> Battery (+) - 3v for Pro Micro, 5v for Nano, others");
  Serial.println("GND -> Battery (-)");
  Serial.println("D2  -> GND = Closed -and- D2 -> <open> = Open");  //D2 is the same as the wakeUp pin and since we did an INPUT_PULLUP then taking the pin low (to ground) indicates that the circut is closed
  Serial.println("GND -> [NRF24L01+] GND  (Pin 1)");
  Serial.println("3V3 -> [NRF24L01+] VCC  (Pin 2)");
  Serial.println("D7  -> [NRF24L01+] CE   (Pin 3)");
  Serial.println("D8  -> [NRF24L01+] CSN  (Pin 4)");
  Serial.println("D11 -> [NRF24L01+] MOSI (Pin 6)");
  Serial.println("D12 -> [NRF24L01+] MISO (Pin 7)");
  Serial.println("D13 -> [NRF24L01+] SCK  (Pin 5)");
  Serial.println("A0  -> <reserved but not connected>");
  Serial.println("");
  Serial.println("No other Arduino pins needed");
  Serial.println("No other NRF24L01+ pins needed");
  Serial.println("Do no use Arduino pin A0 as it is used for sensing internal voltage to report remaining battery millivolts");
  Serial.println("Other Arduino pins may be used as needed");
  Serial.println("");
  Serial.println("** Boot up complete **");
  Serial.println("");

  radio.printDetails();  // if you get a bunch of 0x000000 and the data looks sparse your raido is either not hooked up right or it's fried.

}

// function called when an interrupt is caught
void wakeUp() {

  detachInterrupt(wakeUpPin);
  wakeCnt = 1;
  sendMsg = 0;
}

// I used this technique for calculating remaining battery.  I cannot say for certian that it is accurate
// but it works for my purposes: https://code.google.com/archive/p/tinkerit/wikis/SecretVoltmeter.wiki
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

void loop() {

  if (initRun == 1) {  // initial run condition to 'warm up' vcc test and set current state of lock
    Serial.println("Initializing Run... ");
    delay (2000);  // sleep for a couple seconds to let things settle (ie. vcc)
    initRun = 0;
  }
  else // normal run condition
  { 
    if (wakeCnt == 1) { //wakeCnt = 1 means we are after an intrrupt has fired, maybe many intrrupts.

    // this will prevent any flutter of interrupts resulting in a race condition and an invalid setting of the state of the lock
      LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
      
      // this will cause the method wakeUp() to be called whenever there is a change in the wakeUpPin (ie. the pin was closed (low) or opened)
      attachInterrupt(digitalPinToInterrupt(wakeUpPin), wakeUp, CHANGE);
      wakeCnt = 0; // reset wakeCnt so the arduino will go into low power on the next loop
      sendMsg = 1; // actually transmit state on exit of this for loop (after next line resumes from 4 second power down)
    } else {
      // To save the most power the arduino is put into low power mode FOREVER.  Then the
      // attachInterrupt is used (enabed after the 4 seconds sleep above) to set the 
      // wakeCnt to 1 so that the next loop will fall into the if block above which will
      // put the arduino back to sleep for 4 seconds before sending a final transmission with the
      // current state of the sensor.
      // 
      // This is necessary because the attachInterrupt fires repeatedly as the contact closes.
      // This logic will greatly reduce transmitting a flood of messages which sometimes results in a 
      // race condition leaving the last transmitted state incorrect.

      sendMsg = 0; //reset the intrrupt flag to false because no need to send on initial wakeup until we sleep for 4s
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    }
  }
  
  // you will likely notice that delicate connections of the contacts causes several transmissions.
  // This is because the interrupt causes the arduino void-loop to act in an asynchronous manner
  // and since we are using global variables to indicate when to send a message then if there
  // are interrupts caught at the right time then transmissions will be sent.  There's not much
  // we can do to eliminate this but the extra transmissions are greatly reduced with the attaching
  // and detaching of the interrupts logic above.

  if (sendMsg == 1) { // we really want to transmit a message

    millivolts = readVcc();  // get the current voltage

    if (digitalRead(wakeUpPin) == LOW) {  // read the state of the open/close pin which is the same as the interrupt pin
      openClosed = 2; // for my setup, LOW is closed (grounded)
    } else {
      openClosed = 1; // not grounded is OPEN
    }

    // this simple bit of math is to create a single int that represents the message I want to send.  
    // You can do your own thing here but I'll explain what I'm doing.
    // num = the raido (node) id and is in position _XXXXX
    // openClosed = the current state of the sensor and is in position X_XXXX
    // millivolts are simply added to the end so they are in position XX____ (4 digits)
    msg = (num*100000) + (openClosed*10000) + millivolts; 

    // Some logging for when the arduino is being tested
    Serial.print("Node ");
    Serial.print(num);
    Serial.print(" is ");

    // more logging in human readable form
    if (openClosed == 2) {
      Serial.print("LOCKED");
    } else {
      Serial.print("UNLOCKED");
    }

    // yes I like my logging
    Serial.print(" with millivolts = ");
    Serial.print( millivolts, DEC ); 
    Serial.println(" remaining.");

    // it helps me debug
    Serial.print("Sending ");
    Serial.print(msg, DEC);
    Serial.print("... ");
    delay(50);  //allow the serial output to clear before sleeping for debugging

    radio.write(&msg, sizeof(msg)); // ACTUALLY TRANSMIT THE MESSAGE

    Serial.println(" ...Sent.");
    delay(50);  //allow the serial output to clear before sleeping for debugging
  }
}


