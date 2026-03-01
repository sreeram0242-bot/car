/*
 * Neon Runner - C++ Port
 * Requires: Raylib (https://www.raylib.com/)
 * Compile example: g++ main.cpp -lraylib -lGL -lm -lpthread -ldl -rt -Xlinker -zmuldefs
 */

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <algorithm>
#include <string>

// Constants
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const float LANE_WIDTH = 5.0f;
const float GRAVITY = -40.0f;
const float JUMP_FORCE = 15.0f;
const float BASE_SPEED = 20.0f;

struct Player {
    Vector3 position;
    float velocityY;
    int lane; // -1, 0, 1
    bool isJumping;
    Color color;
    float boostEnergy;
    bool isBoosting;
};

struct Obstacle {
    Vector3 position;
    bool active;
    int type; // 0: Box, 1: Coin, 2: Boost
    Color color;
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Neon Synthwave Runner [C++]");
    SetTargetFPS(60);

    // Camera Setup
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 7.0f, 12.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, -10.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Game State
    Player p1 = { {0, 1, 0}, 0, 0, false, (Color){0, 255, 255, 255}, 100.0f, false };
    std::vector<Obstacle> obstacles;
    float gameSpeed = BASE_SPEED;
    float spawnTimer = 0.0f;
    float score = 0.0f;
    bool gameOver = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // --- UPDATE ---
        if (!gameOver) {
            // Input
            if (IsKeyPressed(KEY_LEFT) && p1.lane > -1) p1.lane--;
            if (IsKeyPressed(KEY_RIGHT) && p1.lane < 1) p1.lane++;
            if (IsKeyPressed(KEY_UP) && !p1.isJumping) {
                p1.velocityY = JUMP_FORCE;
                p1.isJumping = true;
            }
            if (IsKeyPressed(KEY_DOWN) && p1.position.y > 1.5f) {
                p1.velocityY = -20.0f; // Fast fall
            }
            
            // Boost Logic
            if (IsKeyDown(KEY_SPACE) && p1.boostEnergy > 0) {
                p1.isBoosting = true;
                p1.boostEnergy -= 50 * dt;
            } else {
                p1.isBoosting = false;
                if (p1.boostEnergy < 100) p1.boostEnergy += 5 * dt;
            }

            float currentSpeed = p1.isBoosting ? gameSpeed + 30.0f : gameSpeed;

            // Physics
            float targetX = p1.lane * LANE_WIDTH;
            p1.position.x = Lerp(p1.position.x, targetX, 10.0f * dt);
            
            p1.velocityY += GRAVITY * dt;
            p1.position.y += p1.velocityY * dt;

            // Ground Collision
            if (p1.position.y < 1.0f) {
                p1.position.y = 1.0f;
                p1.velocityY = 0;
                p1.isJumping = false;
            }

            // Game Progression
            gameSpeed += 0.5f * dt;
            score += currentSpeed * dt;

            // Spawn Obstacles
            spawnTimer += dt;
            if (spawnTimer > 20.0f / currentSpeed) {
                spawnTimer = 0;
                int lane = GetRandomValue(-1, 1);
                Obstacle obs;
                obs.position = (Vector3){ lane * LANE_WIDTH, 1.0f, -100.0f };
                obs.active = true;
                int typeRnd = GetRandomValue(0, 10);
                if (typeRnd < 2) { obs.type = 1; obs.color = GOLD; } // Coin
                else if (typeRnd < 3) { obs.type = 2; obs.color = GREEN; } // Boost
                else { obs.type = 0; obs.color = RED; } // Barrier
                obstacles.push_back(obs);
            }

            // Update Obstacles
            for (auto& obs : obstacles) {
                obs.position.z += currentSpeed * dt;
                
                if (obs.active) {
                    // Simple AABB Collision
                    bool collisionX = fabs(p1.position.x - obs.position.x) < 1.5f;
                    bool collisionY = fabs(p1.position.y - obs.position.y) < 1.5f;
                    bool collisionZ = fabs(p1.position.z - obs.position.z) < 1.5f;

                    if (collisionX && collisionY && collisionZ) {
                        if (obs.type == 1) { // Coin
                            score += 500;
                            obs.active = false;
                        } else if (obs.type == 2) { // Boost
                            p1.boostEnergy = 100.0f;
                            obs.active = false;
                        } else { // Crash
                            gameOver = true;
                        }
                    }
                }
            }

            // Cleanup
            obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(), 
                [](const Obstacle& o) { return o.position.z > 20.0f; }), obstacles.end());
        
            // Camera Shake on Boost
            if (p1.isBoosting) {
                camera.position.x = (float)GetRandomValue(-10, 10) / 100.0f;
                camera.fovy = Lerp(camera.fovy, 80.0f, 5.0f * dt);
            } else {
                camera.position.x = 0;
                camera.fovy = Lerp(camera.fovy, 60.0f, 5.0f * dt);
            }
        } else {
            if (IsKeyPressed(KEY_ENTER)) {
                gameOver = false;
                score = 0;
                gameSpeed = BASE_SPEED;
                obstacles.clear();
                p1.lane = 0;
                p1.position = (Vector3){0, 1, 0};
                p1.boostEnergy = 100;
            }
        }

        // --- DRAW ---
        BeginDrawing();
        ClearBackground((Color){ 5, 0, 17, 255 }); // Deep Dark Blue

        BeginMode3D(camera);
            
            // Grid Floor
            for (int i = -20; i <= 20; i++) {
                DrawLine3D((Vector3){ (float)i * 5, 0, -150 }, (Vector3){ (float)i * 5, 0, 20 }, DARKPURPLE);
            }
            // Moving Horizontal Lines
            float offset = fmod(GetTime() * (p1.isBoosting ? gameSpeed + 30 : gameSpeed), 10.0f);
            for (int i = 0; i < 30; i++) {
                float z = 20 - (i * 10) + offset;
                if (z > -150 && z < 20)
                    DrawLine3D((Vector3){ -100, 0, z }, (Vector3){ 100, 0, z }, MAGENTA);
            }

            // Player
            if (!gameOver) {
                DrawCube(p1.position, 1.8f, 0.5f, 4.0f, BLACK);
                DrawCubeWires(p1.position, 1.8f, 0.5f, 4.0f, p1.color);
                // Wheels
                DrawCylinder((Vector3){p1.position.x - 1.0f, 0.5f, p1.position.z - 1.2f}, 0.4f, 0.4f, 0.5f, 10, DARKGRAY);
                DrawCylinder((Vector3){p1.position.x + 1.0f, 0.5f, p1.position.z - 1.2f}, 0.4f, 0.4f, 0.5f, 10, DARKGRAY);
                // Boost Flame
                if (p1.isBoosting) {
                    DrawCylinder((Vector3){p1.position.x, 0.5f, p1.position.z - 3.0f}, 0.1f, 0.5f, 2.0f, 8, ORANGE);
                }
            }

            // Obstacles
            for (const auto& obs : obstacles) {
                if (!obs.active) continue;
                if (obs.type == 0) {
                    DrawCube(obs.position, 2.0f, 2.0f, 2.0f, ColorAlpha(obs.color, 0.8f));
                    DrawCubeWires(obs.position, 2.0f, 2.0f, 2.0f, WHITE);
                } else if (obs.type == 1) {
                    DrawSphere(obs.position, 0.8f, GOLD);
                } else {
                    DrawCube(obs.position, 1.0f, 1.0f, 1.0f, GREEN);
                }
            }

        EndMode3D();

        // HUD
        DrawText(TextFormat("SCORE: %06.0f", score), 20, 20, 30, WHITE);
        
        // Boost Bar
        DrawRectangle(SCREEN_WIDTH - 220, 20, 200, 20, Fade(BLACK, 0.5f));
        DrawRectangle(SCREEN_WIDTH - 220, 20, (int)(p1.boostEnergy * 2), 20, CYAN);
        DrawRectangleLines(SCREEN_WIDTH - 220, 20, 200, 20, WHITE);
        DrawText("BOOST", SCREEN_WIDTH - 220, 45, 10, CYAN);

        if (gameOver) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));
            DrawText("BUSTED", SCREEN_WIDTH/2 - MeasureText("BUSTED", 60)/2, SCREEN_HEIGHT/2 - 60, 60, RED);
            DrawText("PRESS ENTER TO RETRY", SCREEN_WIDTH/2 - MeasureText("PRESS ENTER TO RETRY", 20)/2, SCREEN_HEIGHT/2 + 20, 20, WHITE);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
