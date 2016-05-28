
/*
  Stampede.ino - Controller for Traxxas Stampede.
  Created by Louis SR, November 26, 2015.
  Released into the public domain.
*/

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
	SWITCH,
};

bool debug; //true if Serial is connected then send message through Serial 
byte pins[REMOTE_NB_CHANNELS];
byte messageReceived[COM_IN_LENGTH];
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
			Serial.println("Debug mode ");
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
	pins[SWITCH] = REMOTE_SWITCH_PIN;
	remote.begin(pins, REMOTE_NB_CHANNELS, false);
	remote.setSwitch(SWITCH);
	
	//Timing
	ClockSensors.begin(1000);
	ClockMotors.begin(1000);
	ClockLoop.begin(100);
}

void loop() 
{
	int motorSpeed = 0;
	int motorSteering = 0;
	int remoteSpeed = 0;
	int remoteSteering = 0;
	byte remoteSwitch = 0;

	chronoSensors.start();
	if( ClockSensors.isItTime() )
	{
		//read sensors
		byte messageToSend[8];
		byte index = 0;
		messageToSend[index++] = batteryVoltage(BATTERY, BATTERY_LED);
		messageToSend[index++] = photoCell(LUMINOSITY);
		messageToSend[index++] = remoteSpeed;
		messageToSend[index++] = remoteSteering;
		messageToSend[index++] = remoteSwitch;
		messageToSend[index++] = motorSpeed;
		messageToSend[index++] = motorSteering;
		messageToSend[index++] = stampede.getState();

		//fill message to send
		i2cSetMessage(messageToSend);
	}
	chronoSensors.stop();
	chronoMotors.start();
	if( ClockMotors.isItTime() )
	{
		if( remote.isConnected() )
		{
			remoteSpeed = map(remote.read(THROTTLE), -500, 500, -100, 100);
			remoteSteering = map(remote.read(STEERING), -500, 500, -100, 100);
			remoteSwitch = remote.read(SWITCH);
		}
		else
		// if no RC communication, then stop
		{
			if(debug)
			{
				Serial.println("No Remote");
			}
			remoteSpeed = 0;
			remoteSteering = 0;
			remoteSwitch = 0;
		}

		if(remoteSwitch != 0)
		// remoteSwitch == 0: remote only
		// remoteSwitch == 1: remote with IMU correction
		// remoteSwitch == 2: autonomous mode
		{
			//check for new messages
			if( i2cGetMessage(messageReceived) )
			{
				//messageReceived is unsigned
				motorSpeed = int(messageReceived[0]) - 100;
				motorSteering = int(messageReceived[1]) - 100;
			}

		}
		else
		{
			motorSpeed = remoteSpeed;
			motorSteering = remoteSteering;
		}

		//update command to motors
		if(debug)
		{
		// 	printSigned("Remote Speed", remoteSpeed);
		// 	printSigned("Motor Speed", motorSpeed);
		 	printSigned("Remote Steer", remoteSteering);
			printSigned("Motor Steer", motorSteering);
		}
		stampede.setSteer(motorSteering);
		stampede.setSpeed(motorSpeed);
		stampede.update();
	}
	chronoMotors.stop();

	if(debug)
	{
		//printUnsigned("Sensors", chronoSensors.elapsedTime() );
		//printUnsigned("Motors", chronoMotors.elapsedTime() );
	}
	//wait Loop Time
	// if time permits, check if remote is connected
	if( ClockLoop.elapsedTime() < 10000 )
	{
		remote.checkIfConnected();
	}
	
	ClockLoop.wait();

}