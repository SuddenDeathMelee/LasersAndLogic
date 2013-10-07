/*
PlayerControl.ino
Amelia Peterson
10/05/13

This code manages the I/O for the player's arduino (IR and RF transmitters and receivers, triggers, item selection),
damage, status conditions, and items.

Notes:
Ensure that VirtualWire and IRremote do not use the same timers. The IRremote timer can be edited in IRremoteInt.h
(make sure you change the pins after you change the timer). I have VirtualWire using Timer 1 and IRremote using Timer 2.

*/
/* 
Team
Attack
Health
Defense
Items
Number items
Number Statuses
*/
#include "Headers/rfControl.h"
#include "Headers/irControl.h"
#include "Headers/Status.h"
#include <LiquidCrystal.h>

typedef void (*funcptr)(byte carrier, byte value);

//Display
LiquidCrystal lcd(4,5,6,10,11,13);

//Control Variables//
unsigned char Stats[3] = 
{	100, 
	5,
	201};							//Health, Blaster Attack, Shield

int items[2] = 
{	0xA00F,
	0xA00F};					//2 Heals

unsigned char Status_timer = 0;

funcptr carriers[8] = 
{	Normal, 
	Timed, 
	Buf, 
	Clear, 
	Massive, 
	Special, 
	Element, 
	Disable};					//Function pointers for each carrier code 0x0-0x7

bool statcon = 0;				//True if Player has any status conditions
bool Team = 0;					//0 or 1

void setup()
{
  //RF Setup//
  rfSetup();
  //IR Setup//
  irSetup();
  //Initilize Variables
  Status temp = {0, 0, 0, 0xFF, 0};
  for(int i=0; i<6; i++)
  {
	current[i] = temp;
  }
  
  //Display Setup
  lcd.begin(16, 2);
  lcd.setCursor(0,1);
  lcd.print(Stats[0]);
}

void Get_Damage()
{
  if(irrecv.decode(&decodedSignal)==true){				//If IR signal has been received...
    if(decodedSignal.rawlen==34)
	{													//If IR signal is not 16 bits...
      byte data[3];										//data = {Header, Carrier, Value byte}
      parse(decodedSignal.value, data);					//Parse code into Header, Team/Carrier, Value byte
      if(data[0]==0xA && data[1]<6){					//If Header is equal to 0xA and carruer value exists...
        carriers[data[1]](data[1],data[2]);				//Call function corresponding to carrier
      }
    }
    irrecv.resume();									// Receive the next value
  }
  //If RF signal has been received...
	int code = 0x0000;
	if(rfGetMessage(code))
	{													//If the data is not corrupted
		byte data[3];									//data = {Header, Team/Carrier, Value byte}
		parse(code,data);								//Parse code into Header, Carrier, Value byte
		if(data[0]==0xA && data[1]<7)
		{												//If Header is equal to 0xA and carrier value exists...
			carriers[data[1]&0x7](data[1],data[2]);		//Call function corresponding to carrier
		}
	}
  
}

void parse(long unsigned int signal, byte* data)
{
  //parse into header, carrier, damage
  data[0] = (signal>>12)&0x0F;							//Header
  data[1] = (signal>>8)&(0x0F);							//Team, Carrier
  data[2] = signal;										//Level/Value
}

void Normal(byte carrier, byte value)
{
  bool sending_Team = carrier>>7;
  bool bit_sign = value>>7;
  if( ((bit_sign)&&(sending_Team==Team)) 
	  || ((!bit_sign)&&(sending_Team!=Team)) )
  {														//If healing and sent by team member, or if damage and not sent by team member
    byte sign = 2*(bit_sign);							//0 or 2 -> sign of value will be (-1+sign) = -1 or 1
    value = value&0x7F;									//Grab value of code (0-127)
    Stats[0] = Stats[0] + (-1+sign)*value;				//Health = Health + (-1+sign)(Value) - Add or Subtract Health
    lcd.setCursor(1,1);
    lcd.print(" ");
    lcd.print(Stats[0]);
  }
}

void Timed(byte carrier, byte value)
{
  Status stat;
  bool sending_Team = carrier>>7;
  stat.b_db = (value>>4)&0x01;
  if( ((stat.b_db)&&(sending_Team==Team)) 
	  || ((!stat.b_db)&&(sending_Team!=Team)) )
  {													//If healing and sent by team member, or if damage and not sent by team member
    stat.index = value>>5;
    stat.level = (value&0x0F);
    stat.num_updates = pow(2,stat.level);
    stat.reset_val = Stats[stat.index];
    pushStat(stat);
    statcon = 1;
  }
}

void Buf(byte carrier, byte value)
{

}

void Clear(byte carrier, byte value)
{
  Status temp;											//Create empty Status
  temp.num_updates = 0xFF;
  for(int i = 3; i<6; i++)
  {
    current[i] = temp;									//Clear all status conditions (only debufs)
  }
}
void Massive(byte carrier, byte value)
{
  Stats[0] = Stats[0] - value;							//Health = Health - Value
}
void Special(byte carrier, byte value)
{
  delay(1000);
}
void Element(byte carrier, byte value)
{
  
}
void Disable(byte carrier, byte value)
{

}

void popStatus(char index)
{
  Status temp = {0,0,0,0xFF,0};
  current[index] = temp;
}


void StatusConditions()
{
  char empty_status = 0;
  for(char i=0; i<6; i++)
  {															//Go through all status conditions and accumulate results
    if(current[i].num_updates==0xFF)
	{														//If current[i] does not hold a status, increment empty_status counter and continue to next index
      empty_status++;
      continue;
    }
    if(current[i].num_updates==0)
	{														//If num_updates==0, Status complete
      empty_status++;
      if(current[i].index)									//If not damage status...
        Stats[current[i].index] = current[i].reset_val;		//reset value
      popStatus(i);											//Remove Status
      continue;
    }
    Stats[current[i].index] = Stats[current[i].index]-(1-2*(current[i].b_db))*current[i].level;	//Accumulate Effects
    current[i].level = (!(0x1&&current[i].index))*(current[i].level);
    current[i].num_updates = current[i].num_updates-1;
  }
  if(empty_status==6)
  {
    statcon = 0;
  }
}

void loop()
{
  if(digitalRead(RF_trigger)==HIGH)
  {															//If RF trigger pulled...
    RF_fire();
  }
  if(digitalRead(IR_trigger)==HIGH)
  {															//If IR trigger pulled...
    IR_fire();
  }
  Get_Damage();												//Detect if IR or RF signal was received
  
  Status_timer++;
  if(statcon && Status_timer==0x00001000)
  {															//Accumulate damage from Status Conditions
    StatusConditions();
    Status_timer=0;
  }
}