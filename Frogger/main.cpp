#include <iostream>
#include <vector>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <raylib.h>

using namespace std;

const int screen_width = 1600;
const int screen_height = 1800;

const int cellsize = 128;
const int cells_x = screen_width / cellsize;
const int cells_y = screen_height / cellsize;

// Show a simple "You Win" screen and wait for user action.
// Returns 1 = restart, 0 = quit (window will be closed inside the function)
int winloop() {
    const char* title = "YOU WIN!";
    const char* instr = "Press ENTER to play again or ESC to quit";
    const int titleSize = 96;
    const int instrSize = 22;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        int titleW = MeasureText(title, titleSize);
        DrawText(title, screen_width / 2 - titleW / 2, screen_height / 2 - titleSize - 20, titleSize, GREEN);

        int instrW = MeasureText(instr, instrSize);
        DrawText(instr, screen_width / 2 - instrW / 2, screen_height / 2 + 20, instrSize, RAYWHITE);

        EndDrawing();

        if (IsKeyPressed(KEY_ENTER)) {
            return 1; // restart
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            CloseWindow();
            return 0; // quit
        }
    }

    return 0;
}

// Game over screen: R to retry, ESC to quit
int gameoverloop() {
    const char* title = "GAME OVER";
    const char* instr = "Press R to retry or ESC to quit";
    const int titleSize = 96;
    const int instrSize = 22;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        int titleW = MeasureText(title, titleSize);
        DrawText(title, screen_width / 2 - titleW / 2, screen_height / 2 - titleSize - 20, titleSize, RED);

        int instrW = MeasureText(instr, instrSize);
        DrawText(instr, screen_width / 2 - instrW / 2, screen_height / 2 + 20, instrSize, RAYWHITE);

        EndDrawing();

        if (IsKeyPressed(KEY_R)) {
            return 1; // restart
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            CloseWindow();
            return 0; // quit
        }
    }

    return 0;
}

class water {
public:
    float x, y;
    void draw(Texture2D texture) {
        float startX = x;
        for (int i = 0; i <= cells_x; i++) {
            float centerX = startX + i * (float)cellsize + cellsize / 2.0f;
            Vector2 position = { centerX - texture.width / 2.0f, y - texture.height / 2.0f };
            DrawTextureEx(texture, position, 0.0f, 1, WHITE);
        }
    }
};

class road {
public:
    float x, y;
    void draw(Texture2D texture) {
        float startX = x;
        for (int i = 0; i <= cells_x; i++) {
            float centerX = startX + i * (float)cellsize + cellsize / 2.0f;
            Vector2 position = { centerX - texture.width / 2.0f, y - texture.height / 2.0f };
            DrawTextureEx(texture, position, 0.0f, 1, WHITE);
        }
    }
};

class grass {
public:
    float x, y;
    void draw(Texture2D texture) {
        float startX = x;
        for (int i = 0; i <= cells_x; i++) {
            float centerX = startX + i * (float)cellsize + cellsize / 2.0f;
            Vector2 position = { centerX - texture.width / 2.0f, y - texture.height / 2.0f };
            DrawTextureEx(texture, position, 0.0f, 1, WHITE);
        }
    }
};

class woodPlat {
public:
    float x, y;
    void draw(Texture2D texture) {
        float scale = 0.064f;
        float startX = x;
        for (int i = 0; i <= cells_x; i++) {
            float centerX = startX + i * (float)cellsize + cellsize / 2.0f;
            Vector2 position = { centerX - (texture.width * scale) / 2.0f, y - (texture.height * scale) / 2.0f };
            DrawTextureEx(texture, position, 0.0f, scale, WHITE);
        }
    }
};

// Smooth moving logs (already implemented earlier)
struct Log {
    float x;           // leftmost pixel position (not centered)
    float y;           // center y of row
    int lengthCells;   // 1..5
    float speed;       // pixels per second (positive magnitude)
    int dir;           // +1 right, -1 left

    void update(float dt) {
        x += dir * speed * dt;
        float totalWidth = lengthCells * (float)cellsize;
        if (dir > 0 && x > screen_width) x = -totalWidth;
        if (dir < 0 && x + totalWidth < 0) x = screen_width;
    }

