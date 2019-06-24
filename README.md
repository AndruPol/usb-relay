# usb-relay


relay control module via USB bus on STM32F103XX

ChibiOS 18.2 is required for compile this project

---
connect PC to module: minicom -D /dev/ttyACM0 

usage:

 r1:?,r2:? - get state relay 1, relay 2
 
 r1:1,r1:0 - set relay 1 on/off
 
 r2:1,r2:0 - set relay 2 on/off
 