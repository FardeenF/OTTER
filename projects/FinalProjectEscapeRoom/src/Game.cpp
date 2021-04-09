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
	AudioEngine& engine = AudioEngine::Instance();
	engine.Init();
	engine.LoadBank("EscapeRoom");
	engine.LoadBus("MusicBus", "{fc9c4d82-6f53-4fb4-9721-91f70d476d46}");

	AudioEvent& AudioClip1 = engine.CreateEvent("Line1", "{5c9c1e3d-c65c-4757-a2a0-ba11e9f042f9}");
	AudioEvent& AudioClip2 = engine.CreateEvent("Line2", "{9996db1a-a8e9-4ae0-8399-7e219cd06f9f}");
	AudioEvent& AudioClip3 = engine.CreateEvent("Line3", "{25bd72d9-a65e-464d-93a1-27ae396af7be}");
	AudioEvent& AudioClip4 = engine.CreateEvent("Line4", "{0667b465-9d0b-4440-8351-cf290fab4bd0}");
	AudioEvent& AudioClip5 = engine.CreateEvent("Line5", "{3e21c6af-653e-4738-8fba-30e2fb694a5a}");
	AudioEvent& AudioClip6 = engine.CreateEvent("Line6", "{baf315b6-dc34-4cac-9f46-3c5f76448094}");
	AudioEvent& AudioClip7 = engine.CreateEvent("Line7", "{7fa17143-2d2a-4175-980b-3a7ca0de114e}");
	AudioEvent& AudioClip8 = engine.CreateEvent("Line8", "{04d3efb3-6a32-4eee-9f07-131c6eed87ce}");
	AudioEvent& AudioClip9 = engine.CreateEvent("Line9", "{549504cf-46f3-43a8-8ce7-318118f236e6}");
	AudioEvent& AudioClip10 = engine.CreateEvent("Line10", "{83a14de3-a8e0-4308-8848-797f03ca21e9}");
	AudioEvent& AudioClip11 = engine.CreateEvent("Line11", "{d4b5c6dc-26c0-45fc-993d-0a1055751ea6}");
	AudioEvent& AudioClip12 = engine.CreateEvent("Line12", "{a8ebbdf6-6397-405f-9fd5-c4c514ca980d}");
	AudioEvent& AudioClip13 = engine.CreateEvent("Line13", "{a8dc6709-11e4-4965-b1a4-eb64663c3ccc}");
	AudioEvent& AudioClip14 = engine.CreateEvent("Line14", "{a1d3272f-064e-4c59-b677-354707de5b07}");
	AudioEvent& AudioClip15 = engine.CreateEvent("Line15", "{d5cf1915-491a-434d-9172-f4867cf55e3c}");
	AudioEvent& AudioClip16 = engine.CreateEvent("Line16", "{21d79347-e519-40c8-b21b-0a797129e0a4}");
	AudioEvent& AudioClip17 = engine.CreateEvent("Line17", "{f96ed195-1f8c-4d54-af49-b0b416e76dda}");

	//AudioClip1.Play();
}

//------------------------------------------------------------------------
// Update your game here. 
//------------------------------------------------------------------------
void Update(float deltaTime)
{
	// Increment game time
	gameTime += deltaTime;

	AudioEngine& engine = AudioEngine::Instance();

	//Get ref to music
	AudioEvent& AudioClip1 = engine.GetEvent("Audio1");

	//Get ref to bus
	AudioBus& musicBus = engine.GetBus("MusicBus");

	AudioListener& listener = engine.GetListener();

	listener.SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));


	
	engine.Update();
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
	AudioEngine::Instance().Shutdown();
}


void Gameplay()
{
	AudioEngine& engine = AudioEngine::Instance();
	AudioEvent& AudioClip1 = engine.GetEvent("Line1");
	AudioEvent& AudioClip2 = engine.GetEvent("Line2");
	AudioEvent& AudioClip3 = engine.GetEvent("Line3");
	AudioEvent& AudioClip4 = engine.GetEvent("Line4");
	AudioEvent& AudioClip5 = engine.GetEvent("Line5");
	AudioEvent& AudioClip6 = engine.GetEvent("Line6");
	AudioEvent& AudioClip7 = engine.GetEvent("Line7");
	AudioEvent& AudioClip8 = engine.GetEvent("Line8");
	AudioEvent& AudioClip9 = engine.GetEvent("Line9");
	AudioEvent& AudioClip10 = engine.GetEvent("Line10");
	AudioEvent& AudioClip11 = engine.GetEvent("Line11");
	AudioEvent& AudioClip12 = engine.GetEvent("Line12");
	AudioEvent& AudioClip13 = engine.GetEvent("Line13");
	AudioEvent& AudioClip14 = engine.GetEvent("Line14");
	AudioEvent& AudioClip15 = engine.GetEvent("Line15");
	AudioEvent& AudioClip16 = engine.GetEvent("Line16");
	AudioEvent& AudioClip17 = engine.GetEvent("Line17");


	AudioClip1.Play();
}