    void draw(Texture2D texture) const {
        float scale = (float)cellsize / (float)texture.width;
        for (int i = 0; i < lengthCells; i++) {
            float centerX = x + i * (float)cellsize + cellsize / 2.0f;
            Vector2 position = { centerX - (texture.width * scale) / 2.0f, y - (texture.height * scale) / 2.0f };
            DrawTextureEx(texture, position, 0.0f, scale, WHITE);
        }
    }

    bool coversX(float px) const {
        float left = x;
        float right = x + lengthCells * (float)cellsize;
        return (px >= left) && (px <= right);
    }
};

// Cars driving on roads
// NOTE: Car.x is CENTER X now (not leftmost) — this avoids placement/rotation off-by-half issues.
struct Car {
    float x;    // center pixel X
    float y;    // center y of road row
    float speed; // pixels/sec
    int dir;    // +1 right, -1 left
    float rotationDeg; // rotation to apply when drawing

    void update(float dt, float carHalfWidth) {
        x += dir * speed * dt;
        // wrap using center and half-width
        if (dir > 0 && (x - carHalfWidth) > screen_width) x = -carHalfWidth;
        if (dir < 0 && (x + carHalfWidth) < 0) x = screen_width + carHalfWidth;
    }

    void draw(Texture2D texture, float scale) const {
        float drawW = texture.width * scale;
        float drawH = texture.height * scale;
        Rectangle src = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
        Rectangle dest = { x, y, drawW, drawH };          // center x,y as the pivot point
        Vector2 origin = { drawW / 2.0f, drawH / 2.0f };  // origin relative to dest top-left
        DrawTexturePro(texture, src, dest, origin, rotationDeg, WHITE);
    }
};

class Frog {
public:
    float x, y;
    int speed_x, speed_y;

    void draw(Texture2D texture) const {
        float scale = 0.25f;
        Vector2 position = { x - (texture.width * scale) / 2, y - (texture.height * scale) / 2 };
        DrawTextureEx(texture, position, 0.0f, scale, WHITE);
    }

    void update() {
        if (IsKeyPressed(KEY_LEFT)) x -= cellsize;
        if (IsKeyPressed(KEY_RIGHT)) x += cellsize;
        if (IsKeyPressed(KEY_UP)) y -= cellsize;
        if (IsKeyPressed(KEY_DOWN)) y += cellsize;

        if (x < cellsize / 2) x = cellsize / 2;
        if (x > screen_width - cellsize / 2) x = screen_width - cellsize / 2;
        if (y > screen_height - cellsize / 2) y = screen_height - cellsize / 2;
    }
};

// Helper to (re)populate logs for water rows
void populateLogs(vector<vector<Log>> &logRows, const vector<float> &waterYs) {
    logRows.clear();
    logRows.resize(waterYs.size());
    for (size_t row = 0; row < waterYs.size(); ++row) {
        int dir = (row % 2 == 0) ? 1 : -1; // alternate direction per row
        int numLogs = GetRandomValue(2, 4);
        for (int i = 0; i < numLogs; ++i) {
            Log L;
            L.lengthCells = GetRandomValue(1, 5);
            float baseCellsPerSec = 1.0f + (float)row * 0.1f;
            L.speed = baseCellsPerSec * (float)cellsize;
            L.dir = dir;
            L.y = waterYs[row];
            float totalWidth = L.lengthCells * (float)cellsize;
            L.x = (float)GetRandomValue((int)-totalWidth, screen_width + (int)totalWidth);
            logRows[row].push_back(L);
        }
    }
}

