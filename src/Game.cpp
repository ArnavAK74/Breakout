#include "game.h"
#include "sprite_renderer.h"
#include "resourceManager.h"
#include "game_object.h"
#include "ball_object.h"
#include "particle_gen.h"
#include "post_processor.h"
#include "power_up.h"
#include "sound_engine.h"
#include "text_renderer.h"

#include <iostream>
#include <sstream>
#include <algorithm>

SpriteRenderer *Renderer;
GameObject *Player;
BallObject *Ball;
ParticleGenerator   *Particles; 
PostProcessor *Effects;
SoundEngine *SoundDevice;
TextRenderer  *Text;


float ShakeTime = 0.0f;

Game::Game(unsigned int width, unsigned int height) 
    : State(GAME_MENU), Keys(), KeysProcessed(), Width(width), Height(height), Level(0), Lives(3)
{ 

}

Game::~Game()
{
    delete(Renderer);
    delete(Player);
    delete(Ball);
    delete(Particles);
    delete(Effects);
    delete(SoundDevice);
}

void Game::Init()
{
    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.fs", nullptr, "sprite");
    ResourceManager::LoadShader("shaders/particle.vs", "shaders/particle.fs", nullptr, "particle");
    ResourceManager::LoadShader("shaders/post_processing.vs", "shaders/post_processing.fs", nullptr, "postprocessing");

    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width), static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    ResourceManager::GetShader("particle").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("particle").SetMatrix4("projection", projection);

    ResourceManager::LoadTexture("textures/powerup_speed.png", true, "powerup_speed");
    ResourceManager::LoadTexture("textures/powerup_sticky.png", true, "powerup_sticky");
    ResourceManager::LoadTexture("textures/powerup_increase.png", true, "powerup_increase");
    ResourceManager::LoadTexture("textures/powerup_confuse.png", true, "powerup_confuse");
    ResourceManager::LoadTexture("textures/powerup_chaos.png", true, "powerup_chaos");
    ResourceManager::LoadTexture("textures/powerup_passthrough.png", true, "powerup_passthrough");
   
    ResourceManager::LoadTexture("textures/background.jpg", false, "background");
    ResourceManager::LoadTexture("textures/awesomeface.png", true, "face");
    ResourceManager::LoadTexture("textures/block.png", false, "block");
    ResourceManager::LoadTexture("textures/block_solid.png", false, "block_solid");
    ResourceManager::LoadTexture("textures/paddle.png", true, "paddle");
    ResourceManager::LoadTexture("textures/particle.png", true, "particle");

    Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
    Particles = new ParticleGenerator(ResourceManager::GetShader("particle"), ResourceManager::GetTexture("particle"), 500);
    Effects = new PostProcessor(ResourceManager::GetShader("postprocessing"), this->Width*2, this->Height*2);
    SoundDevice = new SoundEngine();
    Text = new TextRenderer(this->Width, this->Height);

    GameLevel one; one.Load("levels/one.lvl", this->Width, this->Height / 2);
    GameLevel two; two.Load("levels/two.lvl", this->Width, this->Height / 2);
    GameLevel three; three.Load("levels/three.lvl", this->Width, this->Height / 2);
    GameLevel four; four.Load("levels/four.lvl", this->Width, this->Height / 2);
    this->Levels.push_back(one);
    this->Levels.push_back(two);
    this->Levels.push_back(three);
    this->Levels.push_back(four);
    this->Level = 0;

    glm::vec2 playerPos = glm::vec2((this->Width - PLAYER_SIZE.x)/2.0f, this->Height - PLAYER_SIZE.y);
    Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));   

    glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -BALL_RADIUS * 2.0f);
    Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("face"));

    SoundDevice->playBackgroundMusic("audio/8-bit-arcade-mode-158814.mp3");
    SoundDevice->setBackgroundMusicVolume(0.5f);

    Text->Load("fonts/OCRAEXT.TTF", 24);
}

void Game::Update(float dt)
{
    Ball->Move(dt, this->Width);

    this->DoCollisions();

    Particles->Update(dt, *Ball, 2, glm::vec2(Ball->Radius / 2.0f));

    this->UpdatePowerUps(dt);

    if (ShakeTime > 0.0f)
    {
        ShakeTime -= dt;
        if (ShakeTime <= 0.0f)
            Effects->Shake = false;
    }

    if (Ball->Position.y >= this->Height)
    {
        --this->Lives;
        if (this->Lives == 0)
        {
            this->ResetLevel();
            this->State = GAME_MENU;
        }
        this->ResetPlayer();
    }

    if (this->State == GAME_ACTIVE && this->Levels[this->Level].IsCompleted())
    {
        this->ResetLevel();
        this->ResetPlayer();
        Effects->Chaos = true;
        this->State = GAME_WIN;
    }
}

