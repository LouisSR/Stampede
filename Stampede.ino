
/*
  Stampede.ino - Controller for Traxxas Stampede.
  Created by Louis SR, November 26, 2015.
  Released into the public domain.
*/

#include <Servo.h>
#include <Wire.h>
#include "Stampede.h"
#include "Remote.h"
#include "Utils.h"
#include "I2C.h"
#include "Clock.h"

#define SLAVE_ADDRESS 0x12

enum Buttons
{
	THROTTLE,
	STEERING,
};

bool debug; //true if Serial is connected then send message through Serial 
byte pins[REMOTE_NB_CHANNELS];
uint8_t messageReceived[COM_IN_LENGTH];
Stampede stampede;
Remote remote;
Clock ClockSensors;
Clock ClockMotors;
Clock ClockLoop;
Clock chronoMotors;
Clock chronoSensors;

bool checkSerial(unsigned long baudrate)
{

	bool toggle = HIGH;
	byte timeout = 10;
	Serial.begin(baudrate); // start serial port
	while(timeout)
	{
		if(Serial)
		{
			return true;
		}
		else
		{
			timeout--;
			digitalWrite(LED_BUILTIN, toggle);
			toggle = !toggle;
			delay(400);
		}
	}
	Serial.end();
	return false;
}

void setup() 
{  
	//Serial communication with computer
	debug = checkSerial(115200); //Blink led 5 times while ckecking if Serial is opened
	
	stampede.begin(debug);
	i2cBegin(SLAVE_ADDRESS);
	delay(1000); // wait for ESC

	//Remote
	pins[THROTTLE] = REMOTE_THROTTLE_PIN;
	pins[STEERING] = REMOTE_STEERING_PIN;
	remote.begin(pins, REMOTE_NB_CHANNELS);

	//Timing
	ClockSensors.begin(500);
	ClockMotors.begin(500);
	ClockLoop.begin(50);
}

void loop() 
{
	int motorSpeed = 0;
	int motorSteering = 0;

	chronoSensors.start();
	if( ClockSensors.isItTime() )
	{
		//read sensors
		uint8_t sensors[2];
		sensors[0] = batteryVoltage(BATTERY, BATTERY_LED);
		sensors[1] = photoCell(LUMINOSITY);	

		//fill message_to_send
		i2cSetMessage(sensors);
	}
	chronoSensors.stop();

	chronoMotors.start();
	if( ClockMotors.isItTime() )
	{
		if( remote.isConnected() )
		{
			motorSpeed = remote.read(THROTTLE);
			motorSteering = remote.read(STEERING);
		}
		else
		{
			//check for new messages
			i2cGetMessage(messageReceived);
		}

		//update command to motors
		stampede.setSteer(motorSteering);
		stampede.setSpeed(motorSpeed);
		stampede.update();
	}
	chronoMotors.stop();

	if(debug)
	{
		printUnsigned("Sensors", chronoSensors.elapsedTime() );
		printFloat("Test float", 3.24565);
		printSigned("Text long", -4525235);
	}

	//wait Loop Time
	// if time permits, check if remote is connected
	if( ClockLoop.elapsedTime() < 30000 )
	{
		remote.checkIfConnected();
	}
	ClockLoop.wait();
 
}