// Populate cars on a specific set of road Y positions.
// Spacing increased by 2 cells across all rows (user request).
void populateCars(vector<vector<Car>> &carRows, const vector<float> &roadYs, float carWidth) {
    carRows.clear();
    carRows.resize(roadYs.size());
    // base configuration: spacing in cells and speed (cells/sec) per road (bottom->top)
    // original was {2,3,4} -> space out by +2 => {4,5,6}
    int spacingCellsArr[3] = { 4, 5, 6 };                    // spaced out by 2 cells
    float speedCellsPerSec[3] = { 0.5f, 0.9f, 1.6f };        // slow, medium, fast

    for (size_t r = 0; r < roadYs.size() && r < 3; ++r) {
        int dir = (r % 2 == 0) ? 1 : -1; // alternate directions for variety
        int spacingPixels = spacingCellsArr[r] * cellsize;
        // ensure spacing at least car width + small margin to avoid overlap
        spacingPixels = max(spacingPixels, (int)ceil(carWidth) + 8);

        int num = screen_width / spacingPixels + 3; // +3 to ensure coverage across wrapping
        float halfSpacing = spacingPixels * 0.5f;

        for (int i = 0; i < num; ++i) {
            Car c;
            c.dir = dir;
            c.speed = speedCellsPerSec[r] * (float)cellsize;
            c.y = roadYs[r];
            c.rotationDeg = (r == 0 || r == 2) ? 180.0f : 0.0f;

            // deterministic center X spacing (no random offset)
            if (dir > 0) {
                // centers start left of screen by half spacing so they flow in naturally
                c.x = -halfSpacing + i * spacingPixels;
            } else {
                c.x = screen_width + halfSpacing - i * spacingPixels;
            }

            carRows[r].push_back(c);
        }
    }
}

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screen_width, screen_height, "Frogger");
    SetTargetFPS(60);

    InitAudioDevice();

    srand((unsigned)time(nullptr));

    Texture2D frogTexture = LoadTexture("Your File Path /images/frog.png");
    Texture2D woodTexture = LoadTexture("Your File Path /images/wood.jpg");
    Texture2D roadTexture = LoadTexture("Your File Path /images/road.png");
    Texture2D grassTexture = LoadTexture("Your File Path /images/grass.png");
    Texture2D waterTexture = LoadTexture("Your File Path /images/water.png");
    Texture2D carTexture = LoadTexture("Your File Path /images/car.png");

	Music Background = LoadMusicStream("Your File Path /sounds/background.mp3");

    cout << "The frog texture is " << frogTexture.width << " pixels wide and " << frogTexture.height << " pixels tall." << endl;
    cout << "The wood texture is " << woodTexture.width << " pixels wide and " << woodTexture.height << " pixels tall." << endl;

    Frog frog;
    frog.x = screen_width / 2.0f;
    frog.y = screen_height - cellsize / 2.0f;

    woodPlat wood;
    wood.x = 0;
    wood.y = screen_height - cellsize / 2.0f;

    road road1; road1.x = 0; road1.y = wood.y - cellsize * 1.0f;
    grass grass1; grass1.x = 0; grass1.y = road1.y - cellsize * 1.0f;

    road road2; road2.x = 0; road2.y = grass1.y - cellsize * 1.0f;
    grass grass2; grass2.x = 0; grass2.y = road2.y - cellsize * 1.0f;

    road road3; road3.x = 0; road3.y = grass2.y - cellsize * 1.0f;
    grass grass3; grass3.x = 0; grass3.y = road3.y - cellsize * 1.0f;

    // water rows and win grass
    water water1; water1.x = 0; water1.y = grass3.y - cellsize * 1.0f;
    water water2; water2.x = 0; water2.y = water1.y - cellsize * 1.0f;
    water water3; water3.x = 0; water3.y = water2.y - cellsize * 1.0f;
    water water4; water4.x = 0; water4.y = water3.y - cellsize * 1.0f;
    water water5; water5.x = 0; water5.y = water4.y - cellsize * 1.0f;
    water water6; water6.x = 0; water6.y = water5.y - cellsize * 1.0f;

    grass winGrass; winGrass.x = 0; winGrass.y = water6.y - cellsize * 1.0f; // top row -> win

    vector<float> waterYs = { water1.y, water2.y, water3.y, water4.y, water5.y, water6.y };
    vector<float> roadYs = { road1.y, road2.y, road3.y };

    vector<vector<Log>> logRows;
    populateLogs(logRows, waterYs);

    // compute car scale that fits within a cell (preserve aspect and ensure full car visible)
    float carScale = min((float)cellsize / (float)carTexture.width, (float)cellsize / (float)carTexture.height) * 0.85f;
    float carW = carTexture.width * carScale;
    float carH = carTexture.height * carScale;

    vector<vector<Car>> carRows;
    populateCars(carRows, roadYs, carW);

    auto resetGame = [&]() {
        frog.x = screen_width / 2.0f;
        frog.y = screen_height - cellsize / 2.0f;
        populateLogs(logRows, waterYs);
        populateCars(carRows, roadYs, carW);
    };

    PlayMusicStream(Background);
	SetMusicVolume(Background, 0.5f);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

		UpdateMusicStream(Background);

        // Update logs
        for (size_t row = 0; row < logRows.size(); ++row) {
            for (auto &L : logRows[row]) L.update(dt);
        }

        // Update cars (car.x is center; update uses half width)
        for (size_t r = 0; r < carRows.size(); ++r) {
            for (auto &c : carRows[r]) c.update(dt, carW * 0.5f);
        }

        // Update frog input (grid moves)
        frog.update();

        // Car collision check (car.x is center)
        float frogScale = 0.25f;
        Rectangle frogRect = { frog.x - (frogTexture.width * frogScale) / 2.0f,
                               frog.y - (frogTexture.height * frogScale) / 2.0f,
                               frogTexture.width * frogScale,
                               frogTexture.height * frogScale };

        bool collided = false;
        for (size_t r = 0; r < carRows.size(); ++r) {
            for (const auto &c : carRows[r]) {
                Rectangle carRect = { c.x - carW / 2.0f, c.y - carH / 2.0f, carW, carH };
                if (CheckCollisionRecs(frogRect, carRect)) {
                    collided = true;
                    break;
                }
            }
            if (collided) break;
        }

        if (collided) {
            int action = gameoverloop();
            if (action == 1) { resetGame(); continue; }
            else break;
        }

        // Determine frog state relative to water/logs
        bool frogOnLog = false;
        const Log* supportingLog = nullptr;
        for (size_t row = 0; row < logRows.size(); ++row) {
            if (fabs(frog.y - waterYs[row]) < 1e-3f) {
                for (const auto &L : logRows[row]) {
                    if (L.coversX(frog.x)) {
                        frogOnLog = true;
                        supportingLog = &L;
                        break;
                    }
                }
                break;
            }
        }

        // If on log -> move frog with log
        if (frogOnLog && supportingLog != nullptr) {
            frog.x += supportingLog->dir * supportingLog->speed * dt;
            if (frog.x < cellsize / 2 || frog.x > screen_width - cellsize / 2) {
                int action = gameoverloop();
                if (action == 1) { resetGame(); continue; }
                else break;
            }
        } else {
            // If frog standing in any water row and NOT on a log -> game over
            bool inWater = false;
            for (float wy : waterYs) {
                if (fabs(frog.y - wy) < 1e-3f) { inWater = true; break; }
            }
            if (inWater && !frogOnLog) {
                int action = gameoverloop();
                if (action == 1) { resetGame(); continue; }
                else break;
            }
        }

        // If frog reached the win grass row -> win
        if (fabs(frog.y - winGrass.y) < 1e-3f) {
            int action = winloop();
            if (action == 1) { resetGame(); continue; }
            else break;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        // Sequence: wood, road, grass, road, grass, road, grass
        wood.draw(woodTexture);

        // road1 + cars
        road1.draw(roadTexture);
        if (carRows.size() > 0) for (const auto &c : carRows[0]) c.draw(carTexture, carScale);
        grass1.draw(grassTexture);

        // road2 + cars
        road2.draw(roadTexture);
        if (carRows.size() > 1) for (const auto &c : carRows[1]) c.draw(carTexture, carScale);
        grass2.draw(grassTexture);

        // road3 + cars
        road3.draw(roadTexture);
        if (carRows.size() > 2) for (const auto &c : carRows[2]) c.draw(carTexture, carScale);
        grass3.draw(grassTexture);

        // draw water rows (top-down)
        water1.draw(waterTexture);
        water2.draw(waterTexture);
        water3.draw(waterTexture);
        water4.draw(waterTexture);
        water5.draw(waterTexture);
        water6.draw(waterTexture);

        // draw logs using the wood texture
        for (size_t row = 0; row < logRows.size(); ++row)
            for (auto &L : logRows[row]) L.draw(woodTexture);

        // draw winning grass row
        winGrass.draw(grassTexture);

        frog.draw(frogTexture);

        EndDrawing();
    }

    // UNLOAD TEXTURES HERE (Before CloseWindow)
    UnloadTexture(frogTexture);
    UnloadTexture(woodTexture);
    UnloadTexture(roadTexture);
    UnloadTexture(grassTexture);
    UnloadTexture(waterTexture);
    UnloadTexture(carTexture);
	UnloadMusicStream(Background);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
