#ifndef SPRITE_RENDERER
#define SPRITE_RENDERER

#include <glad/glad.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include "texture.h"
#include "shader.h"

class SpriteRenderer 
{
    public:
        SpriteRenderer(Shader &Shader);
        ~SpriteRenderer();

        void DrawSprite(Texture2D &Texture, glm::vec2 position, glm::vec2 size = glm::vec2(10.0f, 10.0f), float rotate = 0.0f, 
        glm::vec3 color = glm::vec3(1.0f));
    private:
        Shader shader;
        unsigned int quadVAO;

        void initRenderData();
};
#endif