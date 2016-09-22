#include "minisharc.h"

/*
	"Volume" isn't really volume in MiniSHARC. What we're actually measuring/setting
	is attenuation - the amount by which we're attenuatin the signal. When we call
	Sharc.setAttenuation(x), we're telling the DSP to attenuate the signal by X where
	X is in the range 0 to 255.

	As such, calling Sharc.increaseAttenuation() actually decreases volume; similarly, 
	Sharc.decreaseAttenuation() will increase the volume. Calling Sharc.getAttenuation() 
	will return a value in the range 0 - 255 describing the attenuation value.

	Therefore a value of:
		0   = no attenuation (full volume)
		255 = max attenuation (muted)
*/

void setup() {
	// Setup serial monitor
	Serial.begin(115200);

	// Initialize the MiniSHARC
	Sharc.waitForMiniSHARCToInitialize();
}

void loop() {
	// Always run this every time around the loop() to make sure things are sane.
	// If the MiniSHARC isn't detected, this will wait until it is.
	Sharc.check_Sharc();

	
	// Dump internal state (volume, config, state) to the serial monitor
	Sharc.printStatus();


	// Get the current attenuation
	byte currentAttenuation = Sharc.getAttenuation(); // 0 - 255
	int volumePercent = Sharc.getVolumePercentage(); // 0 - 100%
	int currentConfig = Sharc.getConfig(); // 1 - 4
	serprintf("Attenuation: %d  //  Vol %%: %d%%  //  Config: %d", currentAttenuation, volumePercent, currentConfig);

	
	// Toggle mute on and off a few times
	serprintf("Muted: %d", Sharc.isMuted());
	
	Sharc.toggleMute();
	serprintf("Muted: %d", Sharc.isMuted());
	delay(500);

	Sharc.toggleMute();
	serprintf("Muted: %d", Sharc.isMuted());
	delay(500);

	Sharc.toggleMute();
	serprintf("Muted: %d", Sharc.isMuted());
	delay(2000);

	
	// Force mute on/off a few times
	Sharc.muteOn();
	serprintf("Muted: %d", Sharc.isMuted());
	delay(500);

	Sharc.muteOff();
	serprintf("Muted: %d", Sharc.isMuted());
	delay(500);

	Sharc.muteOn();
	serprintf("Muted: %d", Sharc.isMuted());
	delay(500);

	Sharc.muteOff();
	serprintf("Muted: %d", Sharc.isMuted());
	delay(2000);

	
	// Forcibly set the attenuation to a specific value
	serprintf("Setting attenuation to 50");
	Sharc.setAttenuation(50);
	Sharc.printStatus();
	delay(2000);

	
	// Forcibly set the attenuation to a specific value
	serprintf("Setting attenuation to 100");
	Sharc.setAttenuation(100);
	Sharc.printStatus();
	delay(2000);

	
	// Increase the volume 10 times in .5dB increments
	serprintf("Decreasing attenuation using 10x 0.5dB decrements");
	Sharc.decreaseAttenuation(10);
	Sharc.printStatus();
	delay(2000);

	
	// Decrease the volume 10 times in .5dB decrements
	serprintf("Increasing attenuation using 10x 0.5dB increments");
	Sharc.increaseAttenuation(10);
	Sharc.printStatus();

	
	// Change the configuration (in range 1 - 4)
	serprintf("Changing MiniSHARC config to 2");
	Sharc.setConfig(2); 
	Sharc.printStatus();

	
	// Change the configuration (in range 1 - 4)
	serprintf("Changing MiniSHARC config to 1");
	Sharc.setConfig(1); 

	delay(2000);
}