void Game::ProcessInput(float dt)
{
    if (this->State == GAME_MENU)
    {
        if (this->Keys[GLFW_KEY_ENTER] && !this->KeysProcessed[GLFW_KEY_ENTER])
        {
            this->State = GAME_ACTIVE;
            this->KeysProcessed[GLFW_KEY_ENTER] = true;
        }
        if (this->Keys[GLFW_KEY_W] && !this->KeysProcessed[GLFW_KEY_W])
        {
            this->Level = (this->Level + 1) % 4;
            this->KeysProcessed[GLFW_KEY_W] = true;
        }
        if (this->Keys[GLFW_KEY_S] && !this->KeysProcessed[GLFW_KEY_S])
        {
            if (this->Level > 0)
                --this->Level;
            else
                this->Level = 3;
            this->KeysProcessed[GLFW_KEY_S] = true;
        }
    }

    if (this->State == GAME_WIN)
    {
        if (this->Keys[GLFW_KEY_ENTER])
        {
            this->KeysProcessed[GLFW_KEY_ENTER] = true;
            Effects->Chaos = false;
            this->State = GAME_MENU;
        }
    }

    if (this->State == GAME_ACTIVE)
    {
        float ds = PLAYER_VELOCITY * dt;
        if (this->Keys[GLFW_KEY_A] || this->Keys[GLFW_KEY_LEFT])
        {
            if (Player->Position.x >= 0.0f)
            {
                Player->Position.x -= ds;
                if(Ball->Stuck) 
                {
                    Ball->Position.x -= ds;
                }
            }                
        }
        if (this->Keys[GLFW_KEY_D] || this->Keys[GLFW_KEY_RIGHT])
        {
            if (Player->Position.x <= this->Width - Player->Size.x)
            {
                Player->Position.x += ds;
                if(Ball->Stuck)
                {
                    Ball->Position.x += ds;
                }
            }                
        }

        if(this->Keys[GLFW_KEY_SPACE])
        {
            Ball->Stuck = false;
        }

    }
   
}

void Game::Render()
{
    if (this->State == GAME_ACTIVE || this->State == GAME_MENU || this->State == GAME_WIN)
    {       
        Effects->BeginRender();

        Renderer->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f); 
        this->Levels[this->Level].Draw(*Renderer);
        Player->Draw(*Renderer);
    
        for (PowerUp &powerUp : this->PowerUps)
            if (!powerUp.Destroyed)
                powerUp.Draw(*Renderer); 

        Particles->Draw();
        Ball->Draw(*Renderer);

        Effects->EndRender();
        Effects->Render(glfwGetTime());

        std::stringstream ss; ss << this->Lives;
        Text->RenderText("Lives:" + ss.str(), 5.0f, 5.0f, 1.0f);
        
    }
    if (this->State == GAME_MENU)
    {
        Text->RenderText("Press ENTER to start", 250.0f, this->Height / 2.0f, 1.0f);
        Text->RenderText("Press W or S to select level", 245.0f, this->Height / 2.0f + 20.0f, 0.75f);
    }
    if (this->State == GAME_WIN)
    {
        Text->RenderText("You WON!!!", 320.0f, this->Height / 2.0f - 20.0f, 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        Text->RenderText("Press ENTER to retry or ESC to quit", 130.0f, this->Height / 2.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));
    }   
       
}

//PowerUps

bool IsOtherPowerUpActive(std::vector<PowerUp> &powerUps, std::string type)
{
    for (const PowerUp &powerUp : powerUps)
    {
        if (powerUp.Activated)
            if (powerUp.Type == type)
                return true;
    }
    return false;
}

bool ShouldSpawn(unsigned int chance)
{
    unsigned int random = rand() % chance;
    return random == 0;
}

