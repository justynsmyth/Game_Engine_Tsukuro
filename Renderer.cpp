#include "Renderer.h"
#include "SceneDB.h"

namespace fs = std::filesystem;

Renderer::Renderer(rapidjson::Document& renderConfig, rapidjson::Document& gameConfig, AudioDB* audioDB, GameManager* game, openvr_utils* Open_VR)
	: gameManager(game), audioDB(audioDB), x_scale_flip_on_movement(false), OpenGLActivated(false) {

	SDL_Init(SDL_INIT_VIDEO);

	int DEF_SCREEN_WIDTH = 640;
	int DEF_SCREEN_HEIGHT = 360;

	cameraPosition.x = 0;
	cameraPosition.y = 0;

	int r, g, b;
	std::string rendering_mode = "SDL"; // Default to SDL
	if (!renderConfig.IsNull()) {
		r = renderConfig.HasMember("clear_color_r") ? renderConfig["clear_color_r"].GetInt() : 255;
		g = renderConfig.HasMember("clear_color_g") ? renderConfig["clear_color_g"].GetInt() : 255;
		b = renderConfig.HasMember("clear_color_b") ? renderConfig["clear_color_b"].GetInt() : 255;
		zoom_factor = renderConfig.HasMember("zoom_factor") ? renderConfig["zoom_factor"].GetFloat() : 1;
		window_w = renderConfig.HasMember("x_resolution") ? renderConfig["x_resolution"].GetInt() : DEF_SCREEN_WIDTH;
		window_h = renderConfig.HasMember("y_resolution") ? renderConfig["y_resolution"].GetInt() : DEF_SCREEN_HEIGHT;
		x_scale_flip_on_movement = renderConfig.HasMember("x_scale_actor_flipping_on_movement") ? renderConfig["x_scale_actor_flipping_on_movement"].GetBool() : false;
		if (renderConfig.HasMember("rendering_mode")) {
			rendering_mode = renderConfig["rendering_mode"].GetString();
		}
	}
	else {
		r = 255;
		g = 255;
		b = 255;
		zoom_factor = 1;
		window_w = DEF_SCREEN_WIDTH;
		window_h = DEF_SCREEN_HEIGHT;
	}

	std::string windowName = gameConfig.HasMember("game_title") ? gameConfig["game_title"].GetString() : "";

	if (rendering_mode == "OpenGL") {
		OpenGLActivated = true;
		//Graphics program
		gProgramID = 0;
		gVertexPos2DLocation = -1;
		// vertices are put in Vertex Buffer Objects (VBO)
		gVBO = 0;
		// specify the order to draw VBOs in Index Buffer Objects (IBO)
		gIBO = 0;

		// Set OpenGL attributes
		// Request an OpenGL 3.3 context (requires compatible hardware)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		// Use core OpenGL profile
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		// Debug OpenGL Mode
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

		window = Helper::SDL_CreateWindow498(
			windowName.c_str(), // window title
			SDL_WINDOWPOS_UNDEFINED, // initial x position
			SDL_WINDOWPOS_UNDEFINED, // initial y position
			window_w, // width, in pixels
			window_h, // height, in pixels
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN // flags
		);

		if (window == nullptr) {
			std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
			return;
		}

		SDL_GLContext gContext = SDL_GL_CreateContext(window);
		if (gContext == NULL)
		{
			std::cout << "OpenGL context could not be created! SDL Error: " << SDL_GetError() << std::endl;
		}
		else
		{
			// Initialize GLAD
			if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
			{
				std::cout << "Failed to initialize GLAD" << std::endl;
			}

			// Use Vsync
			if (SDL_GL_SetSwapInterval(1) < 0)
			{
				std::cout << "Warning: Unable to set VSync! SDL Error: " << SDL_GetError() << std::endl;
			}

			Open_VR->Initialize();

			// Initialize OpenGL
			if (!initGL(Open_VR))
			{
				std::cout << "Unable to initialize OpenGL!\n" << std::endl;
			}

		

		}
		Open_VR->InitiateCompanionWindow(window, window_w, window_h);
	}
	else if (rendering_mode != "SDL") {
		std::cout << "render_mode parameter: " << rendering_mode << " in rendering.config is not allowed." << std::endl;
		std::cout << "Default is SDL. You can change it to OpenGL." << std::endl;
		exit(1);
	}
	else {
		window = Helper::SDL_CreateWindow498(
			windowName.c_str(), // window title
			SDL_WINDOWPOS_UNDEFINED, // initial x position
			SDL_WINDOWPOS_UNDEFINED, // initial y position
			window_w, // width, in pixels
			window_h, // height, in pixels
			SDL_WINDOW_SHOWN // flags
		);

		if (window == nullptr) {
			std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
			return;
		}
		// -1: Grab first driver available 
		renderer = Helper::SDL_CreateRenderer498(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
		clear_color_r = r;
		clear_color_g = g;
		clear_color_b = b;

		SDL_SetRenderDrawColor(renderer, clear_color_r, clear_color_g, clear_color_b, 255);
		SDL_RenderClear(renderer);
	}

	if (!gameConfig.HasMember("intro_image")) {
		game->setState('s');
	}
}

bool Renderer::isOpenGL()
{
	return OpenGLActivated;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize OpenGL. Returns true if OpenGL has been successfully
//          initialized, false if shaders could not be created.
//          If failure occurred in a module other than shaders, the function
//          may return true or throw an error. 
//-----------------------------------------------------------------------------
bool Renderer::initGL(openvr_utils* Open_VR)
{
	//Success flag
	bool success = true;

	if (!Open_VR->CreateAllShaders())
		return false;

	Open_VR->SetupTexturemaps();
	Open_VR->SetupScene();
	Open_VR->SetupCameras();
	Open_VR->SetupStereoRenderTargets();
	Open_VR->SetupCompanionWindow();

	//Generate program
	gProgramID = glCreateProgram();
	//Create vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

	//Get vertex source
	const GLchar* vertexShaderSource[] =
	{
		"#version 140\nin vec2 LVertexPos2D; void main() { gl_Position = vec4( LVertexPos2D.x, LVertexPos2D.y, 0, 1 ); }"
	};

	//Set vertex source
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);

	//Compile vertex source
	glCompileShader(vertexShader);

	//Check vertex shader for errors
	GLint vShaderCompiled = GL_FALSE;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
	if (vShaderCompiled != GL_TRUE)
	{
		printf("Unable to compile vertex shader %d!\n", vertexShader);
		printShaderLog(vertexShader);
		success = false;
	}
	else
	{
		//Attach vertex shader to program
		glAttachShader(gProgramID, vertexShader);


		//Create fragment shader
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

		//Get fragment source
		const GLchar* fragmentShaderSource[] =
		{
			"#version 140\nout vec4 LFragment; void main() { LFragment = vec4( 1.0, 1.0, 1.0, 1.0 ); }"
		};

		//Set fragment source
		glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);

		//Compile fragment source
		glCompileShader(fragmentShader);

		//Check fragment shader for errors
		GLint fShaderCompiled = GL_FALSE;
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
		if (fShaderCompiled != GL_TRUE)
		{
			printf("Unable to compile fragment shader %d!\n", fragmentShader);
			printShaderLog(fragmentShader);
			success = false;
		}
		else
		{
			//Attach fragment shader to program
			glAttachShader(gProgramID, fragmentShader);


			//Link program
			glLinkProgram(gProgramID);

			//Check for errors
			GLint programSuccess = GL_TRUE;
			glGetProgramiv(gProgramID, GL_LINK_STATUS, &programSuccess);
			if (programSuccess != GL_TRUE)
			{
				printf("Error linking program %d!\n", gProgramID);
				printProgramLog(gProgramID);
				success = false;
			}
			else
			{
				//Get vertex attribute location
				gVertexPos2DLocation = glGetAttribLocation(gProgramID, "LVertexPos2D");
				if (gVertexPos2DLocation == -1)
				{
					printf("LVertexPos2D is not a valid glsl program variable!\n");
					success = false;
				}
				else
				{
					//Initialize clear color
					glClearColor(0.f, 0.f, 0.f, 1.f);

					//VBO data
					GLfloat vertexData[] =
					{
						-0.5f, -0.5f,
						 0.5f, -0.5f,
						 0.5f,  0.5f,
						-0.5f,  0.5f
					};

					//IBO data
					GLuint indexData[] = { 0, 1, 2, 3 };

					//Create VBO
					glGenBuffers(1, &gVBO);
					glBindBuffer(GL_ARRAY_BUFFER, gVBO);
					glBufferData(GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);

					//Create IBO
					glGenBuffers(1, &gIBO);
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
					glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), indexData, GL_STATIC_DRAW);
				}
			}
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);
		}
	}

	return success;
}

