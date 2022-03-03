Driving the Tapestry with Bluetooth LE
=============

This instruction set should be common regardless of the connection method.

BLE has limitations on the default MTU.  I have chosen to stick to these restrictions and only a byte array of length 20 can be sent.

All <> denote a byte.

Stars or Planets
------------

Connect to Characteristic UUID: f82be6f8-3b05-11ec-8d3d-0242ac130003
write command:
	<deviceID>s<red><green><blue><star ID>

Constellations
------------

Connect to Characteristic UUID: f82be6f8-3b05-11ec-8d3d-0242ac130003
write command:
	<deviceID>c<red><green><blue><constellation ID>


Animation
------------

Loading in an animation.

Connect to Characteristic UUID: 6f485ef4-4d28-11ec-81d3-0242ac130003
Write command:
	<deviceID>a
wait 10s
Read the characteristic values.  It will be in the form:
	<deviceID><animation id>
Check deviceID matches current device.

Connect to Characteristic UUID: f82be6f8-3b05-11ec-8d3d-0242ac130003
Write command:
	<deviceID>a<red><green><blue><animation id><high byte><low byte><delay ms>
n.b. <high byte><low byte><delay ms> can be repeated until you reach the 20 character limit.
running this command multiple times will append to the animation.  you can change colours between each iteration.

Connect to Characteristic UUID: f82be6f8-3b05-11ec-8d3d-0242ac130003
Write command:
	<deviceID>A<animation id>
This will play the appropriate animation.

Clearing an animation.
Connect to Characteristic UUID: f82be6f8-3b05-11ec-8d3d-0242ac130003
Write command:
	<deviceID>D<animation id>
This will reset the appropriate animation.

