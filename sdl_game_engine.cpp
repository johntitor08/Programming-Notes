// sdl_game_engine.cpp
// A compact, single-file SDL2-based mini game engine + demo
// Features:
// - Engine class (init, run loop, fixed timestep, FPS cap)
// - Window & Renderer wrapper
// - Resource managers: TextureManager, FontManager, AudioManager
// - Basic Entity-Component system: Entity, Component, Transform, Sprite
// - Simple Scene/World handling
// - Input handling (keyboard)
// - Simple collision detection and movement system
// - Example demo at the bottom showing how to use the engine
// Requires: SDL2, SDL2_image, SDL2_ttf, SDL2_mixer
// Build (Linux pkg-config):
// g++ -std=c++17 -O2 -o engine sdl_game_engine.cpp `pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf SDL2_mixer`

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <cmath>
#include <cassert>

using namespace std;

// --------------------------- Utility ---------------------------
static void logSDLError(const string &msg) {
    cerr << msg << " Error: " << SDL_GetError() << "\n";
}

// --------------------------- Configuration ---------------------------
struct EngineConfig {
    int width = 800;
    int height = 600;
    string title = "SDL Mini Engine";
    int targetFPS = 60;
    bool vSync = false;
};

// --------------------------- Window & Renderer ---------------------------
struct GLWindow {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    bool create(const EngineConfig& cfg) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
            logSDLError("SDL_Init");
            return false;
        }

        window = SDL_CreateWindow(cfg.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  cfg.width, cfg.height, SDL_WINDOW_SHOWN);
        if (!window) { logSDLError("CreateWindow"); SDL_Quit(); return false; }

        Uint32 flags = SDL_RENDERER_ACCELERATED;
        if (cfg.vSync) flags |= SDL_RENDERER_PRESENTVSYNC;

        renderer = SDL_CreateRenderer(window, -1, flags);
        if (!renderer) { logSDLError("CreateRenderer"); SDL_DestroyWindow(window); SDL_Quit(); return false; }

        int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            cerr << "Warning: IMG_Init failed: " << IMG_GetError() << "\n";
        }

        if (TTF_Init() == -1) {
            cerr << "Warning: TTF_Init failed: " << TTF_GetError() << "\n";
        }

        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            cerr << "Warning: Mix_OpenAudio failed: " << Mix_GetError() << "\n";
        }
        Mix_AllocateChannels(16);

        return true;
    }

    void destroy() {
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
    }
};

// --------------------------- Resource managers ---------------------------
struct TextureManager {
    SDL_Renderer* ren = nullptr;
    unordered_map<string, SDL_Texture*> cache;

    TextureManager(SDL_Renderer* r = nullptr): ren(r) {}
    ~TextureManager(){ clear(); }

    SDL_Texture* load(const string& path) {
        if (!ren) return nullptr;
        auto it = cache.find(path);
        if (it != cache.end()) return it->second;
        SDL_Surface* surf = IMG_Load(path.c_str());
        if (!surf) {
            cerr << "IMG_Load failed for " << path << ": " << IMG_GetError() << "\n";
            return nullptr;
        }
        SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
        SDL_FreeSurface(surf);
        if (!tex) { logSDLError("CreateTextureFromSurface"); return nullptr; }
        cache[path] = tex;
        return tex;
    }

    void clear() {
        for (auto &p : cache) SDL_DestroyTexture(p.second);
        cache.clear();
    }
};

struct FontManager {
    unordered_map<string, TTF_Font*> cache;

    TTF_Font* load(const string& path, int ptsize) {
        string key = path + "#" + to_string(ptsize);
        auto it = cache.find(key);
        if (it != cache.end()) return it->second;
        TTF_Font* f = TTF_OpenFont(path.c_str(), ptsize);
        if (!f) { cerr << "TTF_OpenFont failed for " << path << ": " << TTF_GetError() << "\n"; return nullptr; }
        cache[key] = f;
        return f;
    }

    void clear() {
        for (auto &p : cache) TTF_CloseFont(p.second);
        cache.clear();
    }
};

struct AudioManager {
    unordered_map<string, Mix_Chunk*> sfx;
    unordered_map<string, Mix_Music*> mus;

