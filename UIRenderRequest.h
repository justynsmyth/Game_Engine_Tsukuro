#pragma once
#include <string>
#include "ThirdParty/SDL/include/SDL.h"
class UIRenderRequest
{
public:
	std::string image;
	SDL_Color color;
	float x;
	float y;
	int sorting_order;

	UIRenderRequest();
	UIRenderRequest(std::string image, SDL_Color color, float x, float y, int sorting_order);

};

