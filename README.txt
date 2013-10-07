Lasers and Logic - A Sci-Fi LARP

The code provided here is for establishing a laser tag system to be used in a science fiction based LARP (Live Action Role Play) Game.
The code is meant to handle I/O from RF and IR devices, calculate damage, and store items that the players can use. For more information
about the game and schematics for devices, go to our site at lasersandlogic.weebly.com.

NOTES: The Lasers and Logic code base uses the arduino libraries IRremote, VirtualWire, and LiquidCrystal. The files for those can be 
found at https://github.com/shirriff/Arduino-IRremote, http://www.pjrc.com/teensy/td_libs_VirtualWire.html, and LiquidCrystal comes with 
the Arduino IDE. These libraries have also been included under "libraries" of this git. Make sure you set IRremote to use Timer 2 on the 
arduino, otherwise VirtualWire and IRremote will have conflicts.

FILES:
Player Control 
	These files are for the different classes and levels of character you can have in the game. They are the main code to be uploaded
to the arduino for handling each player's gun damage, items, AoE (Area of Effect) weapons, etc.

Devices
	Devices currently consist of a motion tracking turret and an environment trap.
