#pragma once
#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>

#include "ThirdParty/rapidjson/include/rapidjson/filereadstream.h"
#include "ThirdParty/rapidjson/include/rapidjson/document.h"

class GameManager
{
public:
	static void ReadJsonFile(const std::string& path, rapidjson::Document& out_document)
	{
		FILE* file_pointer = nullptr;
#ifdef _WIN32
		fopen_s(&file_pointer, path.c_str(), "rb");
#else
		file_pointer = fopen(path.c_str(), "rb");
        
#endif
        if (!file_pointer) {
            return;
        }
		char buffer[65536];
		rapidjson::FileReadStream stream(file_pointer, buffer, sizeof(buffer));
		out_document.ParseStream(stream);
		std::fclose(file_pointer);

		if (out_document.HasParseError()) {
			std::cout << "error parsing json at [" << path << "]" << std::endl;
			exit(0);
		}
	}

	GameManager(int initialHealth, int initialScore);
    
	void changeHealth(int val);

	void changeScore(int val);

	bool running();

	void quitGame();

	char getState();

	void setState(char c);

	int getScore();

	int getHealth();

	bool playerWon();

	// End the scene and set player win/loss
	// Parameters:
	// state: determines whether you won or lost
	void setWin(bool state);

private:
	int health;
	int score;
	char state;
	bool run = true;
	bool playerWin;
};
#endif