void Renderer::printProgramLog(GLuint program)
{
	//Make sure name is shader
	if (glIsProgram(program))
	{
		//Program log length
		int infoLogLength = 0;
		int maxLength = infoLogLength;

		//Get info string length
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

		//Allocate string
		char* infoLog = new char[maxLength];

		//Get info log
		glGetProgramInfoLog(program, maxLength, &infoLogLength, infoLog);
		if (infoLogLength > 0)
		{
			//Print Log
			printf("%s\n", infoLog);
		}

		//Deallocate string
		delete[] infoLog;
	}
	else
	{
		printf("Name %d is not a program\n", program);
	}
}

void Renderer::printShaderLog(GLuint shader)
{
	//Make sure name is shader
	if (glIsShader(shader))
	{
		//Shader log length
		int infoLogLength = 0;
		int maxLength = infoLogLength;

		//Get info string length
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		//Allocate string
		char* infoLog = new char[maxLength];

		//Get info log
		glGetShaderInfoLog(shader, maxLength, &infoLogLength, infoLog);
		if (infoLogLength > 0)
		{
			//Print Log
			printf("%s\n", infoLog);
		}

		//Deallocate string
		delete[] infoLog;
	}
	else
	{
		printf("Name %d is not a shader\n", shader);
	}
}



