#include "game_level.h"

#include <fstream>
#include <sstream>


void GameLevel::Load(const char* file, unsigned int levelWidth, unsigned int levelHeight) 
{
    this->Bricks.clear();

    unsigned int tileCode;
    GameLevel level;
    std::string line;
    std::ifstream fstream(file);
    std::vector<std::vector<unsigned int>> tileData;

    if (fstream) 
    {
        while (std::getline(fstream, line)) 
        {
            std::istringstream sstream(line);
            std::vector<unsigned int> row;
            while (sstream >> tileCode)
            {
                row.push_back(tileCode);
            }
            tileData.push_back(row);
        }
        if (tileData.size() > 0)
        {
            this -> init(tileData, levelWidth, levelHeight);
        }            
    }
}

void GameLevel::Draw(SpriteRenderer &renderer) 
{
    for(GameObject &tile : this -> Bricks) 
        if(!tile.Destroyed)
            tile.Draw(renderer);
}

bool GameLevel::IsCompleted() 
{
    for(GameObject &tile : this -> Bricks)
        if(!tile.IsSolid && !tile.Destroyed)
            return false;

    return true;
}

void GameLevel::init(std::vector<std::vector<unsigned int>> tileData, unsigned int lvlWidth, unsigned int lvlHeight)
{
    unsigned int height = tileData.size();
    unsigned int width = tileData[0].size();
    float unit_width = lvlWidth / static_cast<float>(width);
    float unit_height = lvlHeight / height;

    for (unsigned int j = 0; j < height; j++) 
    {
        for (unsigned int i = 0; i < width; i++) 
        {
            if(tileData[j][i] == 1) 
            {
                glm::vec2 pos(unit_width * i, unit_height * j);
                glm::vec2 size(unit_width, unit_height);
                GameObject obj(pos, size, ResourceManager::GetTexture("block_solid"), glm::vec3(0.8f, 0.8f, 0.7f));
                obj.IsSolid = true;
                this -> Bricks.push_back(obj);

            }
            else if (tileData[j][i] > 1)
            {
                glm::vec3 color = glm::vec3(1.0f);
                if (tileData[j][i] == 2)
                    color = glm::vec3(0.2f, 0.6f, 1.0f);
                else if (tileData[j][i] == 3)
                    color = glm::vec3(0.0f, 0.7f, 0.0f);
                else if (tileData[j][i] == 4)
                    color = glm::vec3(0.8f, 0.8f, 0.4f);
                else if (tileData[j][i] == 5)
                    color = glm::vec3(1.0f, 0.5f, 0.0f);
                
                glm::vec2 pos(unit_width * i, unit_height * j);
                glm::vec2 size(unit_width, unit_height);
                this -> Bricks.push_back(GameObject(pos, size, ResourceManager::GetTexture("block"), color));
            }
        }

    }  
}