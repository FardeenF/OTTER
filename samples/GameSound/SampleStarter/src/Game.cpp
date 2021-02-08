//------------------------------------------------------------------------
// Game.cpp
//------------------------------------------------------------------------

#include "AudioEngine.h"

float gameTime;

//------------------------------------------------------------------------
// Called before first update. Do any initial setup here.
//------------------------------------------------------------------------

void Init()
{
	// Setup FMOD
	AudioEngine& engine = AudioEngine::Instance();
	engine.Init();
	engine.LoadBank("Master");

	// Play a music event
	AudioEvent& music = engine.CreateEvent("music", "{b56cb9d2-1d47-4099-b80e-7d257b99a823}"); // Right-click event in fmod -> copy GUID
	music.Play();
	
	// Load a bus
	engine.LoadBus("MusicBus", "{a5b53ded-d7b3-4e6b-a920-0b241ef6f268}"); // Right-click bus in fmod -> Copy GUID
																		  
	// Get ref to Listener
	AudioListener& listener = engine.GetListener(); // Can use this listener to change the player's 3D position
}

//------------------------------------------------------------------------
// Update your game here. 
//------------------------------------------------------------------------
void Update(float deltaTime)
{

	// Increment game time
	gameTime += deltaTime;


	//// Grab some references to our audio objects ////
	AudioEngine& audioEngine = AudioEngine::Instance();

	AudioEvent& musicEvent = audioEngine.GetEvent("music"); // Make sure things strings match the names we gave them above!
	AudioBus& musicBus = audioEngine.GetBus("MusicBus");
	
	
	// Pause music for anticipation
	if (gameTime > 4.0f)
	{
		// Do something
		musicBus.SetPaused(true);
	}

	if (gameTime > 15.0f) 
	{
		// Change some parameters to make it sound funky
		musicEvent.SetParameter("Underwater", 1.0f);
		audioEngine.SetGlobalParameter("Timewarp", 0.0f);
	}



	// Make sure to update!
	audioEngine.Update();
}


//------------------------------------------------------------------------
// Add your display calls here
//------------------------------------------------------------------------
void Render()
{
	// Who needs graphics when you have audio?
}


//------------------------------------------------------------------------
// Add your shutdown code here.
//------------------------------------------------------------------------
void Shutdown()
{
	// Important! We don't want memory leaks! 
	AudioEngine::Instance().Shutdown();
}