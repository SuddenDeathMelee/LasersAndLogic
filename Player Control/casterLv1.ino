/*
PlayerControl_Caster.ino
Amelia Peterson
10/06/13

This code manages the I/O for the player's arduino (IR and RF transmitters and receivers, triggers, item selection),
damage, status conditions, and items.

Notes:
Ensure that VirtualWire and IRremote do not use the same timers. The IRremote timer can be edited in IRremoteInt.h
(make sure you change the pins after you change the timer). I have VirtualWire using Timer 1 and IRremote using Timer 2.

*/
#include <VirtualWire.h>
#include <IRremote.h>
#include "Headers/Status.h"

typedef void (*funcptr)(byte carrier, byte value);

//Accelerometer Definitions
#define ADC_AMPLITUDE 1024							//amplitude of the 10bit-ADC of Arduino is 1024LSB
#define ADC_REF 5									//ADC reference is 5v
#define ZERO_X 1.22									//accleration of X-AXIS is 0g, the voltage of X-AXIS is 1.22v
#define ZERO_Y 1.22									//
#define ZERO_Z 1.25									//
#define SENSITIVITY 0.25							//sensitivity of X/Y/Z axis is 0.25v/g

//RF Device Variables//
const int RF_input = 8;								//Must be a PWM pin - RF Receiver Digital Output
const int RF_output = 9;							//RF Transmitter Digital input
const int RF_trigger = 7;							//Normally GND, Power to trigger
const int X = A0;									//X input from ADXL335
const int Y = A1;									//Y input from ADXL335
const int Z = A2;									//Z input from ADXL335
const int item = A3;								//Analog input for item selection (currently selects one of three items)
const int spell = A4;								//Analog input for spell selection
//IR Device Variables//
const int IR_input = 2;								//Must be a PWM pin - IR Receiver Data Output
const int IR_output = 3;							//Anode of IR LED
const int IR_trigger = 12;							//Normally GND, Power to trigger
IRrecv irrecv(IR_input);							//create an IRrecv object
decode_results decodedSignal;						//Stores results from IR detector
IRsend irsend;
//Lights and Sound
const int RED = 4;	//Red LED for Damage
const int GRN = 5;	//Green LED for Heal
const int BLU = 6;	//Blue LED for Status condition
//Control Variables//
unsigned char Stats[2] = {100, 5}; //Health, Attack
int items[3] = {0xA50A, 0xA08A, 0xA104};//Magic Missile, Mass Heal, 30 MASSIVE
int spells[3] = {0xA60A, 0xA00A, 0xA81E);
unsigned char Status_timer = 0;

funcptr carriers[8] = {Normal, Timed, Buf, Clear, Massive, Special, Element, Disable};//Function pointers for each carrier code 0x0-0x7
bool statcon = 0; //True if Player has any status conditions
bool Team = 0; //0 or 1

void setup(){
  //LEDs and Buzzer
  pinMode(RED, OUTPUT);
  pinMode(GRN, OUTPUT);
  pinMode(BLU, OUTPUT);
  //Power Up lights
  digitalWrite(RED, HIGH);
  delay(200);
  digitalWrite(GRN, HIGH);
  delay(200);
  digitalWrite(BLU, HIGH);
  delay(200);
  digitalWrite(RED, LOW);
  digitalWrite(GRN, LOW);
  digitalWrite(BLU, LOW);
  vw_set_ptt_inverted(true); // Required for DR3100
  vw_setup(2500);	// Bits per sec
  vw_set_tx_pin(RF_output);
  vw_set_rx_pin(RF_input);
  vw_rx_start(); // Start the receiver PLL running
  //IR Setup//
  irrecv.enableIRIn(); //Begin the receiving process. This will enable the timer interrupt which consumes a small amount of CPU every 50 Âµs.
  //Initilize Variables
  Status temp = {0, 0, 0, 0xFF, 0};
  for(int i=0; i<6; i++){
current[i] = temp;
  }
  // println("Setup complete");
}

