#include "DrawPixelRequest.h"

DrawPixelRequest::DrawPixelRequest() {
	x = 0;
	y = 0;
	color = { 255, 255, 255, 255 };
}
DrawPixelRequest::DrawPixelRequest(SDL_Color color, float x, float y) :
	color(color), x(x), y(y)
{}