void Renderer::ProcessInput() {
	SDL_Event e;
	while (Helper::SDL_PollEvent498(&e)) {
		Input::ProcessEvent(e);
		if (e.type == SDL_QUIT) {
			gameManager->quitGame();
		}
	}
}

void Renderer::renderScene(std::vector<std::shared_ptr<Actor>> actors) {
	std::stable_sort(image_render_requests.begin(), image_render_requests.end(), CompareImageRequests());
	std::stable_sort(ui_render_requests.begin(), ui_render_requests.end(), CompareUIRequests());
	SDL_RenderSetScale(renderer, zoom_factor, zoom_factor);
		
	for (auto& request : image_render_requests) {
		RenderImageRequest(request);
	}
	image_render_requests.clear();	
	SDL_RenderSetScale(renderer, 1.0f, 1.0f);
	
	for (auto& request : ui_render_requests) {
		RenderUIRequest(request);
	}
	ui_render_requests.clear();
	
	for (auto& request : text_render_requests) {
		RenderTextRequest(request);
	}
	text_render_requests.clear();

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	for (auto& request : pixel_render_requests) {
		RenderPixelRequest(request);
	}
	SDL_SetRenderDrawColor(renderer, clear_color_r, clear_color_g, clear_color_b, 255);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	pixel_render_requests.clear();
}

void Renderer::RenderPixelRequest(DrawPixelRequest& pixel_request) {
	SDL_SetRenderDrawColor(renderer, pixel_request.color.r, pixel_request.color.g, pixel_request.color.b, pixel_request.color.a);
	SDL_RenderDrawPoint(renderer, pixel_request.x, pixel_request.y);
}

void Renderer::RenderImageRequest(ImageRenderRequest& image_request) {
	float centerX = window_w / 2;
	float centerY = window_h / 2;
	SDL_Texture* texture = getTexture(image_request.image);

	int texW = 0;
	int texH = 0;
	SDL_QueryTexture(texture, 0, 0, &texW, &texH);

	SDL_Point pivotSDLPoint = {
	   static_cast<int>(image_request.pivot_x * texW * image_request.scale_x),
	   static_cast<int>(image_request.pivot_y * texH * image_request.scale_y)
	};
	float rotationDegrees = static_cast<int>(image_request.rotation);

	SDL_Rect dstRect;
	dstRect.x = image_request.x * 100 + centerX / zoom_factor - pivotSDLPoint.x - cameraPosition.x * 100;
	dstRect.y = image_request.y * 100 + centerY / zoom_factor - pivotSDLPoint.y - cameraPosition.y * 100;
	dstRect.w = texW * std::abs(image_request.scale_x);
	dstRect.h = texH * std::abs(image_request.scale_y);


	SDL_SetTextureColorMod(texture, image_request.color.r, image_request.color.g, image_request.color.b);
	SDL_SetTextureAlphaMod(texture, image_request.color.a);

	Helper::SDL_RenderCopyEx498(0, "", renderer, texture, NULL, &dstRect, rotationDegrees, &pivotSDLPoint, SDL_FLIP_NONE);

	SDL_SetTextureColorMod(texture,
		image_request.color.r,
		image_request.color.g,
		image_request.color.b);
	SDL_SetTextureAlphaMod(texture,
		image_request.color.a);
}


