# sms-remote-starter
This is the code for my Arduino-powered, SMS-controlled, remote starter trigger

The hardware I'm using is a SIM900 shield attached to an Arduino Uno clone. 

It controls the remote starter by sending pulses to wires soldered to the spare remote's button contacts. 

The Arduino and SIM900 shield are powered in the car using a 5V LM2596 Step Down Module, which steps the car's 12-14 volts down to a stable 5 volts. The remote is powered through the Arduino's 3.3V power pin.

Pictures here:
http://imgur.com/a/1iLDO

Video here:
https://www.youtube.com/watch?v=nciXATeEKHQ