#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <vector>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Entity.h"

#define PLATFORM_COUNT 18
#define ENEMY_COUNT 3
#define WALL_COUNT 2
#define BULLET_COUNT 100
#define LIVES 3

struct GameState {
    Entity* player;
    Entity* enemies;
    Entity* platforms;
    Entity* walls;
    Entity* playerBullets;
    Entity* background;
};

// Mode
enum class GameMode { GAME_LEVEL, GAME_WON, GAME_OVER, GAME_FAIL};
GameState state;
GameMode mode;

// fontTextureID
GLuint fontTextureID;

// Next Bullet Index
int nextBullet = 0;

SDL_Window* displayWindow;
bool gameIsRunning = true;
int lives;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;
GLuint playerTextureID;

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

void setPlayer() {
    // Initialize Player
    state.player = new Entity();
    state.player->entityType = PLAYER;
    state.player->position = glm::vec3(-4.7, 0, 0);
    state.player->facingRight = true;
    state.player->acceleration = glm::vec3(0, -9.81f, 0);
    state.player->jumpPower = 5.5f;
    state.player->speed = 1.5f;
    state.player->height = 0.9f;
    state.player->width = 0.25f;
    state.player->textureID = playerTextureID;

    state.player->animRight = new int[9]{ 143, 144, 145, 146, 147, 148, 149, 150, 151 };
    state.player->animLeft = new int[9]{ 117, 118, 119, 120, 121, 122, 123, 124, 125 };

    state.player->animIndices = state.player->animRight;
    state.player->animFrames = 9;
    state.player->animIndex = 0;
    state.player->animTime = 0;
    state.player->animCols = 13;
    state.player->animRows = 21;
}

void initializeGameObjects() {
    state.platforms = new Entity[PLATFORM_COUNT];
    GLuint platformTextureID = LoadTexture("platformPack_tile001.png");
    GLuint enemyTextureID = LoadTexture("goblin.png");
    GLuint bulletTextureID = LoadTexture("laserBullet.png");
    GLuint backgroundTextureID = LoadTexture("tilesetOpenGameBackground.png");

    // Initialize background entity
    state.background = new Entity();
    state.background->textureID = backgroundTextureID;
    state.background->entityType = BACKGROUND;
    state.background->Update(0, NULL, NULL, 0, NULL, 0, NULL, 0);

    // Initialize Platforms
    for (int i = 0; i < 10; i++) {
        state.platforms[i].entityType = PLATFORM;
        state.platforms[i].textureID = platformTextureID;
        state.platforms[i].position = glm::vec3(-4.5 + i, -3.25, 0);
        state.platforms[i].Update(0, NULL, NULL, 0, NULL, 0, NULL, 0);
    }

    for (int i = 10; i < 13; i++) {
        state.platforms[i].entityType = PLATFORM;
        state.platforms[i].textureID = platformTextureID;
        state.platforms[i].position = glm::vec3(4.5-i+10, -2.25, 0);
        state.platforms[i].Update(0, NULL, NULL, 0, NULL, 0, NULL, 0);
    }

    state.platforms[13].entityType = PLATFORM;
    state.platforms[13].textureID = platformTextureID;
    state.platforms[13].position = glm::vec3(-4.5, -2.25, 0);
    state.platforms[13].Update(0, NULL, NULL, 0, NULL, 0, NULL, 0);
    state.platforms[14].entityType = PLATFORM;
    state.platforms[14].textureID = platformTextureID;
    state.platforms[14].position = glm::vec3(-3.5, -2.25, 0);
    state.platforms[14].Update(0, NULL, NULL, 0, NULL, 0, NULL, 0);
    state.platforms[15].entityType = PLATFORM;
    state.platforms[15].textureID = platformTextureID;
    state.platforms[15].position = glm::vec3(-2.8, -1.15, 0);
    state.platforms[15].Update(0, NULL, NULL, 0, NULL, 0, NULL, 0);
    state.platforms[16].entityType = PLATFORM;
    state.platforms[16].textureID = platformTextureID;
    state.platforms[16].position = glm::vec3(-1.8, -1.15, 0);
    state.platforms[16].Update(0, NULL, NULL, 0, NULL, 0, NULL, 0);
    state.platforms[17].entityType = PLATFORM;
    state.platforms[17].textureID = platformTextureID;
    state.platforms[17].position = glm::vec3(0.8, 0.10, 0);
    state.platforms[17].Update(0, NULL, NULL, 0, NULL, 0, NULL, 0);
    


    //// Player Bullet
    state.playerBullets = new Entity[BULLET_COUNT];
    for (int i = 0; i < BULLET_COUNT; i++) {
        state.playerBullets[i].entityType = BULLET;
        state.playerBullets[i].textureID = bulletTextureID;
        state.playerBullets[i].speed = 3.0f;
        state.playerBullets[i].width = 0.1;
        state.playerBullets[i].height = 0.1;
        state.playerBullets[i].active = false;
    }

    // Initialize enemies
    state.enemies = new Entity[ENEMY_COUNT];

    for (int i = 0; i < ENEMY_COUNT; i++) {
        state.enemies[i].entityType = ENEMY;
        state.enemies[i].textureID = enemyTextureID;
        state.enemies[i].movement = glm::vec3(-1, 0, 0);
        state.enemies[i].acceleration = glm::vec3(0, -9.81f, 0);
        state.enemies[i].jumpPower = 4.0f;
        state.enemies[i].speed = 1.0f;
        state.enemies[i].height = 0.75f;
        state.enemies[i].width = 0.20f;

        state.enemies[i].animRight = new int[6]{ 16, 11, 12, 13, 14, 15 };
        state.enemies[i].animLeft = new int[6]{ 33, 38, 37, 36, 35, 34 };

        state.enemies[i].animIndices = state.enemies[i].animRight;
        state.enemies[i].animFrames = 6;
        state.enemies[i].animIndex = 0;
        state.enemies[i].animTime = 0;
        state.enemies[i].animCols = 11;
        state.enemies[i].animRows = 5;
    }

    state.enemies[0].position = glm::vec3(0, -2.25, 0);
    state.enemies[0].aiType = WALKER;
    state.enemies[1].position = glm::vec3(3.5, -2.25, 0);
    state.enemies[1].movement = glm::vec3(0);
    state.enemies[1].aiType = AGRO;
    state.enemies[2].position = glm::vec3(0.8, 1.5, 0);
    state.enemies[2].movement = glm::vec3(0);
    state.enemies[2].aiType = JUMPER;
    state.enemies[2].animIndices = state.enemies[2].animLeft;
  
    
    // Initialize walls (left and right bounds)
    state.walls = new Entity[WALL_COUNT];
    state.walls[0].position = glm::vec3(-5, 0, 0);
    state.walls[1].position = glm::vec3(5, 0, 0);
    for (int i = 0; i < WALL_COUNT; i++) {
        state.walls[i].width = 0.0f;
        state.walls[i].height = 7.0f;
    }
}