void Game::SpawnPowerUps(GameObject &block)
{
    if (ShouldSpawn(75)) 
        this->PowerUps.push_back(PowerUp("speed", glm::vec3(0.5f, 0.5f, 1.0f), 0.0f, block.Position, ResourceManager::GetTexture("powerup_speed")));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("sticky", glm::vec3(1.0f, 0.5f, 1.0f), 20.0f, block.Position, ResourceManager::GetTexture("powerup_sticky")));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("pass-through", glm::vec3(0.5f, 1.0f, 0.5f), 10.0f, block.Position, ResourceManager::GetTexture("powerup_passthrough")));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("pad-size-increase", glm::vec3(1.0f, 0.6f, 0.4), 0.0f, block.Position, ResourceManager::GetTexture("powerup_increase")));
    if (ShouldSpawn(15)) // Negative powerups should spawn more often
        this->PowerUps.push_back(PowerUp("confuse", glm::vec3(1.0f, 0.3f, 0.3f), 15.0f, block.Position, ResourceManager::GetTexture("powerup_confuse")));
    if (ShouldSpawn(15))
        this->PowerUps.push_back(PowerUp("chaos", glm::vec3(0.9f, 0.25f, 0.25f), 15.0f, block.Position, ResourceManager::GetTexture("powerup_chaos")));
}



void ActivatePowerUp(PowerUp &powerUp)
{
    if (powerUp.Type == "speed")
    {
        Ball->Velocity *= 1.2;
    }
    else if (powerUp.Type == "sticky")
    {
        Ball->Sticky = true;
        Player->Color = glm::vec3(1.0f, 0.5f, 1.0f);
    }
    else if (powerUp.Type == "pass-through")
    {
        Ball->PassThrough = true;
        Ball->Color = glm::vec3(1.0f, 0.5f, 0.5f);
    }
    else if (powerUp.Type == "pad-size-increase")
    {
        Player->Size.x += 50;
    }
    else if (powerUp.Type == "confuse")
    {
        if (!Effects->Chaos)
            Effects->Confuse = true;
    }
    else if (powerUp.Type == "chaos")
    {
        if (!Effects->Confuse)
            Effects->Chaos = true;
    }
}

void Game::UpdatePowerUps(float dt)
{
    for (PowerUp &powerUp : this->PowerUps)
    {
        powerUp.Position += powerUp.Velocity * dt;
        if (powerUp.Activated)
        {
            powerUp.Duration -= dt;

            if (powerUp.Duration <= 0.0f)
            {
                powerUp.Activated = false;
                // deactivate effects
                if (powerUp.Type == "sticky")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "sticky"))
                    {	
                        Ball->Sticky = false;
                        Player->Color = glm::vec3(1.0f);
                    }
                }
                else if (powerUp.Type == "pass-through")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "pass-through"))
                    {	
                        Ball->PassThrough = false;
                        Ball->Color = glm::vec3(1.0f);
                    }
                }
                else if (powerUp.Type == "confuse")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "confuse"))
                    {	
                        Effects->Confuse = false;
                    }
                }
                else if (powerUp.Type == "chaos")
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "chaos"))
                    {	
                        Effects->Chaos = false;
                    }
                }
            }
        }
    }
    
    this->PowerUps.erase(std::remove_if(this->PowerUps.begin(), this->PowerUps.end(),
        [](const PowerUp &powerUp) { return powerUp.Destroyed && !powerUp.Activated; }
    ), this->PowerUps.end());
}


//Collisions

Direction VectorDirection(glm::vec2 target)
{
    glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f),	
        glm::vec2(1.0f, 0.0f),	
        glm::vec2(0.0f, -1.0f),	
        glm::vec2(-1.0f, 0.0f)
    };

    float max = 0.0f;
    unsigned int best_match = -1;
    for (unsigned int i = 0; i < 4; i++) 
    {
        float dot_product = glm::dot(glm::normalize(target), compass[i]);
        if(dot_product > max) 
        {
            max = dot_product;
            best_match = i;
        }
    }

    return (Direction)best_match;
}

bool CheckCollision(GameObject &one, GameObject &two) 
{
    bool collisionX = one.Position.x + one.Size.x >= two.Position.x && two.Position.x + two.Size.x >= one.Position.x;
    bool collisionY = one.Position.y + one.Size.y >= two.Position.y && two.Position.y + two.Size.y >= one.Position.y;

    return collisionX && collisionY;
}

Collision CheckCollision(BallObject &one, GameObject &two) 
{
    glm::vec2 center(one.Position + one.Radius);

    glm::vec2 aabb_half(two.Size/2.0f); 
    glm::vec2 aabb_center(two.Position + aabb_half);

    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half, aabb_half);

    glm::vec2 closest = aabb_center + clamped;
    difference = closest - center;

    if(glm::length(difference) < one.Radius) 
    {
        return std::make_tuple(true, VectorDirection(difference), difference);
    }
    else 
    {
        return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
    }
}

