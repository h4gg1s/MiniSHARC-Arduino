/*
	MiniSHARC / Arduino integration via I2C.
	haggismonster @ https://www.minidsp.com/forum/

	This is open source with a "do what the fuck you want" license.
*/
#include <Wire.h>
#include "minisharc.h"

/*****************************************************************
 * This is the MiniSHARC class. Do NOT instantiate this.
 * You automatically get an instance of this Class called "Sharc".
 * See "loop()", below for more usage.
 *****************************************************************/

MiniSHARC::MiniSHARC() {
	resetToDefaults();
	if(!Sharc.isCallbackRegistered()) {
		Wire.onReceive([&Sharc](int i) { return Sharc.I2CReceiveCallback(i); }); // hehe
		Sharc.setIsCallbackRegistered(true);
	}
}

bool MiniSHARC::isInitialized() {
	printStatus();
	return _state[0] == 0x06;
}

void MiniSHARC::resetToDefaults() {
	_isCallbackRegistered = false;
	
	memset(_state, 0, 6);

	Wire.begin(VOLFP_ADDR);
}

void MiniSHARC::sendI2CBytes(byte major, byte minor) {
	Wire.beginTransmission(SHARC_ADDR);
	Wire.write(3);
	Wire.write(major);
	Wire.write(minor);
	Wire.endTransmission();
	delay(1);
}

void MiniSHARC::forceMiniSHARCRefresh() {
	sendI2CBytes(150, 0xff); // junk command does nothing except elicit a response from MiniSHARC
}

void MiniSHARC::waitForMiniSHARCToInitialize() {
	int count = 0;

	Serial.println("Waiting for MiniSHARC to initialize...");
	
	while(isInitialized() == false) {
		forceMiniSHARCRefresh();
		delay(1000);
		serprintf("Waiting... %d", ++count);
	}
	
	Serial.println("Initialized!");
}

void MiniSHARC::printStatus() {
	serprintf("Attenuation: %u  //  Input: %u  //  State: %02x-%02x-%02x-%02x-%02x-%02x", getAttenuation(), getConfig(), _state[0]&0xff, _state[1]&0xff, _state[2]&0xff, _state[3]&0xff, _state[4]&0xff, _state[5&0xff]);
}

void MiniSHARC::increaseAttenuation() {
	if(_currentAttenuation == 255)
		return;

	sendI2CBytes(150, 0);
}

void MiniSHARC::decreaseAttenuation() {
	if(_currentAttenuation == 0)
		return;

	sendI2CBytes(150, 1);
}

void MiniSHARC::increaseAttenuation(int steps) {
	for(int i = 0; i < steps; i++) {
		increaseAttenuation();
	}
}

void MiniSHARC::decreaseAttenuation(int steps) {
	for(int i = 0; i < steps; i++) {
		decreaseAttenuation();
	}
}

#define MAX(x,y) ( (x > y) ? x : y)
#define MIN(x,y) ( (x < y) ? x : y)
#define UP 1
#define DOWN -1

void MiniSHARC::setAttenuation(byte targetAttenuation) {
	if(targetAttenuation > 255)
		targetAttenuation = 255;

	if(targetAttenuation < 0)
		targetAttenuation = 0;

	int diff = MAX(targetAttenuation, getAttenuation()) - MIN(targetAttenuation, getAttenuation());
	if(diff == 0)
		return;

	int direction = (getAttenuation() < targetAttenuation) ? UP : DOWN;

	if(direction == UP)
		increaseAttenuation(diff);
	else
		decreaseAttenuation(diff);
}

void MiniSHARC::setConfig(byte config) {
	if(config < 1 || config > 4)
		return;

	config--; // The caller uses the range 1-4 but MiniSHARC I2C expects 0-3
		
	sendI2CBytes(138, config);
	
	while(getConfig() != config) {
		sendI2CBytes(138, config);
		delay(250);
	}
}

int MiniSHARC::getConfig() {
	return _currentConfig + 1;
}

int MiniSHARC::getAttenuation() {
	return _currentAttenuation;
}

bool MiniSHARC::isCallbackRegistered() {
	return _isCallbackRegistered;
}

void MiniSHARC::setIsCallbackRegistered(bool value) {
	_isCallbackRegistered = value;
}

void MiniSHARC::I2CReceiveCallback(int numBytes) {
	int pos = 0;

	while (0 < Wire.available()) {
		byte c = Wire.read();
		if(numBytes ==6 && pos < 6)
			_state[pos++] = c;
	}

	// All the responses we're interested in begin with 0x06
	if(_state[0] == 0x06) {
		_currentConfig =	_state[POS_CONFIG];
		_currentAttenuation =	_state[POS_VOLUME];
		_muted =			_state[POS_MUTE];
		//Sharc.printStatus();
	}
}

/*
	I don't know if this will work for you. The codes for mute *might*
	have something to do with the IR remote codes that the MiniSHARC
	is programmed for; more research is needed here.
*/
void MiniSHARC::toggleMute() {
	Wire.beginTransmission(SHARC_ADDR);
	Wire.write(9);
	Wire.write(149);
	Wire.write(135);
	Wire.write(1);
	Wire.write(0);
	Wire.write(255);
	Wire.write(2);
	Wire.write(253);
	Wire.write(246);
	Wire.endTransmission();
	delay(50);
	Wire.beginTransmission(SHARC_ADDR);
	Wire.write(5);
	Wire.write(149);
	Wire.write(131);
	Wire.write(1);
	Wire.write(0);
	Wire.endTransmission();
	delay(50);
}

bool MiniSHARC::isMuted() {
	return _muted;
}

void MiniSHARC::muteOn() {
	if(!_muted)
		toggleMute();
}

void MiniSHARC::muteOff() {
	if(_muted)
		toggleMute();
}

int MiniSHARC::getAttenuationPercentage() {
	int v = (getAttenuation() * 100) / 255;
	
	return v;
}

int MiniSHARC::getVolumePercentage() {
	int v = (getAttenuation() * 100) / 255;
	
	return 99 - v;
}

void MiniSHARC::check_Sharc() {
	while(!Sharc.isInitialized()) {
		Sharc.waitForMiniSHARCToInitialize();
	}
}