void getXYZ(int16_t *x,int16_t *y,int16_t *z){
  *x = analogRead(X_AXIS_PIN);
  *y= analogRead(Y_AXIS_PIN);
  *z = analogRead(Z_AXIS_PIN);
}

void getAcceleration(float *ax,float *ay,float *az){
  int x,y,z;
  float xvoltage,yvoltage,zvoltage;
  getXYZ(&x,&y,&z);
  xvoltage = (float)x*ADC_REF/ADC_AMPLITUDE;
  yvoltage = (float)y*ADC_REF/ADC_AMPLITUDE;
  zvoltage = (float)z*ADC_REF/ADC_AMPLITUDE;
}

signed int CalculateCast(){
  float acc[3];
  stats[2] = 0;
  signed int spell_select;
  float spell_read, multiplier;
  spell_select = AnalogRead(spell);
  for(int i=0; i<10; i++){
    if(!RF_Trigger){
      break;
    }
    getAcceleration(&acc[0],&acc[1],&acc[2]);
    spell_read = AnalogRead(spell);
    multiplier = abs(spell_select-(float)spell_read)/spell_select
    spell_read = spell_read%3;
    stats[2] = (char)(damage+acc[spell_read]*multiplier);
    delay(100);
  }
  return spell_select;
}

void RF_fire(){
  digitalWrite(BLU,1);
  int cast = spells(CalculateCast());
  cast = cast + stats[2];
  vw_send((uint8_t *)(&cast), 2);//Send AoE, vw_send(data,#bytes)
  vw_wait_tx(); // Wait until the whole message is gone
  digitalWrite(BLU,0);
}

void Get_Damage(){
if(irrecv.decode(&decodedSignal)==true){ //If IR signal has been received...
    Serial.print("Received IR signal: ");
    Serial.println(decodedSignal.value, HEX);
    Serial.println(decodedSignal.rawlen);
    if(decodedSignal.rawlen==34){ //If IR signal is not 16 bits...
      byte data[3]; //data = {Header, Carrier, Value byte}
      parse(decodedSignal.value, data); //Parse code into Header, Team/Carrier, Value byte
      if(data[0]==0xA && data[1]<6){ //If Header is equal to 0xA and carruer value exists...
        carriers[data[1]](data[1],data[2]); //Call function corresponding to carrier
      }
    }
    irrecv.resume(); // Receive the next value
  }
  if (vw_have_message()){ //If RF signal has been received...
    Serial.print("Received RF signal: ");
    uint8_t buf[VW_MAX_MESSAGE_LEN]; //Initialize buffer for message
    uint8_t buflen = VW_MAX_MESSAGE_LEN; //Initialize variable to store code length
    if (vw_get_message(buf, &buflen)){ //If the data is not corrupted
      int code = 0x0000;
      for (int i = buflen-1; i > -1; i--){
        code = code+buf[i]<<(i*8);
Serial.print(code,HEX);
      }
      Serial.println();
      if(buflen!=16){ //If the message length is not equal to 16
       byte data[3]; //data = {Header, Team/Carrier, Value byte}
       parse(code,data); //Parse code into Header, Carrier, Value byte
       if(data[0]==0xA && data[1]<7){ //If Header is equal to 0xA and carrier value exists...
         Serial.println("Header correct and code not sent from Team member...");
         carriers[data[1]&0x7](data[1],data[2]); //Call function corresponding to carrier
}
      }
    }
  }
}

