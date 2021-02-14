// Written By Garson Chow (grc284)
// CS3113 Project 1
// Reference: Class codes

// Including libraries and definitions
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

// Instantiations
SDL_Window* displayWindow;
ShaderProgram program;

bool gameIsRunning = true;

// Matrices
glm::mat4 viewMatrix, playerMatrix, spikeMatrix, projectionMatrix;

// Spike Positions
float spike_x = -5; // x-position
float spike_rotate = 0;

// Player Position
float player_y = 0;
float player_rotate = 0;

// Ticks
float lastTicks = 0.0f; // Amount of time gone by since intitializing SDL

// Texture ID
GLuint spikeTextureID;
GLuint playerTextureID;

// Load Texture Function
GLuint LoadTexture(const char* filePath) {
	int w, h, n;
	unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	stbi_image_free(image);
	return textureID;
}

// Initialization (main)
void Initialize() {
	SDL_Init(SDL_INIT_VIDEO); // Initialize SDL
	displayWindow = SDL_CreateWindow("Project1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL); // Create Window
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow); // Create context
	SDL_GL_MakeCurrent(displayWindow, context); // Draw to current window

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 480); // Tell camera is draw from (0,0) to (640,480)
	
	program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl"); // Load shader program

	// Initialize identity matrix and projection matrix
	viewMatrix = glm::mat4(1.0f);
	playerMatrix = glm::mat4(1.0f);
	spikeMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);

	glUseProgram(program.programID);

	// Load Texture ID
	spikeTextureID = LoadTexture("Star.png");
	playerTextureID = LoadTexture("Swordsman.png");

	// Enabling blending
	glEnable(GL_BLEND);
	// Good setting for transparency
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Background color for clearing screen. Color used when rendering
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
}

// Processing keyboard/mouse events or inputs
void ProcessInput() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) { // Stops program if you quit or close
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			gameIsRunning = false;
		}
	}
}

bool spikeMoveBack = false;
// Updates states of the objects
void Update() {
	// Calculaitng deltatime
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	float deltaTime = ticks - lastTicks;
	lastTicks = ticks;
	
	// Spike behavior
	if (spike_x > 4.8f) {
		spikeMoveBack = true;
	}
	if (spike_x < -4.8f)
		spikeMoveBack = false;

	if (!spikeMoveBack) {
		spike_x += 1.0f * deltaTime;
	}
	else {
		spike_x -= 1.0f * deltaTime;
	}

	spike_rotate += 90.0f * deltaTime;

	// Player behavior
	if (!spikeMoveBack) { // Spike moves to the right
		if (spike_x >= -1 && spike_x <= 0) {
			player_y += 1.5f * deltaTime;
			player_rotate += 180.0f * deltaTime;
		}
		if (spike_x > 0 && spike_x <= 1) {
			player_y -= 1.5f * deltaTime;
			player_rotate += 180.0f * deltaTime;
		}
	}
	else { // Spike moves to the left
		if (spike_x >= -1 && spike_x <= 0) {
			player_y -= 1.5f * deltaTime;
			player_rotate += 180.0f * deltaTime;
		}
		if (spike_x > 0 && spike_x <= 1) {
			player_y += 1.5f * deltaTime;
			player_rotate += 180.0f * deltaTime;
		}
	}

	// Transforming spike matrix based off of spike position
	spikeMatrix = glm::mat4(1.0f); // Initialize identity matrix
	spikeMatrix = glm::translate(spikeMatrix, glm::vec3(spike_x, 0.0f, 0.0f));
	spikeMatrix = glm::rotate(spikeMatrix, glm::radians(spike_rotate), glm::vec3(0.0f, 0.0f, 1.0f));

	// Transforming player matrix
	playerMatrix = glm::mat4(1.0f); // Initialize identity matrix
	playerMatrix = glm::translate(playerMatrix, glm::vec3(0.0f, player_y, 1.0f));
	playerMatrix = glm::rotate(playerMatrix, glm::radians(player_rotate), glm::vec3(0.0f, 0.0f, 1.0f));
}


// Rendering
void Render() {
	glClear(GL_COLOR_BUFFER_BIT);

	float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
	float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);

	// Draw spike
	program.SetModelMatrix(spikeMatrix);
	glBindTexture(GL_TEXTURE_2D, spikeTextureID);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// Draw player
	program.SetModelMatrix(playerMatrix);
	glBindTexture(GL_TEXTURE_2D, playerTextureID);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

	SDL_GL_SwapWindow(displayWindow);
}

// Quit SDL
void Shutdown() {
	SDL_Quit();
}

// Game cyles from ProcessInputs -> Update -> Render
int main(int argc, char* argv[]) {
	Initialize();

	while (gameIsRunning) {
		ProcessInput();
		Update();
		Render();
	}

	Shutdown();
	return 0;
}
