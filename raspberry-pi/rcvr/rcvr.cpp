/*
    This is a basic cpp program that uses the RF24 library to receive a transmission and do something useful with it
    RF24 Library: https://maniacbug.github.io/RF24/index.html
    
    Author: Jason Brown
    Date:   Sept 13, 2018
    
*/

/**

Pinouts Raspberry Pi B+ to ...
GPIO22 (pin 15) -> [NRF24L01+] CE (Pin 3)
3v3 (pin 17)    -> [NRF24L01+] VCC (Pin 2)
GPIO10 (pin 19) -> [NRF24L01+] MOSI (Pin 6)
GPIO9 (pin 21)  -> [NRF24L01+] MISO (Pin 7)
GPIO11 (pin 23) -> [NRF24L01+] SCK (Pin 5)
GPIO8 (pin 24)  -> [NRF24L01+] CSN (Pin 4)
GND (pin 25)    -> [NRF24L01+] GND (Pin 1)

*/

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <RF24/RF24.h>
#include <stdio.h>

using namespace std;

// http://tmrh20.github.io/RF24/RPi.html (pinouts under Connections and Pin Config section)
// Setup for GPIO 15 CE and CE0 CSN with SPI Speed @ 8Mhz
RF24 radio(RPI_BPLUS_GPIO_J8_15,RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);


// Radio pipe addresses for the 3 nodes to communicate.  ** This must match the 'address' for each arduino (line 25 in node1/2/3.ino)
const uint8_t addresses[3][6] = {"1Node","2Node","3Node"};

// this is a helper function to output a timestame prefix for every logging line
void printf_timestamp() {
    time_t ltime; /* calendar time */
    ltime=time(NULL); /* get current cal time */
    char *foo = asctime( localtime(&ltime) ); /* get timestamp */
    foo[strlen(foo) - 1] = 0; /* remove trailing newline */
    printf("%s - ", foo);
}

// This function is your main handler function.
// This is where you put the code to handle the message.
// I'm sending it to a web endpoint via curl call
void saveResult (unsigned long payload) {
  char postbody [200]; //text below should be less than 175 chars
  
  // build the curl command, if it's longer than 200 chars remember to change the array size on the above line
  sprintf(postbody, "curl -s -X POST --Header \"Content-Type: application/json\" https://<your_site>/<your_endpoint> -d \"{\\\"payload\\\":%ld}\"", payload);

  popen(postbody,"r");  // Actually exceute the curl command...
  //     I don't care about the result because there's no point in resending.
  //     Without receiving a new transmisison from the source, a resend could
  //     overwrite a later (successful) transmission since these messages are
  //     received asynchronously.
  
  
}

int main(int argc, char** argv){

  printf_timestamp();
  printf("The key to make the radios work together is to have the CE Pin wired correctly and\n"
  printf_timestamp();
  printf("ensure that that all setting match (RX_ADDR_P*, Data Rate, CRC & radio addresses)\n");

  // Setup and configure rf radio
  radio.begin();
  radio.setRetries(3,5);  // delay, count
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(108);
  
  radio.openReadingPipe(0,addresses[0]);  // Yes this is sloppy, please forgive me.  I didn't have time to figure out if this was indexed at 0 or 1.
  radio.openReadingPipe(1,addresses[1]);
  radio.openReadingPipe(2,addresses[2]);
  radio.openReadingPipe(3,addresses[3]);

  // Output the configuration of the rf unit for debugging
  radio.printDetails();

  radio.startListening();
  fflush(stdout); // flush the write buffer so I can read the logs in realtime
  
  int t=1; // using this to write out the transmission received lines in pairs

  //
  // future upgrade: use interrupt instead of polling.  https://gist.github.com/TMRh20/fa015027c6e39fd984f3
  //                 this branch of nRF24 git has more on interrupts:
  //                 https://github.com/nRF24/RF24/blob/Interrupts/examples_RPi/interrupts/gettingstarted_call_response_int.cpp
  //
  // While using polling you shouldn't use the raspberry pi that this is running on for anything else but receiving tx from IoT sensors
  //    doing so could cause your tx to be lost.  This is because if the CPU is doing something besides polling on the radio.available()
  //    then there is no buffer to get the transmission.  There are some challenges using interrupts on a linux board because
  //    Linux was designed to handle all interrupts at the kernel level, not at the application level.  Some techniques exists to
  //    trick the RPi into handling an interrupt like event but I haven't implemented it here because the polling works for my purposes.
  //
  while (1) {  // Loop forever
    
    if (radio.available()) {  // if a transmission is received
      unsigned long msg;

      radio.read( &msg, sizeof(msg) );  // read the transmission into the msg variable
      
      // log the receipt of the message
      printf_timestamp();
      printf("Message Recieved from Node %ld, Saving.\n", msg);
      
      // do something useful with the message (you write your custom logic in this method above)
      saveResult(msg);

      if (t==2) {  // using this to write out the transmission received lines in pairs
        printf("\n");
        t=1;
      } else {
        t++;
      }
      fflush(stdout);  // flush the write buffer so I can read the logs in realtime
    }
  } // forever loop

  return 0;
}
