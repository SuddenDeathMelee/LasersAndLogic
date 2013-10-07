/*
STracker.ino
Amelia Peterson
2/12/13

Motion Tracker Turret, Simple Design
This code controls a turret which tracks and shoots at any motion in the
environment. The turret has one motor which is controlled by pins 0,1, and 11 (PWM)
connected to an H-bridge, 3 ultrasonic rangers for collecting environment data,
a rotary 10k pot with +5V across it and input to the A0 pin for positioning data,
and an IR transmitter and receiver for dealing and receiving damage.

The turret only takes normal damage (codes beggining with 0xA0). After the turret's
health has been depleted, there is a minute delay, then it restarts with full health.
*/
#include IRremote.h

//Motor Control//
const int Motor_A = 0; //Connected to pin 3A on the H-bridge
const int Motor_B = 1;	//Connected to pin 4A on the H-bridge
const int Motor_EN = 11;	//Connected to M_EN (motor enable) on the H-bridge

//Range finder data lines//
const int Ranger1 = 3;	//Connected to Ranger 1 Data Line
const int Ranger2 = 4;	//Connected to Ranger 2 Data Line
const int Ranger3 = 5;	//Connected to Ranger 3 Data Line

//IR Control//
const int IR_input = 3; //Must be a PWM pin - IR Receiver Data Output
const int IR_output = 2;	//Anode of IR LED

//Positioning information//
int POT = A0;	//Connected to position potentiometer

//VARIABLES//
float position;	//The position of the mount is determined by the
// input from a potentiometer which rotates with
// the mount.
signed char Health = 50;
bool Team = 0;

void setup(){
//Motor Setup//
pinMode(Motor_A, OUTPUT);
digitalWrite(Motor_A, HIGH);
pinMode(Motor_B, OUTPUT);
digitalWrite(Motor_B,LOW);
pinMode(Motor_EN, OUTPUT);
digitalWrite(Motor_EN, LOW);

//IR Setup//
pinMode(IR_input, OUTPUT); //set pin 3 to IR input
   irrecv.enableIRIn(); //Begin the receiving process. This will enable the timer interrupt which consumes a small amount of CPU every 50 Âµs.
}

ISR(PCINT2_vect){ //InterruptService-Routine, called by the Interruptvektor PCINT_vect
while(pin==1); //debouncing
Health--;
if(Health ==0)
delay(60000);	
}

void ping(int ranger){
// The PING is triggered by a HIGH pulse of 2 or more microseconds.
// Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
pinMode(ranger,OUTPUT);
digitalWrite(ranger, LOW);
delayMicroseconds(2);
digitalWrite(ranger, HIGH);
delayMicroseconds(5);
digitalWrite(ranger, LOW);
// The same pin is used to read the signal from the PING))): a HIGH
// pulse whose duration is the time (in microseconds) from the sending
// of the ping to the reception of its echo off of an object.
pinMode(ranger, INPUT);
duration[ranger-3] = pulseIn(ranger, HIGH);
// convert the time into a distance
inches[ranger-3] = microsecondsToInches(duration[ranger-3]);
cm[ranger-3] = microsecondsToCentimeters(duration[ranger-3]);

Serial.print(inches[ranger-3]);
Serial.print("in, ");
Serial.print(cm[ranger-3]);
Serial.print("cm");
Serial.println();
}

long microsecondsToInches(long microseconds)
{
// According to Parallax's datasheet for the PING))), there are
// 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
// second). This gives the distance travelled by the ping, outbound
// and return, so we divide by 2 to get the distance of the obstacle.
// See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
return microseconds / 74 / 2;
}
 
long microsecondsToCentimeters(long microseconds)
{
// The speed of sound is 340 m/s or 29 microseconds per centimeter.
// The ping travels out and back, so to find the distance of the
// object we take half of the distance travelled.
return microseconds / 29 / 2;
}

void move(){
if(position==0.0){
digitalWrite(Motor_A, LOW);
digitalWrite(Motor_B, HIGH);
}
if(position==5.0){
digitalWrite(Motor_A, HIGH);
digitalWrite(Motor_B, LOW);
}
digitalWrite(Motor_EN, HIGH);
delay(1);
digitalWrite(Motor_EN, LOW);
}

void get_position(){
int sensorValue = analogRead(POT);
position = sensorValue * (5.0/1023.0);
}

void fire(){
long shot = 0xA005;
shot = shot+(Team<<11);
shot = shot+Stats[1];
irsend.sendSony(shot,16);
}

void track(){
boolean MEN, M2A, M1A;
char counter = 1;
MEN = 1;
MA = 1;
MB = 1;
while(!(M1A==0 && M2A==0 && MEN==0)){
if(!(counter%6)){
fire();
counter = 1;
}
MEN = (0x00 && ping(1));	//Will be either 0 or 1
MA = (0x00 && ping(0));
MB = (0x00 && ping(2));

digitalWrite(Motor_EN, !MEN);
digitalWrite(Motor_A, MA);
digitalWrite(Motor_B, MB);

counter++;
}
digitalWrite(Motor_EN, LOW);
digitalWrite(Motor_A, 1);
digitalWrite(Motor_B, 0);
}

void get_damage(){
if(irrecv.decode(&decodedSignal)==true){
     if(decodedSignal.rawlen==16){
       char data[3];
       parse(decodedSignal.value, data);
       if(data[0]=(0xA0+Team<<11)){	//If not normal damage...
         health = health - (data[2]<<1);
       }
     }
  }
if(health<1){
delay(60000);	//Disable for a minute
health = 50;	//Revive
}
}

void parse(long unsigned int signal, char* data){
  //parse into header, carrier, damage
  data[0] = signal>>12; //Header
  data[1] = signal>>8; //Team, Carrier
  data[2] = signal; //Level
}

void loop(){
get_position();
move();
track();
get_damage();
}
