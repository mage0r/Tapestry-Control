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
	<deviceID>c<red><green><blue><constellation ID><highDELAY><lowDELAY>

Screensaver
------------
Connect to Characteristic UUID: f82be6f8-3b05-11ec-8d3d-0242ac130003

write command:
	<deviceID>S


Animation
------------

Loading in an animation.

Connect to Characteristic UUID: f82be6f8-3b05-11ec-8d3d-0242ac130003
Write command:
	<deviceID>n
wait 10s
Read the characteristic values.  It will be in the form:
	<deviceID><highSessionID><lowSessionID>
Check deviceID matches current device.

Connect to Characteristic UUID: f82be6f8-3b05-11ec-8d3d-0242ac130003
Write command:
	<deviceID>a<highAnimationID><lowAnimationID><red><green><blue><highLED><lowLED><highDELAY><lowDELAY><highWAIT><lowWAIT>
n.b. ><highLED><lowLED><highDELAY><lowDELAY><highWAIT><lowWAIT> can be repeated until you reach the 20 character limit.
running this command multiple times will append to the animation.  you can change colours between each iteration.


Connect to Characteristic UUID: f82be6f8-3b05-11ec-8d3d-0242ac130003
Write command:
	<deviceID>A<highAnimationID><lowAnimationID>
This will play the appropriate animation.

Clearing an animation.
Connect to Characteristic UUID: f82be6f8-3b05-11ec-8d3d-0242ac130003
Write command:
	<deviceID>D<highAnimationID><lowAnimationID>
This will reset the appropriate animation.

