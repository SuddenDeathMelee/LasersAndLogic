#define SAFETY_TIME 1000 //Time the connection must be solid to deliver payload
#define HAMMER_TIME 1000 //Additinal time between payloads

#include <VirtualWire.h>

void setup()
{
  pinMode(7,INPUT); //Mousetrap Input
  pinMode(13, OUTPUT); //Warning LED
  //Payload Fn
  vw_set_ptt_inverted(true);
  vw_setup(2500);
  vw_setup_tx_pin(9); //Set pin 9 to TX pin
  vw_rx_start();
}

void loop()
{
  if (digitalRead(7) == HIGH) //Trap is not set off
  {
    digitalWrite(13, LOW); //So keep warning LED OFF
  }
  else //If the trap is set off
  {
    digitalWrite(13, HIGH); //Turn Warning LED ON
    delay(SAFETY_TIME); //Wait 1 second
    if (digitalRead(7) == LOW) //If trap is not disarmed
    {
      digitalWrite(13, LOW);
      payload();
      delay(HAMMER_TIME);
      digitalWrite(13, HIGH);
    }
  }
}

void payload()
{
  int shot = 0xA50A; //5 Damage to all players
  vw_send((uint8_t *)(&shot), 2);//Send AoE, vw_send(data,#bytes)
  vw_wait_tx(); // Wait until the whole message is gone
}