void Game::DoCollisions() 
{
    for(GameObject &box : this->Levels[this->Level].Bricks) 
    {
        if(!box.Destroyed)
        {
            Collision collision = CheckCollision(*Ball, box);
            if(std::get<0>(collision))
            {
                if (!box.IsSolid)
                {
                    box.Destroyed = true;
                    this->SpawnPowerUps(box);
                    SoundDevice->playSoundEffect("audio/mixkit-electronic-retro-block-hit-2185.wav", "block");
                    SoundDevice->setSoundEffectVolume("block", 10.0f);
                }
                else
                {   
                    ShakeTime = 0.05f;
                    Effects->Shake = true;
                    SoundDevice->playSoundEffect("audio/mixkit-wood-hard-hit-2182.wav", "solid_block");
                    SoundDevice->setSoundEffectVolume("solid_block", 10.0f);
                }               

                Direction dir = std::get<1>(collision);
                glm::vec2 diff_vector = std::get<2>(collision);
                if (!(Ball->PassThrough && !box.IsSolid))
                {
                    if (dir == LEFT || dir == RIGHT)
                    {
                        Ball->Velocity.x = -Ball->Velocity.x; 
                        
                        float penetration = Ball->Radius - std::abs(diff_vector.x); 
                        if (dir == LEFT) 
                        {
                            Ball->Position.x += penetration;
                        }
                        else 
                        {
                            Ball->Position.x -= penetration;
                        }
                    }
                    else 
                    {
                        Ball->Velocity.y = -Ball->Velocity.y;

                        float penetration = Ball->Radius - std::abs(diff_vector.y);
                        if(dir == UP) 
                        {
                            Ball->Position.y -= penetration;
                        }
                        else 
                        {
                            Ball->Position.y += penetration;
                        }
                    }
                }                
            }
        }
    }

    for (PowerUp &powerUp : this->PowerUps)
    {
        if (!powerUp.Destroyed)
        {
            if (powerUp.Position.y >= this->Height)
                powerUp.Destroyed = true;
            if (CheckCollision(*Player, powerUp))
            {	// collided with player, now activate powerup
                ActivatePowerUp(powerUp);
                powerUp.Destroyed = true;
                powerUp.Activated = true;
                SoundDevice->playSoundEffect("audio/powerup.wav", "power_up");
                SoundDevice->setSoundEffectVolume("power_up", 10.0f);
            }
        }
    }  

    //Player & Ball Collision

    Collision result = CheckCollision(*Ball, *Player);
    if (!Ball->Stuck && std::get<0>(result))
    {
        float centerBoard = Player->Position.x + Player->Size.x / 2.0f;
        float distance = (Ball->Position.x + Ball->Radius) - centerBoard;
        float percentage = distance / (Player->Size.x / 2.0f);

        float strength = 2.0f;
        glm::vec2 oldVelocity = Ball->Velocity;
        Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength; 
        Ball->Velocity.y = -1.0f * abs(Ball->Velocity.y);  
        Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity);
        Ball->Stuck = Ball->Sticky;
        SoundDevice->playSoundEffect("audio/metal-hit-81380.mp3", "player");
        SoundDevice->setSoundEffectVolume("player", 10.0f);
    } 


}

void Game::ResetLevel()
{
    if (this->Level == 0)
        this->Levels[0].Load("levels/one.lvl", this->Width, this->Height / 2);
    else if (this->Level == 1)
        this->Levels[1].Load("levels/two.lvl", this->Width, this->Height / 2);
    else if (this->Level == 2)
        this->Levels[2].Load("levels/three.lvl", this->Width, this->Height / 2);
    else if (this->Level == 3)
        this->Levels[3].Load("levels/four.lvl", this->Width, this->Height / 2);

    this->Lives = 3;
}

void Game::ResetPlayer()
{
    Player->Size = PLAYER_SIZE;
    Player->Position = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
    Ball->Reset(Player->Position + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -(BALL_RADIUS * 2.0f)), INITIAL_BALL_VELOCITY);

    Effects->Chaos = Effects->Confuse = false;
    Ball->PassThrough = Ball->Sticky = false;
    Player->Color = glm::vec3(1.0f);
    Ball->Color = glm::vec3(1.0f);
}