void Renderer::LuaDrawPixel(float x, float y, float r, float g, float b, float a) {
	Uint8 red = static_cast<int>(r);
	Uint8 green = static_cast<int>(g);
	Uint8 blue = static_cast<int>(b);
	Uint8 alpha = static_cast<int>(a);
	SDL_Color color = { red, green, blue, alpha };
	pixel_render_requests.emplace_back(color, x, y);
}

void Renderer::LuaDraw(const std::string image_name, float x, float y) {
	// TODO: Camera and ZOOM will affect this!
	SDL_Texture* texture = getTexture(image_name);
	int texture_w;
	int texture_h;
	SDL_QueryTexture(texture, 0, 0, &texture_w, &texture_h);

	SDL_Color color = { 255, 255, 255, 255 };
	image_render_requests.emplace_back(image_name, color, x, y, 0, 0, 1, 1, 0.5f, 0.5f);
}

void Renderer::LuaDrawEx(const std::string image_name,
						float x, float y,
						float rotation_degrees,
						float scale_x, float scale_y,
						float pivot_x, float pivot_y,
						float r, float g, float b, float a,
						float sorting_order) {
	// TODO: Camera and ZOOM will affect this!
	Uint8 red = static_cast<int>(r);
	Uint8 green = static_cast<int>(g);
	Uint8 blue = static_cast<int>(b);
	Uint8 alpha = static_cast<int>(a);

	int rot_degree_int = static_cast<int>(rotation_degrees);
	int sort_order_int = static_cast<int>(sorting_order);

	SDL_Texture* texture = getTexture(image_name);
	int texture_w;
	int texture_h;
	SDL_QueryTexture(texture, 0, 0, &texture_w, &texture_h);

	SDL_Color color = { red, green, blue, alpha };
	image_render_requests.emplace_back(image_name, color, x, y, sort_order_int, rot_degree_int, scale_x, scale_y, pivot_x, pivot_y);
}

void Renderer::RenderUIRequest(UIRenderRequest& ui_request) {

	SDL_Texture* texture = getTexture(ui_request.image);
	int texW = 0;
	int texH = 0;
	SDL_QueryTexture(texture, 0, 0, &texW, &texH);
	SDL_Rect dst_rect;
	dst_rect.x = static_cast<int>(ui_request.x);
	dst_rect.y = static_cast<int>(ui_request.y);
	dst_rect.w = texW;
	dst_rect.h = texH;

	SDL_SetTextureColorMod(texture,
		ui_request.color.r,
		ui_request.color.g,
		ui_request.color.b);
	SDL_SetTextureAlphaMod(texture,
		ui_request.color.a);

	SDL_RenderCopy(renderer, texture, nullptr, &dst_rect);

	SDL_SetTextureColorMod(texture,
		ui_request.color.r,
		ui_request.color.g,
		ui_request.color.b);
	SDL_SetTextureAlphaMod(texture,
		ui_request.color.a);
}

void Renderer::LuaDrawUIEx(const std::string image_name, float x, float y, float r, float g, float b, float a, float sorting_order) {
	int x_int = static_cast<int>(x);
	int y_int = static_cast<int>(y);

	Uint8 red = static_cast<int>(r);
	Uint8 green = static_cast<int>(g);
	Uint8 blue = static_cast<int>(b);
	Uint8 alpha = static_cast<int>(a);
	
	int sort_order_int = static_cast<int>(sorting_order);


	SDL_Color color = { red, green, blue, alpha };

	SDL_Texture* texture = getTexture(image_name);
	int texture_w;
	int texture_h;
	SDL_QueryTexture(texture, 0, 0, &texture_w, &texture_h);

	ui_render_requests.emplace_back(image_name, color, x_int, y_int, sort_order_int);
}

