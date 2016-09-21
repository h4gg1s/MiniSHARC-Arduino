/*
	MiniSHARC / Arduino integration via I2C.
	haggismonster @ https://www.minidsp.com/forum/

	This is open source with a "do what the fuck you want" license.
*/
#include <Wire.h>
#include "minisharc.h"

#define MAX(x,y) ( (x > y) ? x : y)
#define MIN(x,y) ( (x < y) ? x : y)
#define UP 1
#define DOWN -1

/*****************************************************************
 * This is the MiniSHARC class. Do not instantiate this.
 * You automatically get an instance of the "Sharc" class by 
 * including "minisharc.h". 
 * See "MiniSHARC-Arduino.ino" for an example.
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

	_muting = false;
	_muted = false;
	_savedAttenuation = 255; // by default we're assuming saved attenuation is fully muted
	
	Serial.println("Initialized!");
}

void MiniSHARC::printStatus() {
	serprintf("Volume: %u  //  Input: %u  //  State: %02x-%02x-%02x-%02x-%02x-%02x", getAttenuation(), getConfig(), _state[0]&0xff, _state[1]&0xff, _state[2]&0xff, _state[3]&0xff, _state[4]&0xff, _state[5&0xff]);
}

// "official" MiniDSP way of pinging the SHARC for a volume/state update
void MiniSHARC::refreshData() {
	Wire.beginTransmission(SHARC_ADDR);
	Wire.write(2);
	Wire.write(131);
	Wire.endTransmission();
}

void MiniSHARC::increaseAttenuation() {
	if(_currentAttenuation == 255)
		return;

	sendI2CBytes(150, 0);
	refreshData();
}

void MiniSHARC::decreaseAttenuation() {
	if(_currentAttenuation == 0)
		return;

	sendI2CBytes(150, 1);
	refreshData();
}

void MiniSHARC::increaseAttenuation(int steps) {
	for(int i = 0; i < steps; i++) {
		increaseAttenuation();
		delay(10);
	}
}

void MiniSHARC::decreaseAttenuation(int steps) {
	for(int i = 0; i < steps; i++) {
		decreaseAttenuation();
		delay(10);
	}
}

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
	if(config < 0 || config > 3)
		return;
		
	sendI2CBytes(138, config);
	
	while(getConfig() != config) {
		sendI2CBytes(138, config);
		delay(250);
	}
}

int MiniSHARC::getConfig() {
	return _currentConfig;
}

int MiniSHARC::getAttenuation() {
	return _currentAttenuation;
}

int MiniSHARC::getSavedAttenuation() {
	return _savedAttenuation;
}

bool MiniSHARC::isCallbackRegistered() {
	return _isCallbackRegistered;
}

void MiniSHARC::setIsCallbackRegistered(bool value) {
	_isCallbackRegistered = value;
}

void MiniSHARC::I2CReceiveCallback(int numBytes) {
	int pos = 0;

	noInterrupts();
	while (0 < Wire.available()) {
		byte c = Wire.read();
		if(numBytes == 6 && pos < 6)
			_state[pos++] = c;
	}

	// All the responses we're interested in begin with 0x06
	if(_state[0] == 0x06) {
		_currentConfig =		_state[POS_CONFIG];
		_currentAttenuation =	_state[POS_ATTENUATION];
		//_muted =			_state[POS_MUTE];
	}

	if(_currentAttenuation == 255)
		_muted = true;
	else
		_muted = false;

	interrupts();
	//Sharc.printStatus();
}

void MiniSHARC::toggleMute() {
	int i;
	unsigned long time;

	if(_muting)
		return;
	_muting = true;

	if(_muted == true) {
		int diff = 255 - _savedAttenuation;

		time = millis();
		serprintf("Unmuting! Attenuation: %d @ %lu. _savedAttenuation: %d. Diff: %d", _currentAttenuation, time, _savedAttenuation, diff);

		// takes about 2.5 seconds
		decreaseAttenuation(diff);
		serprintf("Time of finish: %lu (%lu ms)", millis(), millis() - time);
		refreshData();
		delay(50);
		serprintf("Attenuation is now: %d @ %lu. i = %d. _savedAttenuation = %d", _currentAttenuation, millis(), i, _savedAttenuation);
		_muted = false;
	} 
	// We are about to mute the audio
	else if(_muted == false) {		
		_savedAttenuation = _currentAttenuation;
		time = millis();
		serprintf("Muting! Attenuation: %d @ %lu. _savedAttenuation: %d", _currentAttenuation, time, _savedAttenuation);
		// takes about 300ms
		for(i = _currentAttenuation; i < 255; i++) {
			sendI2CBytes(150, 0);	
		}
		serprintf("Time of finish: %lu (%lu ms)", millis(), millis() - time);
		refreshData();
		delay(50);
		serprintf("Attenuation is now: %d @ %lu", _currentAttenuation, millis());
		_muted = true;
	}
	_muting = false;
}

/*
void MiniSHARC::toggleMute() {
	if(_muting)
		return;

	char cmd1[] = { 9, 149, 135, 1, 0, 255, 2, 253, 246 };
	char cmd2[] = { 5, 149, 131, 1, 0 };

	Wire.beginTransmission(SHARC_ADDR);
	
	for(int i = 0; i < 9; i++)
		Wire.write(cmd1[i]);
	
	Wire.endTransmission();
	delay(50);
	Wire.beginTransmission(SHARC_ADDR);
	
	for(int i = 0; i < 5; i++)
		Wire.write(cmd2[i]);

	Wire.endTransmission();
	delay(50);
}
*/


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

int MiniSHARC::getVolumePercentage() {
	int v = (getAttenuation() * 100) / 255;
	if(v == 100)
		v--;

	return 99 - v;
}

void MiniSHARC::check_Sharc() {
	while(!Sharc.isInitialized()) {
		Sharc.waitForMiniSHARCToInitialize();
	}
}
