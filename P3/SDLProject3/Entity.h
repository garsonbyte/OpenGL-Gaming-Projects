#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

enum EntityType { PLATFORM, GOLD_PLATFORM, BACKGROUND };

class Entity {
public:
    EntityType entityType;
    EntityType lastCollision;

    // Dimension
    float width = 1.0f;
    float height = 1.0f;

    // Model Matrix
    glm::mat4 modelMatrix;

    // Texture ID
    GLuint textureID;

    // Movements 
    glm::vec3 position;
    glm::vec3 acceleration;
    glm::vec3 velocity;
    float speed;

    Entity();

    bool CheckCollision(Entity* other);
    void CheckCollisionsY(Entity* objects, int objectCount);
    void CheckCollisionsX(Entity* objects, int objectCount);
    void Update(float deltaTime, Entity* platforms, int platformCount);
    void Render(ShaderProgram* program);

};