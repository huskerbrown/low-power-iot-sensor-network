# Low-Power-Contact-Sensor-Network
Using Arudino Mini Pro with LED and voltage regulator removed add battery powered remote contact sensors anywhere that send updates to a central Raspberry Pi for posting to your central inventory.

# High level notes
Arduinos:

node1 & node2 & node3 are low power sleep on an intrrupt

Both programs are the same except for the variables num and address[6]
  const int num =           2;
  const byte address[6] = {"2Node"};

These programs were designed for the Arduino Nano or Pro-Mini using Arduino ide 1.8.5 in July 2018
```
  Pinouts Arduino to ...
  VIN or RAW -> Battery (+) - 3v for Pro Micro, 5v for Nano, others
  GND -> Battery (-)
  D2  -> GND = Locked -and- D2 -> <open> = Unlocked
  GND -> [NRF24L01+] GND (Pin 1)
  3V3 -> [NRF24L01+] VCC (Pin 2)
  D7  -> [NRF24L01+] CE (Pin 3)
  D8  -> [NRF24L01+] CSN (Pin 4)
  D11 -> [NRF24L01+] MOSI (Pin 6)
  D12 -> [NRF24L01+] MISO (Pin 7)
  D13 -> [NRF24L01+] SCK (Pin 5)
```
No other Arduino pins needed
No other NRF24L01+ pins needed
Do no use Arduino pin A0 as it is used for sensing internal voltage to report remaining battery millivolts
Other Arduino pins may be used as needed

Raspberry PI

The recvr.cpp app is designed for the Raspberry Pi - Strech
```
Pinouts Raspberry Pi B+ to ...
GPIO22 (pin 15) -> [NRF24L01+] CE (Pin 3)
3v3 (pin 17)    -> [NRF24L01+] VCC (Pin 2)
GPIO10 (pin 19) -> [NRF24L01+] MOSI (Pin 6)
GPIO9 (pin 21)  -> [NRF24L01+] MISO (Pin 7)
GPIO11 (pin 23) -> [NRF24L01+] SCK (Pin 5)
GPIO8 (pin 24)  -> [NRF24L01+] CSN (Pin 4)
GND (pin 25)    -> [NRF24L01+] GND (Pin 1)
```

Initial Config:
Install Raspbian Lite image
default user id is pi password is raspberry
`sudo raspi-config`

First Update the config program (option 8 when I did it), you may need to reboot.

Next, set localisation options before changing password
Change Locale
Finish

<reboot> (`sudo reboot now`)

default user id is pi password is raspberry
`sudo raspi-config`
Change Keyboard, Change timezone 
Interfacing Options: Enable SSH
Interfacing Options: Enabel SPI
Network Options: Set Hostname (rf24_gateway)
Network Options: Setup Wifi if you are going to use Wifi
Change password 
Finish

<reboot> (`sudo reboot now`)

Do a full update / upgrade
```
sudo apt-get -y update && sudo apt-get -y upgrade && sudo apt-get -y dist-upgrade
```

<reboot> (`sudo reboot now`)

```
sudo apt-get autoremove
```
install Git: `sudo apt-get install git`
install cmake: `sudo apt-get install cmake`

install NRF24 Libaray by following the steps here: http://tmrh20.github.io/RF24/RPi.html


After customizing rcvr.cpp and compiling it using make then to register rcvr as a service so that it runs at startup:

Copy the rcvr-service to /etc/init.d/ (you must use `sudo` since /etc is a privledged directory)

Make the script executable:
```
$ sudo chmod 755 /etc/init.d/rcvr-service
```

Then update the service registry to include rcvr-service

```
cd /etc/init.d
sudo update-rc.d rcvr-service defaults
```
