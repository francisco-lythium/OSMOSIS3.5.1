#include <Wire.h>
const int sensorPin = 3;
const int measureInterval = 500;
volatile int pulseConter;
int c;
byte flujo;
// YF-S201
//const float factorK = 75;
 
// FS300A
const float factorK = 8.5;
 
// FS400A
//const float factorK = 3.5;
 
 
 
void setup()
{
Wire.begin(9);                // join i2c bus with address #8
Wire.onReceive(receiveEvent); // register event
Wire.onRequest(requestEvent); // register event

   pinMode(LED_BUILTIN, OUTPUT);
   Serial.begin(9600);
   attachInterrupt(digitalPinToInterrupt(sensorPin), ISRCountPulse, RISING);
}
 
void loop()
{
   // obtener frecuencia en Hz
   float frequency = GetFrequency();
 
   // calcular caudal L/min
   float flow_Lmin = (frequency / factorK);
   flujo = 10*(frequency / factorK);
  
  // Serial.print("Frecuencia: ");
  // Serial.print(frequency, 0);
   Serial.print(" (Hz)\tCaudal: ");
   Serial.print(flow_Lmin, 3);
   Serial.println(" (L/min)");
   Serial.print(" y enviado: ");
   Serial.println(flujo);
   
}

void ISRCountPulse()
{
   pulseConter++;
   digitalWrite(LED_BUILTIN, HIGH);
   delayMicroseconds(100);
   digitalWrite(LED_BUILTIN, LOW);
   

   
}
 
float GetFrequency()
{
   pulseConter = 0;
 
   interrupts();
   delay(measureInterval);
   noInterrupts();
 
   return (float)pulseConter * 1000 / measureInterval;
}

void receiveEvent(int howMany) {
 
// c = Wire.read(); // receive byte as a character
//Serial.println(c);         // print the character
  
//if(c==1)
//{Serial.println("recibi uno");
// Wire.beginTransmission(9); 
// Wire.begin();
 Wire.write(flujo);
 Serial.println(flujo);
 //Wire.endTransmission();
//}
//if(c==0)
//{Serial.println("recibi cero");}

  //int x = Wire.read();    // receive byte as an integer
  //Serial.println(x);         // print the integer
}
void requestEvent() 
{Wire.write(flujo);
}



