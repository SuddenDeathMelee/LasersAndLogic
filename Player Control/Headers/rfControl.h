#include "VirtualWire.h"

//RF Device Variables//
const int RF_input = 8; //Must be a PWM pin - RF Receiver Digital Output
const int RF_output = 9;	//RF Transmitter Digital input
const int RF_trigger = 7;	//Normally GND, Power to trigger
const int item = A0;	//Analog input for item selection (currently selects one of three items)

void rfSetup()
{
  vw_set_ptt_inverted(true); // Required for DR3100
  vw_setup(2500);	// Bits per sec
  vw_set_tx_pin(RF_output);
  vw_set_rx_pin(RF_input);
  vw_rx_start(); // Start the receiver PLL running
}

void RF_fire(){
  int index = (analogRead(item)/341)%3; //Grab item number from analog input pin
  index = index%3; //Calculate item index based on raw analog value
  int shot = items[index];
  vw_send((uint8_t *)(&shot), 2);//Send AoE, vw_send(data,#bytes)
  vw_wait_tx(); // Wait until the whole message is gone
  while(digitalRead(RF_trigger)==HIGH);
}

bool rfGetMessage(int& code)
{
	if (vw_have_message())
	{
		uint8_t buf[VW_MAX_MESSAGE_LEN];					//Initialize buffer for message
		uint8_t buflen = VW_MAX_MESSAGE_LEN;				//Initialize variable to store code length
		if(vw_get_message(buf, &buflen))
		{													//If the data is not corrupted
		  int code = 0x0000;
		  for (int i = buflen-1; i > -1; i--)
		  {
			code = code+buf[i]<<(i*8);
		  }
		  return true;
		}
	}
	return false;
}