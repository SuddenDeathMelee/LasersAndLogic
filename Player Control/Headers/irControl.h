#include "IRremote.h"

//IR Device Variables//
const int IR_input = 2; //Must be a PWM pin - IR Receiver Data Output
const int IR_output = 3;	//Anode of IR LED
const int IR_trigger = 12;	//Normally GND, Power to trigger
IRrecv irrecv(IR_input); //create an IRrecv object
decode_results decodedSignal; //Stores results from IR detector
IRsend irsend;

void irSetup()
{
 irrecv.enableIRIn(); //Begin the receiving process. This will enable the timer interrupt which consumes a small amount of CPU every 50 µs.
}

void IR_fire(){
  int shot = 0xA000; //Has Header A, carrier code 0x0
  shot = shot+(Team<<11); //include Team # in shot code
  shot = shot+Stats[1]; //add Attack stat
  byte oldTCCR1A = TCCR1A;
  byte oldTCCR1B = TCCR1B;
  TCCR1A = 0;
  TCCR1B = 0;
  irsend.sendSony(shot,16); //Send attack, sendSony(data,#bits)
  TCCR1A = oldTCCR1A;
  TCCR1B = oldTCCR1B;
  while(digitalRead(IR_trigger)==HIGH);
}
