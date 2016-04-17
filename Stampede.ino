
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
#include "Schedule.h"
#include "Clock.h"

#define SLAVE_ADDRESS 0x12

enum Buttons
{
	THROTTLE,
	STEERING,
};

byte pins[REMOTE_NB_CHANNELS];
uint8_t messageReceived[COM_IN_LENGTH];
Stampede stampede;
Remote remote;
Schedule schedSensors(500);
Schedule schedMotors(500);
Clock loopTime(50);
Clock chronoMotors;
Clock chronoSensors;

void displayTime(const char* name, unsigned long theTime)
{
	Serial.print(" Time of ");
	Serial.print(name);
	Serial.print(": ");
	Serial.print(theTime); 
	Serial.println(" us");
}	

void setup() 
{  
	stampede.begin();
	i2cBegin(SLAVE_ADDRESS);
	delay(1000); // wait for ESC

	pins[THROTTLE] = REMOTE_THROTTLE_PIN;
	pins[STEERING] = REMOTE_STEERING_PIN;
	remote.begin(pins, REMOTE_NB_CHANNELS);

	Serial.begin(19200); // start serial port
	Serial.println("Stampede is Ready! ");

	loopTime.start();
}

void loop() 
{
	int motorSpeed = 0;
	int motorSteering = 0;

	chronoSensors.start();
	if( schedSensors.isItTime() )
	{
		schedSensors.call();
		//read sensors
		uint8_t sensors[2];
		sensors[0] = batteryVoltage(BATTERY, BATTERY_LED);
		Serial.print("BatteryVoltage: \t\t\t");
		Serial.println(sensors[0]);
		sensors[1] = photoCell(LUMINOSITY);	

		//fill message_to_send
		i2cSetMessage(sensors);
	}
	chronoSensors.stop();

	chronoMotors.start();
	if( schedMotors.isItTime() )
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

	//displayTime("Sensors", chronoSensors.elapsedTime());
	//displayTime("Motors", chronoMotors.elapsedTime());

	//wait Loop Time
	// if time permits, check if remote is connected
	if( loopTime.elapsedTime() < 30000 )
	{
		remote.checkIfConnected();
	}
	loopTime.wait();
 
}