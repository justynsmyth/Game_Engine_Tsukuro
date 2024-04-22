#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <string>
#include "Header.h"
#include "ThirdParty/rapidjson/include/rapidjson/document.h"
#include "AudioHelper.h"

class AudioDB
{
public:
	AudioDB();

	static inline bool playing;
	// calls Mix_PlayChannnel498
	// Parameters:
	//    clip_name: checks tracklist dictionary for Mix_Chunk*
	//    loops: determine how many times the track will play (t: infinite, f: once)
	static void PlayChannel(int channel, std::string clip_name, bool loops);
	static void HaltChannel(int channel);
	static void SetVolume(int channel, float volume);
	bool isPlaying();
private:
	static inline std::unordered_map<std::string, Mix_Chunk*> tracklist;
};

