#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define GL_GLEXT_PROTOTYPES 1
#define PLATFORM_COUNT 33
#define OUTER_WALL_COUNT 25
#define LIVES 3
#define FUEL 150.0

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "Entity.h"
#include <vector>
#include <iomanip>
#include <sstream>


// Game is Running
bool gameIsRunning = true;
int lives;
float fuel;

// Declaring Global Variables For Window
SDL_Window* displayWindow;
ShaderProgram program;
glm::mat4 viewMatrix, projectionMatrix;

// Structure of GameState 
struct GameState {
    Entity* ship;
    Entity* platforms;
    Entity* background;
};

// State and mode
enum class GameMode {GAME_LEVEL, GAME_FAIL, GAME_WON, GAME_OVER,};
GameState state;
GameMode mode;

// fontTextureID
GLuint fontTextureID;

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

void DrawText(ShaderProgram* program, GLuint fontTextureID, std::string text,
    float size, float spacing, glm::vec3 position)
{
    float width = 1.0f / 16.0f;
    float height = 1.0f / 16.0f;

    std::vector<float> vertices;
    std::vector<float> texCoords;

    for (int i = 0; i < text.size(); i++) {

        int index = (int)text[i];
        float offset = (size + spacing) * i;

        float u = (float)(index % 16) / 16.0f;
        float v = (float)(index / 16) / 16.0f;

        vertices.insert(vertices.end(), {
            offset + (-0.5f * size), 0.5f * size,
            offset + (-0.5f * size), -0.5f * size,
            offset + (0.5f * size), 0.5f * size,
            offset + (0.5f * size), -0.5f * size,
            offset + (0.5f * size), 0.5f * size,
            offset + (-0.5f * size), -0.5f * size,
            });
        texCoords.insert(texCoords.end(), {
            u, v,
            u, v + height,
            u + width, v,
            u + width, v + height,
            u + width, v,
            u, v + height,
            });

    } // end of for loop

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    program->SetModelMatrix(modelMatrix);

    glUseProgram(program->programID);

    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->positionAttribute);

    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
    glEnableVertexAttribArray(program->texCoordAttribute);

    glBindTexture(GL_TEXTURE_2D, fontTextureID);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void LoopOuterWalls(GLuint textureID) {
    for (int i = 0; i < OUTER_WALL_COUNT; i++) {
        if (i <= 9) state.platforms[i].position = glm::vec3(i - 4.5f, -3.25f, 0);
        else if (i <= 16) state.platforms[i].position = glm::vec3(-4.5f, -2.25f + i - 10, 0);
        else state.platforms[i].position = glm::vec3(4.5f, -2.25f + i - 17, 0);
        state.platforms[i].textureID = textureID;
        state.platforms[i].entityType = PLATFORM;
        if (i == 7) {
            state.platforms[i].textureID = LoadTexture("platformPack_tile029.png");
            state.platforms[i].entityType = GOLD_PLATFORM;
            state.platforms[i].position = glm::vec3(i - 4.5f, -3.00f, 0);
        }
        state.platforms[i].Update(0, NULL, 0);
    }
}


void setLunarLanderTiles() {
    GLuint platformTextureID = LoadTexture("platformPack_tile007.png");
    GLuint backgroundTextureID = LoadTexture("tilesetOpenGameBackground.png");
  
    // New array of platform entities
    state.platforms = new Entity[PLATFORM_COUNT];

    // Loop and Intialize Platforms
    LoopOuterWalls(platformTextureID);

    // Initialize Middle Platforms
    state.platforms[OUTER_WALL_COUNT].textureID = platformTextureID;
    state.platforms[OUTER_WALL_COUNT].position = glm::vec3(-3.5, 2.25f, 0);
    state.platforms[OUTER_WALL_COUNT+1].textureID = platformTextureID;
    state.platforms[OUTER_WALL_COUNT+1].position = glm::vec3(-2.5, 2.25f, 0);
    state.platforms[OUTER_WALL_COUNT+2].textureID = platformTextureID;
    state.platforms[OUTER_WALL_COUNT+2].position = glm::vec3(-1.5, 2.25f, 0);
    state.platforms[OUTER_WALL_COUNT+3].textureID = platformTextureID;
    state.platforms[OUTER_WALL_COUNT+3].position = glm::vec3(1.7, 2.25f, 0);
    state.platforms[OUTER_WALL_COUNT+4].textureID = platformTextureID;
    state.platforms[OUTER_WALL_COUNT+4].position = glm::vec3(2.7, 2.25f, 0);
    state.platforms[OUTER_WALL_COUNT+5].textureID = platformTextureID;
    state.platforms[OUTER_WALL_COUNT+5].position = glm::vec3(-1.8, -0.35f, 0);
    state.platforms[OUTER_WALL_COUNT+6].textureID = platformTextureID;
    state.platforms[OUTER_WALL_COUNT+6].position = glm::vec3(-0.8, -0.35f, 0);
    state.platforms[OUTER_WALL_COUNT+7].textureID = platformTextureID;
    state.platforms[OUTER_WALL_COUNT+7].position = glm::vec3(0.2, -0.35f, 0);

    for (int i = OUTER_WALL_COUNT; i < PLATFORM_COUNT; i++) {
        state.platforms[i].entityType = PLATFORM;
        state.platforms[i].Update(0, NULL, 0);
    }

    // Initialize background entity
    state.background = new Entity();
    state.background->textureID = backgroundTextureID;
    state.background->entityType = BACKGROUND;
    state.background->Update(0, NULL, 0);
}

