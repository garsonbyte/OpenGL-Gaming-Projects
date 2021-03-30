#include "Entity.h"

Entity::Entity()
{
    position = glm::vec3(0);
    movement = glm::vec3(0);
    acceleration = glm::vec3(0);
    velocity = glm::vec3(0);
    speed = 0;

    modelMatrix = glm::mat4(1.0f);
}

bool Entity::CheckSensorCollision(Entity* other, glm::vec3 sensor) {
    if (active == false || other->active == false) return false;
    float xdist = fabs(sensor.x - other->position.x) - ((width + other->width) / 2.0f);
    float ydist = fabs(sensor.y - other->position.y) - ((height + other->height) / 2.0f);;

    if (xdist < 0 && ydist < 0) {
        return true;
    }

    return false;
}

bool Entity::CheckCollision(Entity* other) {
    if (active == false || other->active == false) return false;
    float xdist = fabs(position.x - other->position.x) - ((width + other->width) / 2.0f);
    float ydist = fabs(position.y - other->position.y) - ((height + other->height) / 2.0f);

    if (xdist < 0 && ydist < 0) {
        return true;
    }

    return false;
}

void Entity::CheckCollisionsY(Entity* objects, int objectCount)
{
    for (int i = 0; i < objectCount; i++)
    {
        Entity* object = &objects[i];

        if (CheckCollision(object))
        {
            float ydist = fabs(position.y - object->position.y);
            float penetrationY = fabs(ydist - (height / 2.0f) - (object->height / 2.0f));
            if (velocity.y > 0) {
                position.y -= penetrationY;
                velocity.y = 0;
                collidedTop = true;
            }
            else if (velocity.y < 0) {
                position.y += penetrationY;
                velocity.y = 0;
                collidedBottom = true;
            }
        }
    }
}

void Entity::CheckCollisionsX(Entity* objects, int objectCount)
{
    for (int i = 0; i < objectCount; i++)
    {
        Entity* object = &objects[i];

        if (CheckCollision(object))
        {
            float xdist = fabs(position.x - object->position.x);
            float penetrationX = fabs(xdist - (width / 2.0f) - (object->width / 2.0f));
            if (velocity.x > 0) {
                position.x -= penetrationX;
                velocity.x = 0;
                collidedRight = true;
            }
            else if (velocity.x < 0) {
                position.x += penetrationX;
                velocity.x = 0;
                collidedLeft = true;
            }
            if (entityType == ENEMY) movement *= -1;
        }
    }
}

void Entity::AIWalker() {
}

void Entity::AIWaitAndGo(Entity* player) {
    switch (aiState) {
    case IDLE:
        if (glm::distance(position, player->position) < 3.0f) {
            aiState = CHASING;
        }
        break;
    case CHASING:
        if (player->position.x < position.x) {
            movement = glm::vec3(-1, 0, 0);
        }
        else {
            movement = glm::vec3(1, 0, 0);
        }
        break;
    }
}

void Entity::AIJumper() {
    if (collidedBottom == true) {
        jump = true;
    }
}


void Entity::AI(Entity* player, Entity* platforms, int platformCount) {
    bool leftsensorCollided = false;
    bool rightsensorCollided = false;
    if (movement.x > 0) animIndices = animRight;
    else if (movement.x < 0) animIndices = animLeft;

    glm::vec3 sensorRight = glm::vec3(position.x + 0.6f, position.y - 0.6f, 0);
    glm::vec3 sensorLeft = glm::vec3(position.x - 0.6f, position.y - 0.6f, 0);

    for (int i = 0; i < platformCount; i++)
    {
        Entity* object = &platforms[i];
        if (CheckSensorCollision(object, sensorLeft))
            leftsensorCollided = true;
    }

    for (int i = 0; i < platformCount; i++)
    {
        Entity* object = &platforms[i];
        if (CheckSensorCollision(object, sensorRight))
            rightsensorCollided = true;
    }

    if (leftsensorCollided == false || rightsensorCollided == false) movement *= -1;
    switch (aiType) {
    case WALKER:
        AIWalker(); // Do Nothing
        break;

    case AGRO:
        AIWaitAndGo(player);
        break;
    case JUMPER:
        AIJumper(); // Just Jump
        break;
    }
}

void Entity::Update(float deltaTime, Entity* player, Entity* enemies, int enemyCount, Entity* walls, int wallCount, Entity* platforms, int platformCount)
{
    if (active == false) return;

    if (entityType == ENEMY) {
        AI(player, platforms, platformCount);
    }

    collidedTop = false;
    collidedBottom = false;
    collidedLeft = false;
    collidedRight = false;

    if (position.y <= -5) active = false;

    if (entityType == PLAYER) {
        CheckCollisionsX(walls, wallCount);
        for (int i = 0; i < enemyCount; i++) {
            if (CheckCollision(&enemies[i])) {
                alive = false;
                rotate = 5;
            }
        }
    }

    if (entityType == BULLET) {
        for (int i = 0; i < enemyCount; i++) {
            if (CheckCollision(&enemies[i])) {
                enemies[i].alive = false;
                enemies[i].rotate = 5;
                active = false;
            }
        }
    }

    if (animIndices != NULL) {
        if (glm::length(movement) != 0) {
            animTime += deltaTime;

            if (animTime >= 0.25f)
            {
                animTime = 0.0f;
                animIndex++;
                if (animIndex >= animFrames)
                {
                    animIndex = 0;
                }
            }
        }
        else {
            animIndex = 0;
        }
    }

    if (jump)
    {
        jump = false;
        velocity.y += jumpPower;
    }

    velocity.x = movement.x * speed;
    velocity += acceleration * deltaTime;

    position.y += velocity.y * deltaTime; // Move on Y
    if (entityType != BULLET && alive) CheckCollisionsY(platforms, platformCount);

    position.x += velocity.x * deltaTime; // Move on X
    if (entityType != BULLET && alive) CheckCollisionsX(platforms, platformCount);

    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, rotate, glm::vec3(0, 0, 1));

}

void Entity::DrawSpriteFromTextureAtlas(ShaderProgram* program, GLuint textureID, int index)
{
    float u = (float)(index % animCols) / (float)animCols;
    float v = (float)(index / animCols) / (float)animRows;

    float width = 1.0f / (float)animCols;
    float height = 1.0f / (float)animRows;

    float texCoords[] = { u, v + height, u + width, v + height, u + width, v,
        u, v + height, u + width, v, u, v };

    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };

    glBindTexture(GL_TEXTURE_2D, textureID);

    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);

    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program->texCoordAttribute);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void Entity::Render(ShaderProgram* program) {
    if (active == false) return;

    program->SetModelMatrix(modelMatrix);

    if (animIndices != NULL) {
        DrawSpriteFromTextureAtlas(program, textureID, animIndices[animIndex]);
        return;
    }
    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float backVertices[] = { -5, -3.75, 5, -3.75, 5, 3.75, -5, -3.75, 5, 3.75, -5, 3.75 };
    if (entityType == BULLET) {
        float vertices[] = { -0.05, -0.05, 0.05, -0.05, 0.05, 0.05, -0.05, -0.05, 0.05, 0.05, -0.05, 0.05 };
    } 

    float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

    glBindTexture(GL_TEXTURE_2D, textureID);

    if (entityType == BACKGROUND) {
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, backVertices);
    }
    else {
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }
    glEnableVertexAttribArray(program->positionAttribute);

    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program->texCoordAttribute);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}