void Renderer::LuaDrawUI(const std::string image, float x, float y) {
	int x_int = static_cast<int>(x);
	int y_int = static_cast<int>(y);
	SDL_Texture* texture = getTexture(image);
	int texture_w;
	int texture_h;
	SDL_QueryTexture(texture, 0, 0, &texture_w, &texture_h);

	SDL_Color color = { 255, 255, 255, 255 };
	ui_render_requests.emplace_back(image, color, x_int, y_int, 0);
}

SDL_Texture* Renderer::getTexture(const std::string& imageName) {
	auto found = textures.find(imageName);
	if (found != textures.end()) {
		return found->second;
	}
	else {
		std::string imagePath = "resources/images/" + imageName + ".png";
		if (!fs::exists(imagePath)) {
			std::cout << "error: missing image " << imageName;
			exit(0);
		}
		SDL_Texture* texture = nullptr;
		texture = IMG_LoadTexture(renderer, imagePath.c_str());

		textures[imageName] = texture;
		return texture;
	}
	return nullptr;
}

/*
// Check if font exists in files.
// If it does modify the string reference for fontPath
*/
void Renderer::CheckFont(const std::string& fontName) {
	std::string fontPath = "resources/fonts/" + fontName + ".ttf";
	if (!fs::exists(fontPath)) {
		std::cout << "error: font " << fontName << " missing";
		exit(0);
	}
}

void Renderer::LuaDrawText(const std::string str_content,
	float x, float y,
	const std::string& font_name, float font_size,
	float r, float g, float b, float a) {

	int x_int = static_cast<int>(x);
	int y_int = static_cast<int>(y);
	int font_size_int = static_cast<int>(font_size);

	Uint8 red = static_cast<int>(r);
	Uint8 green = static_cast<int>(g);
	Uint8 blue = static_cast<int>(b);
	Uint8 alpha = static_cast<int>(a);

	SDL_Color color = { red, green, blue, alpha };
	TextRenderRequest t(str_content, font_name, color, font_size_int, x_int, y_int);
	text_render_requests.emplace_back(t);
}



// Function to Render Text
// Parameters:
//   font: the font of the rendered text
//   text: the string that will be rendered
//   x: x-coordinate
//   y: y-coordinate
// Returns: None
void Renderer::RenderText(TTF_Font* font, const std::string& text, int x, int y, SDL_Color& color) {
	SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
	SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
	SDL_Rect dstRect = { x, y, textSurface->w, textSurface->h };
	Helper::SDL_RenderCopyEx498(0, "", renderer, textTexture, nullptr, &dstRect, 0.0, nullptr, SDL_FLIP_NONE);

	// Cleanup
	SDL_FreeSurface(textSurface);
	SDL_DestroyTexture(textTexture);
}

void Renderer::RenderTextRequest(TextRenderRequest& text_request) {
	// check font is stored in cache already
	TTF_Font* font = GetFont(text_request.font, text_request.size);
	RenderText(font, text_request.text, text_request.x, text_request.y, text_request.color);
}

TTF_Font* Renderer::GetFont(const std::string& font_name, int font_size) {
	// Check if the font is in cache
	auto it = fonts.find(font_name);
	if (it != fonts.end()) {
		auto it2 = it->second.find(font_size);
		if (it2 != it->second.end()) {
			return it2->second;
		}
	}
	// Font not found, load and store in cache
	CheckFont(font_name);
	std::string fontPath = "resources/fonts/" + font_name + ".ttf";
	TTF_Font* font = TTF_OpenFont(fontPath.c_str(), font_size);
	if (font != nullptr) {
		fonts[font_name][font_size] = font;
	}

	return font;
}



void Renderer::SetCameraPosition(float x, float y) {
	cameraPosition.x = x;
	cameraPosition.y = y;
}

float Renderer::GetCameraPositionX() {
	return cameraPosition.x;
}
float Renderer::GetCameraPositionY() {
	return cameraPosition.y;
}
void Renderer::SetZoomFactor(float z_factor) {
	zoom_factor = z_factor;
}
float Renderer::GetZoomFactor() {
	return zoom_factor;
}

void Renderer::addTextRequest(TextRenderRequest& text_request)
{
	text_render_requests.emplace_back(text_request);
}