void setShip() {
    // Initialize ship (player)
    state.ship = new Entity();
    state.ship->position = glm::vec3(2, 4.0f, 0);
    state.ship->acceleration = glm::vec3(0, -0.025f, 0);
    state.ship->speed = 1.0f;
    state.ship->textureID = LoadTexture("spaceship.png");

    state.ship->width = 0.90f;
    state.ship->height = 0.90f;
}

void Initialize() {
    // Initialize window
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Lunar Lander", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif
    // View from (0,0) to (640,480)
    glViewport(0, 0, 640, 480);
    viewMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    // Load shaders and setup
    program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    program.SetColor(1.0f, 0.0f, 0.0f, 1.0f);

    glUseProgram(program.programID);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Font Texture ID Initialization
    fontTextureID = LoadTexture("font1.png");

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initial Game Objects
    setLunarLanderTiles();
    setShip();

    // Set Game Mode
    mode = GameMode::GAME_LEVEL;
    lives = LIVES;
    fuel = FUEL;
}

void ProcessInputGameLevel() {
    state.ship->acceleration.x = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_LEFT] && fuel > 0) {
        state.ship->acceleration.x = -0.2f;
        fuel -= 0.1f;
    }
    else if (keys[SDL_SCANCODE_RIGHT] && fuel > 0) {
        state.ship->acceleration.x = 0.2f;
        fuel -= 0.1f;
    }
}

void ProcessInputGameFail() {
    state.ship->acceleration.x = 0;
    state.ship->velocity.x = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_r:
                // Restart gameLevel
                mode = GameMode::GAME_LEVEL;
                fuel = 200.0f;
                setShip();
                break;
            }
            break; // SDL_KEYDOWN
        }
    }
}

void ProcessInputGameWon() {
    state.ship->acceleration.x = 0;
    state.ship->velocity.x = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_SPACE:
                // Initial Game Objects
                setLunarLanderTiles();
                setShip();

                // Set Game Mode
                mode = GameMode::GAME_LEVEL;
                lives = LIVES;
                fuel = FUEL;
                break;
            }
            break; // SDL_KEYDOWN
        }
    }
}

void ProcessInputGameOver() {
    state.ship->acceleration.x = 0;
    state.ship->velocity.x = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;
        }
    }
}

void ProcessInput() {

    switch (mode) {
    case GameMode::GAME_LEVEL:
        ProcessInputGameLevel();
        break;
    case GameMode::GAME_FAIL:
        ProcessInputGameFail();
        break;
    case GameMode::GAME_WON:
        ProcessInputGameWon();
        break;
    case GameMode::GAME_OVER:
        ProcessInputGameOver();
        break;
    }

}

#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;
void Update() {
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;

    switch (mode) {
    case GameMode::GAME_LEVEL:
        deltaTime += accumulator;
        if (deltaTime < FIXED_TIMESTEP) {
            accumulator = deltaTime;
            return;
        }

        while (deltaTime >= FIXED_TIMESTEP) {
            // Update. Notice it's FIXED_TIMESTEP. Not deltaTime
            state.ship->Update(FIXED_TIMESTEP, state.platforms, PLATFORM_COUNT);

            deltaTime -= FIXED_TIMESTEP;
        }

        accumulator = deltaTime;
        // When ship touches platforms
        if (state.ship->lastCollision == PLATFORM) {
            lives -= 1;
            if (lives == 0) {
                mode = GameMode::GAME_OVER;
            }
            else mode = GameMode::GAME_FAIL;
        }
        else if (state.ship->lastCollision == GOLD_PLATFORM) {
            mode = GameMode::GAME_WON;
        }
        break;
    }

}

void RenderEntities() {
    // Draw Background
    state.background->Render(&program);

    // Draw Tiles
    for (int i = 0; i < PLATFORM_COUNT; i++) {
        state.platforms[i].Render(&program);
    }

    // Draw Lives

    DrawText(&program, fontTextureID, "Lives: " + std::to_string(lives), 0.5f, -0.25f,
        glm::vec3(-4.55f, 3.3, 0));

    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << fuel;
    std::string s = stream.str();

    if (fuel <= 0) s = "Empty";

    DrawText(&program, fontTextureID, "Fuel:[" + s + "]", 0.5f, -0.25f,
        glm::vec3(1.55f, 3.3, 0));

    // Draw Player
    state.ship->Render(&program);
}

void Render() {
    // Clear
    glClear(GL_COLOR_BUFFER_BIT);

    switch (mode) {
    case GameMode::GAME_LEVEL:
        RenderEntities();
        break;
    case GameMode::GAME_FAIL:
        RenderEntities();
        DrawText(&program, fontTextureID, "Mission Failed!", 0.8f, -0.4f, glm::vec3(-2.65, 0, 0));
        DrawText(&program, fontTextureID, "Retry: [Press R]", 0.5f, -0.25f, glm::vec3(-1.85, -0.7, 0));
        break;
    case GameMode::GAME_WON:
        RenderEntities();
        DrawText(&program, fontTextureID, "Mission Successful!", 0.8f, -0.4f, glm::vec3(-3.45, 0, 0));
        DrawText(&program, fontTextureID, "Play Again: [Press SPACE]", 0.5f, -0.25f, glm::vec3(-3.00, -0.7, 0));
        break;
    case GameMode::GAME_OVER:
        DrawText(&program, fontTextureID, "GAME OVER", 1.0f, -0.5f, glm::vec3(-2.05, 0, 0));
        break;
    }

    // Swap
    SDL_GL_SwapWindow(displayWindow);
}

void Shutdown() {
    SDL_Quit();
}

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
