#pragma once
#include <string>
#include "ThirdParty/SDL/include/SDL.h"

class TextRenderRequest
{
public:
	std::string text;
	std::string font;
	SDL_Color color;
	int size;
	int x;
	int y;

	TextRenderRequest();
	TextRenderRequest(
		std::string text, std::string font, SDL_Color color, int size, int x, int y);
};

