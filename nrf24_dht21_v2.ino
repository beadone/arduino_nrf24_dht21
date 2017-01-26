/*
   Dec 2014 - TMRh20 - Updated
   Derived from examples by J. Coliz <maniacbug@ymail.com>
*/
/**
 * Example for efficient call-response using ack-payloads 
 * 
 * This example continues to make use of all the normal functionality of the radios including 
 * the auto-ack and auto-retry features, but allows ack-payloads to be written optionlly as well. 
 * This allows very fast call-response communication, with the responding radio never having to 
 * switch out of Primary Receiver mode to send back a payload, but having the option to switch to 
 * primary transmitter if wanting to initiate communication instead of respond to a commmunication. 
 */

#include <SPI.h>
#include "RF24.h"
//#include "printf.h" // uncomment to make the radio.printDetails work
#include <string.h> 

/****************** User Config ***************************/
/***      Set this radio as radio number 0 or 1         ***/
bool radioNumber = 0;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(7,8);
/**********************************************************/
#include "DHT.h"
#define DHTPIN 6
#define DHTTYPE DHT21   // DHT 21
DHT dht(DHTPIN, DHTTYPE);
char  message[10];  //initialise
int i;
int j;
int retry_count = 0;
bool send_ok = 0;

char  gtemp[10];
char  ghum[10];
float h;
float t;
float hic;


String gt;
String gh;

unsigned long timer;

#define DHT21_PIN 6
#define MAX_RETRY_COUNT 20

                                                                           // Topology
//byte addresses[][6] = {"3Node","2Node"};              //House  Radio pipe addresses for the 2 nodes to communicate.
//byte addresses[][6] = {"1Node","2Node"}; 
// Backyard Radio pipe addresses for the 2 nodes to communicate.
//const uint64_t addresses[][6] = {0xF0F0F0F0E1LL, 0xF0F0F0F0E2LL};
//const uint64_t addresses[2] = {0xF0F0F0F0E1LL, 0xF0F0F0F0E2LL}; //pipe 1 -> outside
const uint64_t addresses[2] = {0xF0F0F0F0E3LL, 0xF0F0F0F0E2LL};  //pipe 2 house
//const uint64_t addresses[2] = {0xF0F0F0F0E4LL, 0xF0F0F0F0E2LL}; //pipe 3.. undefined
//{ 0xF0F0F0F0E1LL, 0xF0F0F0F0E2LL, 0xF0F0F0F0E3LL };
// Role management: Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.  


byte counter = 1;                                                          // A single byte to keep track of the data being sent back and forth


void setup(){

  Serial.begin(115200);

  // Setup and configure radio

  radio.begin();

  radio.enableAckPayload();                     // Allow optional ack payloads
  radio.enableDynamicPayloads();                // Ack payloads are dynamic payloads
  
//byte addresses[][6] = {"3Node","2Node"};              //House  Radio pipe addresses for the 2 nodes to communicate.
//byte addresses[][6] = {"1Node","2Node"};  


    radio.openWritingPipe(addresses[0]); // transmitt
    radio.openReadingPipe(1,addresses[1]);

// printf_begin(); // uncomment to make the radio.printDetails work
Serial.println("starting");
  
  radio.startListening();                       // Start listening  
  
  radio.writeAckPayload(1,&counter,1);          // Pre-load an ack-paylod into the FIFO buffer for pipe 1
  radio.printDetails();
  delay(2000);   //sensor needs time
}

void loop(void) {


    byte gotByte;                                           // Initialize a variable for the incoming response
    
    radio.stopListening();                                  // First, stop listening so we can talk.      


      // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
 // float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
 /* if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  } */
  

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
    
    //Serial.println(counter);
        // READ DATA
  // Compute heat index in Fahrenheit (the default)
  //float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humidity:");
  Serial.print(h);
  Serial.print("%\t");
  Serial.print(" Temperature:");
  Serial.print(t);
  Serial.print("*C ");
  //Serial.print(f);
  //Serial.print(" *F\t");
  Serial.print(" Heat index:");
  Serial.print(hic);
  Serial.println("*C ");
  //Serial.print(hif);
  //Serial.println(" *F");


gt = t;
gh = h;

    for ( i = 0; i < 5; i++ ) {      
        message[i]= gt[i];
    }
  message[i] = ','; 
      for ( j = 0; j < 5; j++ ) {
        i++;
        message[i]= gh[j];
    }
  message[i+1] = '\0';




/**

sprintf(gtemp,"%4.2f",t);
sprintf(ghum,"%4.2f",h);
//sprintf(gtemp,"%f",t);
//sprintf(ghum,"%f",h);

Serial.println(gtemp);
Serial.println(ghum);

    for ( i = 0; i < 5; i++ ) {      
        message[i]= gtemp[i];
    }
  message[i] = ','; 
      for ( j = 0; j < 5; j++ ) {
        i++;
        message[i]= ghum[j];
    }
  message[i+1] = '\0';
*/


  //radio.printDetails();  // you have to uncomment #include printf.h and printf.begin to make this work
  
  //Serial.print(",\t");
  Serial.print(message);
  Serial.print(",\t");
  Serial.println(sizeof(message));
  
Serial.println(F("Now sending >>>>>>>>>>>>>>>>>>>>>>>>>> "));   //send temp 






   while(!send_ok ) {                                                 //try 20 times then fail
   if ( radio.write(&message,sizeof(message)) ){                         // Send the temperature to the other radio
  // if ( radio.write(&message,32) ){                         // Send the temperature to the other radio   
        if(!radio.available()){                             // If nothing in the buffer, we got an ack but it is blank
            Serial.print(F("Got blank response. round-trip delay: "));
            Serial.print(micros()-timer);
            Serial.println(F(" microseconds"));
              //radio.printDetails();     
        }else{      
            while(radio.available() ){                      // If an ack with payload was received
            
                radio.read( &gotByte, 1 );                  // Read it, and display the response time
                timer = micros();
                
                Serial.print(F("Got response "));
                Serial.print(gotByte);
                Serial.print(F(" round-trip delay: "));
                Serial.print(micros()-timer);
                Serial.println(F(" microseconds"));
                counter++;                                  // Increment the counter variable
                //delay(3000);
            }
        } //else
        send_ok = 1;  
    }else{        Serial.println(F("Sending failed.")); 
      retry_count++;
      delay(5000);  // wait and then try again
      Serial.println(retry_count); 
      if(retry_count>MAX_RETRY_COUNT) {
        send_ok = 1; // lets move on, the receiver is not available       
      }
    
    }          // If no ack response, sending failed
   }
    
    //reset for next send time
    send_ok = 0;
    retry_count = 0;
    
    delay(30000);  // Try again later
//delay(1000);  // Try again later
    //}// for loop

 
}
