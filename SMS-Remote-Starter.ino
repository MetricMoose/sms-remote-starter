#include "LowPower.h"
#include <SoftwareSerial.h>
#include <OneWire.h> 
#include <DallasTemperature.h>

// SIM900 Shield Serial Pins
SoftwareSerial SIM900(7, 8);

// Set to true if you want confirmation text messages
boolean respond = false;

// Key fob pins
int lockPin = 6;
int starterPin = 5;
int unlockPin = 4;
int auxPin = 3;

// Replace with the number of the controlling phone
String myPhoneNum = "+17055555555";

// DS18B20 Temperature Sensor Data Pin
int tempPin = 2;

OneWire oneWire(tempPin);
DallasTemperature sensors(&oneWire);

void setup()
{
  pinMode(9, OUTPUT);
  pinMode(13, OUTPUT);
  
  pinMode(lockPin, OUTPUT);
  pinMode(starterPin, OUTPUT);
  pinMode(unlockPin, OUTPUT);
  pinMode(auxPin, OUTPUT);
  
  Serial.begin(9600); // for serial monitor
  Serial.println("Starting.");
  SIM900.begin(9600); // Serial for GSM shield
  SIM900poweron();  // turn on shield
  
  //Wake up modem with an AT command
  sendCommand("AT", "OK", 1000);
  if(sendCommand("AT", "OK", 1000) == 1){
    Serial.println("Module started");
  }
  if(sendCommand("AT+CNMI=0,0,0,0,0", "OK", 1000) == 1){
    Serial.println("Notifications disabled");
  }
  if(sendCommand("AT+CMGF=1", "OK", 1000) == 1){
    Serial.println("Text mode enabled");
  }
  if(sendCommand("AT+CSCLK=2", "OK", 1000) == 1){
    Serial.println("Sleeping enabled");
  }
  sensors.begin();
}



void loop()
{
  //Wake up modem with two AT commands
  sendCommand("AT", "OK", 1000);
  sendCommand("AT", "OK", 2000);
  // Check if it's currently registered to Speakout
  if(sendCommand("AT+CREG?","+CREG: 0,1",1000)){ 
    
    Serial.println("Connected to the home cell network");
    
    // Try to get the first SMS. Reply prefixed with +CMGR: if there's a new SMS
    if(sendCommand("AT+CMGR=1", "+CMGR: ", 1000) == 1){ 
      String serialContent = "";
      char serialCharacter;
      
      while(SIM900.available()) {
        serialCharacter = SIM900.read();
        serialContent.concat(serialCharacter);
        delay(10);
      }
      
      // Dividing up the new SMS
      String smsNumber = serialContent.substring(14,26);
      
      String smsMessage = serialContent.substring(53,serialContent.length()-8);
      smsMessage.trim();
      smsMessage.toLowerCase();
      
      Serial.println("New SMS Message");
      Serial.println(smsNumber);
      Serial.println(smsMessage);
      
      // Delete all SMS messages in memory
      sendCommand("AT+CMGD=1,4", "OK", 1000);
 
      // Check if it's coming from my phone
      if(smsNumber == myPhoneNum){
        if(smsMessage == "start"){
          carStart();
          if(respond){ sendSms("Car Started"); }
        }
        else if(smsMessage == "lock"){
          carLock();
          if(respond){ sendSms("Doors Locked"); }
        }
        else if(smsMessage == "unlock"){
          carUnlock();
          if(respond){ sendSms("Doors Unlocked"); }
        }
        else if(smsMessage == "trunk"){
          carTrunk();
          if(respond){ sendSms("Trunk Popped"); }
        }
        else if(smsMessage == "finder"){
          carStart();
          if(respond){ sendSms("Finder Activated"); }
        }
        else if(smsMessage == "rspon"){
          respond = true;
          sendSms("Respond to commands: On");
        }
        else if(smsMessage == "rspoff"){
          respond = false;
          sendSms("Respond to commands: Off");
        }
        else if(smsMessage == "temp"){
          sensors.requestTemperatures();

          // Get temperature as a float and convert it to a string
          float temperature = sensors.getTempCByIndex(0);
          char buffer[10];
          String temperatureString = dtostrf(temperature, 5, 1, buffer);

          String tempMsg = "Temperature: " + temperatureString;
          Serial.println(tempMsg);
          sendSms(tempMsg);
        }
        else if (smsMessage == "ping"){
          sendSms("Pong!");
        }
        else{
          if(respond){ sendSms("Invalid command"); }
          Serial.println("Invalid command");
        }
      }
    }
    else{
      Serial.println("No SMS messages"); 
    }
  }
  else{
    Serial.println("Not connected to home cell network");
  }
  

  delay(500);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  

}


void sendSms(String message){
  Serial.println("Sending message");
  char smsCommand[23];
  String commandStr = "AT+CMGS=\"" + myPhoneNum + "\"";
  commandStr.toCharArray(smsCommand,23);
  
  if(sendCommand(smsCommand, ">", 1000)){
    SIM900.println(message);
    SIM900.println((char)26);
    SIM900.println();
    delay(2000);
  }
}

void carLock(){
  digitalWrite(lockPin, HIGH);
  delay(1000);
  digitalWrite(lockPin, LOW);
  Serial.println("Doors locked");
}

void carUnlock(){
  digitalWrite(unlockPin, HIGH);
  delay(1000);
  digitalWrite(unlockPin, LOW);
  Serial.println("Doors unlocked");
}

void carStart(){
  digitalWrite(starterPin, HIGH);
  delay(1000);
  digitalWrite(starterPin, LOW);
  Serial.println("Car Starting");  
}

void carTrunk(){
  digitalWrite(auxPin, HIGH);
  delay(2000);
  digitalWrite(auxPin, LOW);
  Serial.println("Trunk Released");  
}

void carFind(){
  digitalWrite(auxPin, HIGH);
  digitalWrite(starterPin, HIGH);
  delay(1000);
  digitalWrite(auxPin, LOW);
  digitalWrite(starterPin, LOW);
  Serial.println("Car Finder Activated");
}


void SIM900poweron()
// Software equivalent of pressing the GSM shield "power" button
{
  //Wake up modem with an AT command
  sendCommand("AT", "OK", 1000);
  if(sendCommand("AT", "OK", 2000) == 0){
    digitalWrite(9, HIGH);
    delay(1000);
    digitalWrite(9, LOW);
    delay(7000);
  }

}

int sendCommand(char* ATcommand, char* expected_answer, unsigned int timeout){

    
    int answer=0;
    int responsePos=0;
    char response[100];
    unsigned long previous;

    memset(response, '\0', 100);    // Clears array

    delay(100);

    while( SIM900.available() > 0) SIM900.read();    // Clean the input buffer

    SIM900.println(ATcommand);    // Send the AT command 


    responsePos = 0;
    previous = millis();

    // this loop waits for the answer
    do{
        // if there are data in the UART input buffer, reads it and checks for the asnwer
        if(SIM900.available() != 0){    
            response[responsePos] = SIM900.read();
            responsePos++;
            // check if the desired answer is in the response of the module
            if (strstr(response, expected_answer) != NULL)    
            {
              answer = 1;
            }
        }
        // Waits for the asnwer with time out
    }while((answer == 0) && ((millis() - previous) < timeout));    
    return answer;
}


