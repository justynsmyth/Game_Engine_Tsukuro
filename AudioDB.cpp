#include "AudioDB.h"

namespace fs = std::filesystem;

bool AudioDB::isPlaying() {
	return playing;
}

AudioDB::AudioDB() {
	AudioHelper::Mix_OpenAudio498(22050, AUDIO_S16SYS, 2, 640);
	AudioHelper::Mix_AllocateChannels498(50);
}

void AudioDB::PlayChannel(int channel, std::string clip_name, bool loops) {
	int loop_val = 0;
	if (loops) {
		loop_val = -1;
	}
	if (tracklist.find(clip_name) != tracklist.end()) {
		Mix_Chunk* track = tracklist[clip_name];
		AudioHelper::Mix_PlayChannel498(channel, track, loop_val);
		playing = true;
	}
	else {
		std::string bgmWav = "resources/audio/" + clip_name + ".wav";
		std::string bgmOgg = "resources/audio/" + clip_name + ".ogg";
		if (!fs::exists(bgmWav)) {
			if (!fs::exists(bgmOgg)) {
				std::cout << "error: failed to play audio clip " + clip_name;
				exit(0);
			}
			else {
				tracklist[clip_name] = AudioHelper::Mix_LoadWAV498(bgmOgg.c_str());
			}
		}
		else {
			tracklist[clip_name] = AudioHelper::Mix_LoadWAV498(bgmWav.c_str());
		}
		Mix_Chunk* track = tracklist[clip_name];
		AudioHelper::Mix_PlayChannel498(channel, track, loop_val);
		playing = true;
	}
}


void AudioDB::HaltChannel(int channel) {
	AudioHelper::Mix_HaltChannel498(channel);
	playing = false;
}
void AudioDB::SetVolume(int channel, float volume) {
	int vol = static_cast<int>(volume);
	vol = std::min(std::max(vol, 0), 128);
	AudioHelper::Mix_Volume498(channel, vol);
}