void fire() {
    state.playerBullets[nextBullet].active = true;
    if (state.player->facingRight) {
        state.playerBullets[nextBullet].position = state.player->position + glm::vec3(0.1, -0.4, 0);
        state.playerBullets[nextBullet].movement = glm::vec3(1, 0, 0);
    }
    else {
        state.playerBullets[nextBullet].position = state.player->position + glm::vec3(-0.5, -0.4, 0);
        state.playerBullets[nextBullet].movement = glm::vec3(-1, 0, 0);
    }
}

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Textured!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(0, 0, 640, 480);

    program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

    viewMatrix = glm::mat4(1.0f);
    modelMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);

    glUseProgram(program.programID);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Font Texture ID Initialization
    fontTextureID = LoadTexture("font1.png");

    // Initialize Player
    playerTextureID = LoadTexture("character.png");
    setPlayer();

    // Initialize Game Objects
    initializeGameObjects();

    // Set Game Mode 
    mode = GameMode::GAME_LEVEL;
    lives = LIVES;

}

void ProcessInputGameLevel() {
    state.player->movement = glm::vec3(0);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_LCTRL:
                fire();
                nextBullet = nextBullet + 1;
                if (nextBullet == BULLET_COUNT) nextBullet = 0;
                break;

            case SDLK_RCTRL:
                fire();
                nextBullet = nextBullet + 1;
                if (nextBullet == BULLET_COUNT) nextBullet = 0;
                break;

            case SDLK_SPACE:
                if (state.player->collidedBottom) {
                    state.player->jump = true;
                }
                break;
            }
            break; // SDL_KEYDOWN
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_LEFT]) {
        state.player->movement.x = -1.0f;
        state.player->animIndices = state.player->animLeft;
        state.player->facingRight = false;
    }
    else if (keys[SDL_SCANCODE_RIGHT]) {
        state.player->movement.x = 1.0f;
        state.player->animIndices = state.player->animRight;
        state.player->facingRight = true;
    }


    if (glm::length(state.player->movement) > 1.0f) {
        state.player->movement = glm::normalize(state.player->movement);
    }

    if (state.player->active == false) {
        lives -= 1;
        if (lives == 0) mode = GameMode::GAME_OVER;
        else mode = GameMode::GAME_FAIL;
    }
}

