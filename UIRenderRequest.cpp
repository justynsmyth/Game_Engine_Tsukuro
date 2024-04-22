#include "UIRenderRequest.h"

UIRenderRequest::UIRenderRequest() {
	image = "";
	x = 0;
	y = 0;
	sorting_order = 0;
	color = { 255, 255, 255, 255 };
}
UIRenderRequest::UIRenderRequest(std::string image, SDL_Color color,
	float x, float y,
	int sorting_order) :
	image(image), color(color), x(x), y(y), sorting_order(sorting_order)
{}