    Mix_Chunk* loadSfx(const string& path) {
        auto it = sfx.find(path);
        if (it != sfx.end()) return it->second;
        Mix_Chunk* c = Mix_LoadWAV(path.c_str());
        if (!c) { cerr << "Mix_LoadWAV failed: " << Mix_GetError() << "\n"; return nullptr; }
        sfx[path] = c; return c;
    }
    Mix_Music* loadMusic(const string& path) {
        auto it = mus.find(path);
        if (it != mus.end()) return it->second;
        Mix_Music* m = Mix_LoadMUS(path.c_str());
        if (!m) { cerr << "Mix_LoadMUS failed: " << Mix_GetError() << "\n"; return nullptr; }
        mus[path] = m; return m;
    }
    void cleanup() {
        for (auto &p : sfx) Mix_FreeChunk(p.second);
        for (auto &p : mus) Mix_FreeMusic(p.second);
        sfx.clear(); mus.clear();
    }
};

// --------------------------- ECS (very small) ---------------------------
using EntityId = unsigned int;
static const EntityId INVALID_ENTITY = 0;

struct Component {
    EntityId owner = INVALID_ENTITY;
    virtual ~Component() = default;
};

struct Transform : Component {
    float x=0, y=0; float w=0, h=0; float angle=0;
};

struct Sprite : Component {
    SDL_Texture* texture = nullptr;
    int srcX=0, srcY=0, srcW=0, srcH=0;
    float scale = 1.0f;
};

struct Velocity : Component {
    float vx=0, vy=0;
};

struct Entity {
    EntityId id = INVALID_ENTITY;
    vector<shared_ptr<Component>> components;
    template<typename T, typename... Args>
    shared_ptr<T> addComponent(Args&&... args) {
        auto c = make_shared<T>(std::forward<Args>(args)...);
        c->owner = id;
        components.push_back(c);
        return c;
    }
    template<typename T>
    shared_ptr<T> getComponent() {
        for (auto &c : components) {
            auto p = dynamic_pointer_cast<T>(c);
            if (p) return p;
        }
        return nullptr;
    }
};

// --------------------------- Simple World/Scene ---------------------------
struct World {
    EntityId nextId = 1;
    unordered_map<EntityId, shared_ptr<Entity>> entities;

    shared_ptr<Entity> createEntity() {
        auto e = make_shared<Entity>();
        e->id = nextId++;
        entities[e->id] = e;
        return e;
    }
    void destroyEntity(EntityId id) {
        entities.erase(id);
    }
    vector<shared_ptr<Entity>> all() {
        vector<shared_ptr<Entity>> out;
        out.reserve(entities.size());
        for (auto &p : entities) out.push_back(p.second);
        return out;
    }
};

// --------------------------- Input ---------------------------
struct InputState {
    unordered_map<SDL_Scancode, bool> keys;
    bool quit = false;
    void reset() { keys.clear(); quit = false; }
};

// --------------------------- Engine ---------------------------
class Engine {
public:
    Engine(const EngineConfig& cfg): cfg(cfg) {}
    ~Engine(){ stop(); }

    bool init() {
        if (!window.create(cfg)) return false;
        texman = make_unique<TextureManager>(window.renderer);
        fontman = make_unique<FontManager>();
        audioman = make_unique<AudioManager>();
        world = make_unique<World>();
        running = true;
        return true;
    }

    void run(function<void(Engine&)> onUpdate = nullptr, function<void(Engine&)> onRender = nullptr) {
        const int frameDelay = 1000 / cfg.targetFPS;
        Uint32 frameStart;
        int frameTime;

        while (running) {
            frameStart = SDL_GetTicks();
            // input
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) { running = false; }
                else if (e.type == SDL_KEYDOWN) {
                    input.keys[e.key.keysym.scancode] = true;
                } else if (e.type == SDL_KEYUP) {
                    input.keys[e.key.keysym.scancode] = false;
                }
            }

            // update
            if (onUpdate) onUpdate(*this);

            // render
            SDL_SetRenderDrawColor(window.renderer, 20, 20, 20, 255);
            SDL_RenderClear(window.renderer);
            if (onRender) onRender(*this);
            SDL_RenderPresent(window.renderer);