void parse(long unsigned int signal, byte* data){
  //parse into header, carrier, damage
  data[0] = (signal>>12); //Header
  data[1] = (signal>>8)&(0x0F); //Team, Carrier
  data[2] = signal; //Level/Value
}
void Normal(byte carrier, byte value){
  bool sending_Team = carrier>>7;
  bool bit_sign = value>>7;
  if( ((bit_sign)&&(sending_Team==Team)) || ((!bit_sign)&&(sending_Team!=Team)) ){ //If healing and sent by team member, or if damage and not sent by team member
    if(!bit_sign){
      digitalWrite(RED, 1);
    }
    else{
      digitalWrite(GRN, 1);
    }
    byte sign = 2*(bit_sign);	//0 or 2 -> sign of value will be (-1+sign) = -1 or 1
    value = value&0x7F; //Grab value of code (0-127)
    Stats[0] = Stats[0] + (-1+sign)*value; //Health = Health + (-1+sign)(Value) - Add or Subtract Health
    Serial.print("Health: ");
    Serial.println(Stats[0], DEC);
    if(!bit_sign){
      digitalWrite(RED, 0);
    }
    else{
      digitalWrite(GRN, 0);
    }
  }
}
void popStatus(char index){
  Status temp = {0,0,0,0xFF,0};
  current[index] = temp;
}
void Timed(byte carrier, byte value){
  Status stat;
  bool sending_Team = carrier>>7;
  stat.b_db = (value>>4)&0x01;
  if( ((stat.b_db)&&(sending_Team==Team)) || ((!stat.b_db)&&(sending_Team!=Team)) ){ //If healing and sent by team member, or if damage and not sent by team member
    digitalWrite(BLU, 1);
    stat.index = value>>5;
    stat.level = (value&0x0F);
    stat.num_updates = pow(2,stat.level);
    stat.reset_val = Stats[stat.index];
    pushStat(stat);
    statcon = 1;
    digitalWrite(BLU, 0);
  }
}
void Buf(byte carrier, byte value){
}
void Clear(byte carrier, byte value){
  digitalWrite(GRN, 1);
  Status temp;	//Create empty Status
  temp.num_updates = 0xFF;
  for(int i = 3; i<6; i++){
    current[i] = temp;	//Clear all status conditions
  }
  digitalWrite(GRN, 0);
}
void Massive(byte carrier, byte value){
  digitalWrite(RED, 1);
  Stats[0] = Stats[0] - value;	//Health = Health - Value
  digitalWrite(RED, 0);
}
void Special(byte carrier, byte value){
  digitalWrite(BLU,1);
  delay(1000);
  digitalWrite(BLU,0);
}
void Element(byte carrier, byte value){
  
}
void Disable(byte carrier, byte value){

}
void StatusConditions(){
  digitalWrite(BLU, HIGH);
  char empty_status = 0;
  for(char i=0; i<6; i++){	//Go through all status conditions and accumulate results
    if(current[i].num_updates==0xFF){ //If current[i] does not hold a status, increment empty_status counter and continue to next index
      empty_status++;
      continue;
    }
    if(current[i].num_updates==0){	//If num_updates==0, Status complete
      empty_status++;
      if(current[i].index) //If not damage status...
        Stats[current[i].index] = current[i].reset_val; //reset value
      popStatus(i); //Remove Status
      continue;
    }
    Stats[current[i].index] = Stats[current[i].index]-(1-2*(current[i].b_db))*current[i].level;	//Accumulate Effects
    current[i].level = (!(0x1&&current[i].index))*(current[i].level);
    current[i].num_updates = current[i].num_updates-1;
  }
  if(empty_status==6){
    statcon = 0;
  }
  digitalWrite(BLU,LOW);
}
void loop(){
//Currently, the RF and IR Trigger pins are continuously
  //polled, but later on they will be implemented with
  //interrupts.
  Serial.println("polling");
  if(digitalRead(RF_trigger)==HIGH){ //If RF trigger pulled...
    Serial.println("RF fire");
    RF_fire();
  }
  Get_Damage(); //Detect if IR or RF signal was received
  
  Status_timer++;
  if(statcon && Status_timer==0x00001000){ //Accumulate damage from Status Conditions
    StatusConditions();
    Status_timer=0;
  }
}