void ProcessInputGameFail() {
    state.player->movement = glm::vec3(0);

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
                mode = GameMode::GAME_LEVEL;
                setPlayer();
                initializeGameObjects();
                break;
            }
            break; // SDL_KEYDOWN
        }
    }
}

void ProcessInputGameWon() {
    state.player->movement = glm::vec3(0);

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
                // Initialize Game Objects
                setPlayer();
                initializeGameObjects();

                // Set Game Mode
                mode = GameMode::GAME_LEVEL;
                lives = LIVES;
                break;
            }
            break; // SDL_KEYDOWN
        }
    }
}

void ProcessInputGameOver() {
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

void updateEntities() {
    switch (mode) {
    case GameMode::GAME_LEVEL:
        float ticks = (float)SDL_GetTicks() / 1000.0f;
        float deltaTime = ticks - lastTicks;
        lastTicks = ticks;

        deltaTime += accumulator;
        if (deltaTime < FIXED_TIMESTEP) {
            accumulator = deltaTime;
            return;
        }

        while (deltaTime >= FIXED_TIMESTEP) {
            // Update. Notice it's FIXED_TIMESTEP. Not deltaTime
            state.player->Update(FIXED_TIMESTEP, NULL, state.enemies, ENEMY_COUNT, state.walls, WALL_COUNT, state.platforms, PLATFORM_COUNT);

            for (int i = 0; i < BULLET_COUNT; i++) {
                state.playerBullets[i].Update(FIXED_TIMESTEP, NULL, state.enemies, ENEMY_COUNT, state.walls, WALL_COUNT, state.platforms, PLATFORM_COUNT);
            }

            for (int i = 0; i < ENEMY_COUNT; i++) {
                state.enemies[i].Update(FIXED_TIMESTEP, state.player, NULL, 0, state.walls, WALL_COUNT, state.platforms, PLATFORM_COUNT);
            }

            deltaTime -= FIXED_TIMESTEP;
        }

        accumulator = deltaTime;

        bool allDead = true;

        for (int i = 0; i < ENEMY_COUNT; i++) {
            if (state.enemies[i].active == true) allDead = false;
        }

        if (allDead) mode = GameMode::GAME_WON;
        break;
    }
}

void Update() {
    switch (mode) {
    case GameMode::GAME_LEVEL:
        updateEntities();
        break;
    case GameMode::GAME_FAIL:
        updateEntities();
        break;
    case GameMode::GAME_WON:
        updateEntities();
        break;
    case GameMode::GAME_OVER:
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

    // Draw Player Bullets
    for (int i = 0; i < BULLET_COUNT; i++) {
        state.playerBullets[i].Render(&program);
    }
    // Draw Enemies
    for (int i = 0; i < ENEMY_COUNT; i++) {
        state.enemies[i].Render(&program);
    }

    // Draw Player
    state.player->Render(&program);

    // Draw Lives
    DrawText(&program, fontTextureID, "Lives: " + std::to_string(lives), 0.5f, -0.25f,
        glm::vec3(-4.55f, 3.3, 0));

    // Draw Control
    DrawText(&program, fontTextureID, "Shoot:[Ctrl]", 0.47f, -0.25f,
        glm::vec3(2.1f, 2.7, 0));
    DrawText(&program, fontTextureID, "Jump:[Space]", 0.47f, -0.25f,
        glm::vec3(2.1f, 3.3, 0));
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    switch (mode) {
        case GameMode::GAME_LEVEL:
            RenderEntities();
            break;
        case GameMode::GAME_FAIL:
            RenderEntities();
            DrawText(&program, fontTextureID, "You Died!", 0.8f, -0.4f, glm::vec3(-1.75, 0, 0));
            DrawText(&program, fontTextureID, "Retry: [Press R]", 0.5f, -0.25f, glm::vec3(-2.05, -0.7, 0));
            break;
        case GameMode::GAME_WON:
            RenderEntities();
            DrawText(&program, fontTextureID, "You Win!", 0.8f, -0.4f, glm::vec3(-1.50, 0, 0));
            DrawText(&program, fontTextureID, "Play Again: [Press SPACE]", 0.5f, -0.25f, glm::vec3(-3.00, -0.7, 0));
            break;
        case GameMode::GAME_OVER:
            DrawText(&program, fontTextureID, "GAME OVER", 1.0f, -0.5f, glm::vec3(-2.05, 0, 0));
            break;
    }
    

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