            // frame cap
            frameTime = SDL_GetTicks() - frameStart;
            if (frameDelay > frameTime) SDL_Delay(frameDelay - frameTime);
        }
    }

    void stop() { if (running) { running=false; window.destroy(); texman->clear(); fontman->clear(); audioman->cleanup(); }}

    // helpers for demo usage
    SDL_Texture* loadTexture(const string& path) { return texman->load(path); }
    TTF_Font* loadFont(const string& path, int size) { return fontman->load(path,size); }
    Mix_Chunk* loadSfx(const string& path) { return audioman->loadSfx(path); }
    Mix_Music* loadMusic(const string& path) { return audioman->loadMusic(path); }

    World& getWorld() { return *world; }
    SDL_Renderer* renderer() { return window.renderer; }
    EngineConfig cfg;
    InputState input;

private:
    GLWindow window;
    unique_ptr<TextureManager> texman;
    unique_ptr<FontManager> fontman;
    unique_ptr<AudioManager> audioman;
    unique_ptr<World> world;
    bool running = false;
};

// --------------------------- Simple Systems ---------------------------

// Draw all entities which have Transform + Sprite
void renderEntities(Engine& eng) {
    auto ents = eng.getWorld().all();
    for (auto &e : ents) {
        auto t = e->getComponent<Transform>();
        auto s = e->getComponent<Sprite>();
        if (!t) continue;
        if (s && s->texture) {
            SDL_Rect dst = { (int)std::round(t->x), (int)std::round(t->y), (int)std::round(t->w * s->scale), (int)std::round(t->h * s->scale) };
            if (s->srcW>0 && s->srcH>0) {
                SDL_Rect src = { s->srcX, s->srcY, s->srcW, s->srcH };
                SDL_RenderCopyEx(eng.renderer(), s->texture, &src, &dst, t->angle, nullptr, SDL_FLIP_NONE);
            } else {
                SDL_RenderCopyEx(eng.renderer(), s->texture, nullptr, &dst, t->angle, nullptr, SDL_FLIP_NONE);
            }
        } else {
            // fallback rectangle
            SDL_SetRenderDrawColor(eng.renderer(), 255, 0, 255, 255);
            SDL_Rect r = { (int)std::round(t->x), (int)std::round(t->y), (int)std::round(t->w), (int)std::round(t->h) };
            SDL_RenderFillRect(eng.renderer(), &r);
        }
    }
}

// Basic physics: apply velocity to transform
void physicsSystem(Engine& eng) {
    auto ents = eng.getWorld().all();
    for (auto &e : ents) {
        auto t = e->getComponent<Transform>();
        auto v = e->getComponent<Velocity>();
        if (t && v) {
            t->x += v->vx;
            t->y += v->vy;
            // simple bounds clamp
            if (t->x < 0) t->x = 0;
            if (t->y < 0) t->y = 0;
            if (t->x + t->w > eng.cfg.width) t->x = eng.cfg.width - t->w;
            if (t->y + t->h > eng.cfg.height) t->y = eng.cfg.height - t->h;
        }
    }
}

// Simple AABB collision check
bool aabbIntersect(const Transform& a, const Transform& b) {
    return !(a.x + a.w < b.x || a.x > b.x + b.w || a.y + a.h < b.y || a.y > b.y + b.h);
}

// --------------------------- Demo Game Using Engine ---------------------------

// The demo is a small game: player moves with WASD/arrow, collects targets, enemy chases player.
int main(int argc, char* argv[]) {
    EngineConfig cfg;
    cfg.width = 800; cfg.height = 600; cfg.title = "Engine Demo"; cfg.targetFPS = 60;

    Engine eng(cfg);
    if (!eng.init()) { cerr << "Engine init failed\n"; return 1; }

    // Load assets (placeholders if you don't have assets)
    SDL_Texture* playerTex = eng.loadTexture("player.png");
    SDL_Texture* targetTex = eng.loadTexture("target.png");
    SDL_Texture* enemyTex = eng.loadTexture("enemy.png");
    SDL_Texture* bgTex = eng.loadTexture("bg.png");
    TTF_Font* font = eng.loadFont("font.ttf", 24);
    Mix_Chunk* sfx = eng.loadSfx("hit.wav");
    Mix_Music* music = eng.loadMusic("music.ogg");
    if (music) Mix_PlayMusic(music, -1);

    // Create entities
    auto player = eng.getWorld().createEntity();
    auto pTrans = player->addComponent<Transform>();
    pTrans->x = cfg.width/2 - 32; pTrans->y = cfg.height/2 - 32; pTrans->w = 64; pTrans->h = 64;
    auto pSprite = player->addComponent<Sprite>(); pSprite->texture = playerTex; pSprite->scale = 1.0f;
    auto pVel = player->addComponent<Velocity>();

    auto target = eng.getWorld().createEntity();
    auto tTrans = target->addComponent<Transform>();
    tTrans->x = rand() % (cfg.width - 32); tTrans->y = rand() % (cfg.height - 32); tTrans->w = 32; tTrans->h = 32;
    auto tSprite = target->addComponent<Sprite>(); tSprite->texture = targetTex;

    auto enemy = eng.getWorld().createEntity();
    auto eTrans = enemy->addComponent<Transform>();
    eTrans->x = rand() % (cfg.width - 64); eTrans->y = rand() % (cfg.height - 64); eTrans->w = 48; eTrans->h = 48;
    auto eSprite = enemy->addComponent<Sprite>(); eSprite->texture = enemyTex;

    int score = 0;

    // update function
    auto onUpdate = [&](Engine& E) {
        // input
        const auto& keys = E.input.keys;
        float speed = 4.0f;
        auto pv = player->getComponent<Velocity>();
        pv->vx = 0; pv->vy = 0;
        if (keys.at(SDL_SCANCODE_W) || keys.at(SDL_SCANCODE_UP)) pv->vy = -speed;
        if (keys.at(SDL_SCANCODE_S) || keys.at(SDL_SCANCODE_DOWN)) pv->vy = speed;
        if (keys.at(SDL_SCANCODE_A) || keys.at(SDL_SCANCODE_LEFT)) pv->vx = -speed;
        if (keys.at(SDL_SCANCODE_D) || keys.at(SDL_SCANCODE_RIGHT)) pv->vx = speed;

        // physics
        physicsSystem(E);

        // enemy AI: move toward player
        auto pt = player->getComponent<Transform>();
        auto et = enemy->getComponent<Transform>();
        float dx = (pt->x + pt->w/2.0f) - (et->x + et->w/2.0f);
        float dy = (pt->y + pt->h/2.0f) - (et->y + et->h/2.0f);
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist > 0.001f) {
            float nx = dx / dist; float ny = dy / dist;
            et->x += nx * 1.5f; et->y += ny * 1.5f;
        }

        // collision: player-target
        auto ttt = target->getComponent<Transform>();
        if (aabbIntersect(*pt, *ttt)) {
            score++;
            if (sfx) Mix_PlayChannel(-1, sfx, 0);
            ttt->x = rand() % (E.cfg.width - (int)ttt->w);
            ttt->y = rand() % (E.cfg.height - (int)ttt->h);
            // nudge enemy
            et->x = rand() % (E.cfg.width - (int)et->w);
            et->y = rand() % (E.cfg.height - (int)et->h);
        }

        // collision: player-enemy -> reset
        if (aabbIntersect(*pt, *et)) {
            score = 0;
            pt->x = E.cfg.width / static_cast<float>(2); pt->y = E.cfg.height / 2;
            et->x = rand() % (E.cfg.width - (int)et->w);
            et->y = rand() % (E.cfg.height - (int)et->h);
        }
    };

    // render function
    auto onRender = [&](Engine& E) {
        // optional bg
        if (bgTex) {
            SDL_Rect dst = {0,0, E.cfg.width, E.cfg.height};
            SDL_RenderCopy(E.renderer(), bgTex, nullptr, &dst);
        }

        // render entities
        renderEntities(E);

        // HUD: score
        if (font) {
            SDL_Color white = {255,255,255,255};
            string s = "Score: " + to_string(score);
            SDL_Surface* surf = TTF_RenderUTF8_Blended(font, s.c_str(), white);
            if (surf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(E.renderer(), surf);
                SDL_Rect dst = {10,10, surf->w, surf->h};
                SDL_RenderCopy(E.renderer(), tex, nullptr, &dst);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }
        }
    };

    eng.run(onUpdate, onRender);

    eng.stop();
    return 0;
}
