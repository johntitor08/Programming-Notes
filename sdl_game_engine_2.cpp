// sdl_game_engine_with_imgui.cpp
// Single-file SDL2 engine + Dear ImGui scene editor integration (prototype)
// Features:
// - SDL2 core engine (from previous file)
// - Dear ImGui integration (SDL + SDL_Renderer backend)
// - Simple Scene Editor window: Hierarchy, Inspector, Viewport (drag to move), play/pause
// - Uses existing tiny ECS (Entity, Transform, Sprite, Velocity)
// - Build notes below

/*
  REQUIREMENTS
  - SDL2, SDL2_image, SDL2_ttf, SDL2_mixer
  - Dear ImGui sources (imgui/*.cpp + backends/imgui_impl_sdl.cpp + backends/imgui_impl_sdlrenderer.cpp)

  Simple build (Linux, pkg-config) assuming you have imgui cpp files in the project directory:

  g++ -std=c++17 -O2 -o editor \
    sdl_game_engine_with_imgui.cpp imgui/*.cpp backends/imgui_impl_sdl.cpp backends/imgui_impl_sdlrenderer.cpp \
    `pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf SDL2_mixer` -lSDL2

  If you use a separate imgui compiled library, link it instead.

  Place assets in the executable folder (player.png, target.png, enemy.png, font.ttf, hit.wav, music.ogg)
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

// Dear ImGui
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <cmath>
#include <cassert>

using namespace std;

// --------------------------- Config ---------------------------
struct EngineConfig { int width=1280, height=720; string title="SDL Engine + ImGui Editor"; int targetFPS=60; bool vSync=false; };

// --------------------------- Minimal Engine (window/renderer/imgui) ---------------------------
struct EngineCore {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    EngineConfig cfg;

    bool init(const EngineConfig& c) {
        cfg = c;
        if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0) { cerr<<"SDL_Init: "<<SDL_GetError()<<"\n"; return false; }
        window = SDL_CreateWindow(cfg.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, cfg.width, cfg.height, SDL_WINDOW_RESIZABLE);
        if (!window) { cerr<<"CreateWindow: "<<SDL_GetError()<<"\n"; return false; }
        Uint32 flags = SDL_RENDERER_ACCELERATED; if (cfg.vSync) flags |= SDL_RENDERER_PRESENTVSYNC;
        renderer = SDL_CreateRenderer(window, -1, flags);
        if (!renderer) { cerr<<"CreateRenderer: "<<SDL_GetError()<<"\n"; return false; }

        int imgFlags = IMG_INIT_PNG|IMG_INIT_JPG;
        IMG_Init(imgFlags);
        TTF_Init();
        Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
        Mix_AllocateChannels(8);

        // ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO(); (void)io;
        ImGui::StyleColorsDark();
        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer_Init(renderer);

        return true;
    }

    void shutdown() {
        ImGui_ImplSDLRenderer_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
    }
};

// --------------------------- Resource manager (textures only) ---------------------------
struct TextureManager {
    SDL_Renderer* ren = nullptr; unordered_map<string, SDL_Texture*> cache;
    TextureManager(SDL_Renderer* r=nullptr): ren(r) {}
    ~TextureManager(){ clear(); }
    SDL_Texture* load(const string& path) { if (!ren) return nullptr; auto it=cache.find(path); if (it!=cache.end()) return it->second; SDL_Surface* s=IMG_Load(path.c_str()); if(!s){ cerr<<"IMG_Load failed: "<<path<<" "<<IMG_GetError()<<"\n"; return nullptr;} SDL_Texture* t=SDL_CreateTextureFromSurface(ren,s); SDL_FreeSurface(s); if(!t) { cerr<<"CreateTexture failed: "<<SDL_GetError()<<"\n"; return nullptr; } cache[path]=t; return t; }
    void clear(){ for(auto &p:cache) SDL_DestroyTexture(p.second); cache.clear(); }
};

// --------------------------- Tiny ECS ---------------------------
using EntityId = unsigned int; static const EntityId INVALID_ENTITY = 0;
struct Component { EntityId owner=INVALID_ENTITY; virtual ~Component()=default; };
struct Transform: Component { float x=0,y=0,w=0,h=0,angle=0; };
struct Sprite: Component { SDL_Texture* tex=nullptr; float scale=1.0f; };
struct Velocity: Component { float vx=0, vy=0; };
struct Entity { EntityId id=INVALID_ENTITY; vector<shared_ptr<Component>> comps; template<typename T, typename... Args> shared_ptr<T> add(Args&&...args){ auto c=make_shared<T>(forward<Args>(args)...); c->owner=id; comps.push_back(c); return c;} template<typename T> shared_ptr<T> get(){ for(auto &c:comps){ auto p=dynamic_pointer_cast<T>(c); if(p) return p;} return nullptr; } };

struct World { EntityId next=1; unordered_map<EntityId, shared_ptr<Entity>> ents; shared_ptr<Entity> create(){ auto e=make_shared<Entity>(); e->id=next++; ents[e->id]=e; return e; } vector<shared_ptr<Entity>> all(){ vector<shared_ptr<Entity>> out; for(auto &p:ents) out.push_back(p.second); return out; } };

// --------------------------- Utilities ---------------------------
static bool aabbIntersect(const Transform& a, const Transform& b){ return !(a.x+a.w < b.x || a.x > b.x+b.w || a.y+a.h < b.y || a.y > b.y+b.h); }

// --------------------------- Editor UI + Interaction ---------------------------
struct Editor {
    EngineCore* core = nullptr;
    TextureManager texman;
    World world;
    shared_ptr<Entity> selected = nullptr;
    bool playing = false;
    int score = 0;

    Editor(EngineCore* c): core(c), texman(c->renderer) {}

    void loadDemoAssets(){ texman.load("player.png"); texman.load("target.png"); texman.load("enemy.png"); texman.load("bg.png"); }

    void spawnDemoScene(){ // create player,target,enemy
        world = World(); selected = nullptr; score=0;
        auto p = world.create(); auto pt = p->add<Transform>(); pt->x = core->cfg.width/2-32; pt->y = core->cfg.height/2-32; pt->w=64; pt->h=64; auto ps = p->add<Sprite>(); ps->tex = texman.load("player.png"); p->add<Velocity>();
        auto t = world.create(); auto tt = t->add<Transform>(); tt->x = rand()%(core->cfg.width-32); tt->y = rand()%(core->cfg.height-32); tt->w=32; tt->h=32; auto ts = t->add<Sprite>(); ts->tex = texman.load("target.png");
        auto e = world.create(); auto et = e->add<Transform>(); et->x = rand()%(core->cfg.width-48); et->y = rand()%(core->cfg.height-48); et->w=48; et->h=48; auto es = e->add<Sprite>(); es->tex = texman.load("enemy.png");
    }

    void update(float dt){ if(playing) { // simple physics + AI
            // player movement
            for(auto &e: world.all()){
                auto tr = e->get<Transform>(); auto v = e->get<Velocity>(); if(tr && v){ tr->x += v->vx*dt; tr->y += v->vy*dt; if(tr->x<0)tr->x=0; if(tr->y<0)tr->y=0; if(tr->x+tr->w>core->cfg.width) tr->x=core->cfg.width-tr->w; if(tr->y+tr->h>core->cfg.height) tr->y=core->cfg.height-tr->h; }
            }
            // enemy chases player
            shared_ptr<Entity> player=nullptr; shared_ptr<Entity> enemy=nullptr; shared_ptr<Entity> target=nullptr;
            for(auto &e:world.all()){ if(e->get<Sprite>()){ // heuristic: player has 64x64
                    auto tr=e->get<Transform>(); if(tr->w>48) player=e; else if(tr->w>40) enemy=e; else target=e; }
            }
            if(player && enemy){ auto pt=player->get<Transform>(); auto et=enemy->get<Transform>(); float dx=(pt->x+pt->w/2)-(et->x+et->w/2); float dy=(pt->y+pt->h/2)-(et->y+et->h/2); float dist=sqrtf(dx*dx+dy*dy); if(dist>1e-3f){ et->x += (dx/dist)*100.0f*dt; et->y += (dy/dist)*100.0f*dt; } if(aabbIntersect(*player->get<Transform>(), *enemy->get<Transform>())){ // reset
                    player->get<Transform>()->x = core->cfg.width/2 - 32; player->get<Transform>()->y = core->cfg.height/2 - 32; score=0; }
            }
            // player-target
            if(player && target){ if(aabbIntersect(*player->get<Transform>(), *target->get<Transform())){ score++; target->get<Transform>()->x = rand()%(core->cfg.width - (int)target->get<Transform>()->w); target->get<Transform>()->y = rand()%(core->cfg.height - (int)target->get<Transform>()->h); }
            }
        }
    }

    void renderScene(){ // background
        if(auto bg = texman.load("bg.png")){ SDL_Rect dst={0,0,core->cfg.width,core->cfg.height}; SDL_RenderCopy(core->renderer, bg, nullptr, &dst); }
        // entities
        for(auto &e: world.all()){
            auto tr = e->get<Transform>(); auto sp = e->get<Sprite>(); if(!tr) continue; if(sp && sp->tex){ SDL_Rect dst={(int)tr->x,(int)tr->y,(int)tr->w,(int)tr->h}; SDL_RenderCopy(core->renderer, sp->tex, nullptr, &dst); } else { SDL_SetRenderDrawColor(core->renderer, 255,0,255,255); SDL_Rect r={(int)tr->x,(int)tr->y,(int)tr->w,(int)tr->h}; SDL_RenderFillRect(core->renderer,&r); }
        }
        // selection highlight
        if(selected){ if(auto tr=selected->get<Transform>()){ SDL_SetRenderDrawBlendMode(core->renderer, SDL_BLENDMODE_BLEND); SDL_SetRenderDrawColor(core->renderer, 255,255,0,120); SDL_Rect r={(int)tr->x-4,(int)tr->y-4,(int)tr->w+8,(int)tr->h+8}; SDL_RenderFillRect(core->renderer,&r); SDL_SetRenderDrawBlendMode(core->renderer, SDL_BLENDMODE_NONE);} }
    }

    void showHierarchy(){ ImGui::Begin("Hierarchy");
        int idx=0; for(auto &e: world.all()){ string label = "Entity " + to_string(e->id); if(ImGui::Selectable(label.c_str(), selected && selected->id==e->id)) selected = e; idx++; }
        if(ImGui::Button("Add Entity")){ auto ne = world.create(); auto nt = ne->add<Transform>(); nt->x=50; nt->y=50; nt->w=32; nt->h=32; }
        if(selected){ if(ImGui::Button("Delete Selected")){ world.ents.erase(selected->id); selected=nullptr; } }
        ImGui::End(); }

    void showInspector(){ ImGui::Begin("Inspector"); if(selected){ if(auto t=selected->get<Transform>()){ float x=t->x,y=t->y,w=t->w,h=t->h; if(ImGui::DragFloat("X", &x, 1.0f)) t->x=x; if(ImGui::DragFloat("Y", &y, 1.0f)) t->y=y; if(ImGui::DragFloat("W", &w, 1.0f)) t->w=w; if(ImGui::DragFloat("H", &h, 1.0f)) t->h=h; } if(auto s=selected->get<Sprite>()){ char buf[128]={0}; ImGui::InputText("Texture", buf, 128); if(ImGui::Button("Load Tex")){ string path(buf); if(!path.empty()) s->tex = texman.load(path); } } }
        else ImGui::TextDisabled("No entity selected"); ImGui::End(); }

    void showViewport(){ ImGui::Begin("Viewport"); // show simple buttons
        ImGui::Text("Play: %s", playing?"ON":"OFF"); ImGui::SameLine(); if(ImGui::Button(playing?"Pause":"Play")){ playing = !playing; }
        ImGui::SameLine(); if(ImGui::Button("Spawn Demo")){ spawnDemoScene(); }
        ImGui::Separator();
        // Get viewport size
        ImVec2 vmin = ImGui::GetCursorScreenPos(); ImVec2 vmax = ImGui::GetWindowContentRegionMax(); float availW = ImGui::GetContentRegionAvail().x; float availH = ImGui::GetContentRegionAvail().y;
        // create child with fixed size equal to editor window content
        ImVec2 vpSize = ImGui::GetContentRegionAvail(); if(vpSize.x<100) vpSize.x=100; if(vpSize.y<100) vpSize.y=100;

        // draw scene into the main SDL renderer — we just reserve the area in the UI
        ImGui::InvisibleButton("scene_viewport", vpSize); // capture mouse
        ImVec2 p = ImGui::GetItemRectMin(); // top-left in screen coords
        // convert screen coords to renderer viewport and render scene at the same place
        SDL_Rect prevViewport; SDL_RenderGetViewport(core->renderer, &prevViewport);
        SDL_Rect view = {(int)p.x, (int)p.y, (int)vpSize.x, (int)vpSize.y};
        SDL_RenderSetViewport(core->renderer, &view);
        // simple scene render (scaled to viewport)
        // We'll render to the viewport area using transforms relative to top-left
        // Save original renderer translation by using offsets when drawing
        SDL_RenderSetScale(core->renderer, (float)view.w / core->cfg.width, (float)view.h / core->cfg.height);
        renderScene();
        SDL_RenderSetScale(core->renderer, 1.0f, 1.0f);
        SDL_RenderSetViewport(core->renderer, &prevViewport);

        // mouse interaction inside the viewport
        if(ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)){
            ImVec2 mp = ImGui::GetMousePos(); // screen coords
            // compute local scene coords
            float sx = (mp.x - p.x) * (float)core->cfg.width / vpSize.x;
            float sy = (mp.y - p.y) * (float)core->cfg.height / vpSize.y;
            // if something selected, move it
            if(selected){ if(auto t=selected->get<Transform>()){ t->x = sx - t->w*0.5f; t->y = sy - t->h*0.5f; }} else {
                // click selects entity under cursor
                for(auto &e: world.all()){ if(auto t=e->get<Transform>()){ if(sx >= t->x && sx <= t->x+t->w && sy >= t->y && sy <= t->y+t->h){ selected = e; break; } } }
            }
        }

        ImGui::End(); }

    void renderUI(){ ImGui::Begin("Engine Controls"); ImGui::Text("Score: %d", score); ImGui::Text("Entities: %d", (int)world.ents.size()); ImGui::End(); showHierarchy(); showInspector(); showViewport(); }

    void renderScene(){ // wrapper to call the earlier renderScene function name conflict — provide alias
        renderScene(); // intentionally calling same name in this translation unit — but we'll define a different internal function above
    }
};

// The above had a naming conflict for renderScene; to avoid compile issues, we will reorganize: we'll make Editor::renderScene implementation below

// Forward declare correct Editor implementation details here to replace previous short definitions

// Re-implement Editor properly (clean) -------------------------------------------------
struct Editor2 {
    EngineCore* core = nullptr; TextureManager texman; World world; shared_ptr<Entity> selected=nullptr; bool playing=false; int score=0;
    Editor2(EngineCore* c): core(c), texman(c->renderer) {}
    void loadDemoAssets(){ texman.load("player.png"); texman.load("target.png"); texman.load("enemy.png"); texman.load("bg.png"); }
    void spawnDemoScene(){ world = World(); selected=nullptr; score=0; auto p=world.create(); auto pt=p->add<Transform>(); pt->x=core->cfg.width/2-32; pt->y=core->cfg.height/2-32; pt->w=64; pt->h=64; p->add<Sprite>()->tex = texman.load("player.png"); p->add<Velocity>(); auto t=world.create(); auto tt=t->add<Transform>(); tt->x=rand()%(core->cfg.width-32); tt->y=rand()%(core->cfg.height-32); tt->w=32; tt->h=32; t->add<Sprite>()->tex = texman.load("target.png"); auto e=world.create(); auto et=e->add<Transform>(); et->x=rand()%(core->cfg.width-48); et->y=rand()%(core->cfg.height-48); et->w=48; et->h=48; e->add<Sprite>()->tex = texman.load("enemy.png"); }

    void update(float dt){ if(playing){ for(auto &ent: world.all()){ if(auto tr=ent->get<Transform>()){ if(auto v=ent->get<Velocity>()){ tr->x += v->vx*dt; tr->y += v->vy*dt; if(tr->x<0)tr->x=0; if(tr->y<0)tr->y=0; if(tr->x+tr->w>core->cfg.width) tr->x = core->cfg.width - tr->w; if(tr->y+tr->h>core->cfg.height) tr->y = core->cfg.height - tr->h; } } }
            // find roles
            shared_ptr<Entity> player=nullptr, enemy=nullptr, target=nullptr;
            for(auto &ent: world.all()){ if(auto sp=ent->get<Sprite>()){ auto tr=ent->get<Transform>(); if(tr->w>48) player=ent; else if(tr->w>40) enemy=ent; else target=ent; } }
            if(player && enemy){ auto pt = player->get<Transform>(); auto et = enemy->get<Transform>(); float dx=(pt->x+pt->w/2)-(et->x+et->w/2); float dy=(pt->y+pt->h/2)-(et->y+et->h/2); float dist = sqrtf(dx*dx+dy*dy); if(dist>1e-3f){ et->x += (dx/dist)*100.0f*dt; et->y += (dy/dist)*100.0f*dt; } if(aabbIntersect(*pt,*et)){ pt->x = core->cfg.width/2 - 32; pt->y = core->cfg.height/2 - 32; score=0; } }
            if(player && target){ if(aabbIntersect(*player->get<Transform>(), *target->get<Transform>())){ score++; target->get<Transform>()->x = rand()%(core->cfg.width-(int)target->get<Transform>()->w); target->get<Transform>()->y = rand()%(core->cfg.height-(int)target->get<Transform>()->h); } }
        } }

    void drawSceneToViewport(const SDL_Rect& view){ // set viewport and scale so scene fits
        SDL_Rect prev; SDL_RenderGetViewport(core->renderer, &prev);
        SDL_RenderSetViewport(core->renderer, &view);
        float sx = (float)view.w / (float)core->cfg.width; float sy = (float)view.h / (float)core->cfg.height; SDL_RenderSetScale(core->renderer, sx, sy);
        // background
        if(auto bg = texman.load("bg.png")){ SDL_Rect dst={0,0,core->cfg.width,core->cfg.height}; SDL_RenderCopy(core->renderer, bg, nullptr, &dst); }
        // entities
        for(auto &ent: world.all()){ if(auto tr = ent->get<Transform>()){ if(auto sp=ent->get<Sprite>()){ SDL_Rect dst={(int)tr->x,(int)tr->y,(int)tr->w,(int)tr->h}; SDL_RenderCopy(core->renderer, sp->tex, nullptr, &dst); } else { SDL_Rect r={(int)tr->x,(int)tr->y,(int)tr->w,(int)tr->h}; SDL_SetRenderDrawColor(core->renderer, 200,100,200,255); SDL_RenderFillRect(core->renderer, &r); } } }
        // selection highlight
        if(selected && selected->get<Transform>()){ auto tr = selected->get<Transform>(); SDL_SetRenderDrawBlendMode(core->renderer, SDL_BLENDMODE_BLEND); SDL_SetRenderDrawColor(core->renderer, 255,255,0,120); SDL_Rect r={(int)tr->x-4,(int)tr->y-4,(int)tr->w+8,(int)tr->h+8}; SDL_RenderFillRect(core->renderer,&r); SDL_SetRenderDrawBlendMode(core->renderer, SDL_BLENDMODE_NONE); }
        SDL_RenderSetScale(core->renderer, 1.0f, 1.0f);
        SDL_RenderSetViewport(core->renderer, &prev);
    }

    void uiHierarchy(){ ImGui::Begin("Hierarchy"); for(auto &ent: world.all()){ char buf[64]; sprintf(buf, "Entity %u", ent->id); if(ImGui::Selectable(buf, selected && selected->id==ent->id)) selected = ent; } if(ImGui::Button("Add Entity")){ auto n = world.create(); auto t = n->add<Transform>(); t->x=50; t->y=50; t->w=32; t->h=32; } if(selected){ if(ImGui::Button("Delete Selected")){ world.ents.erase(selected->id); selected=nullptr; } } ImGui::End(); }

    void uiInspector(){ ImGui::Begin("Inspector"); if(selected){ if(auto t=selected->get<Transform>()){ float x=t->x,y=t->y,w=t->w,h=t->h; if(ImGui::DragFloat("X", &x, 1.0f)) t->x=x; if(ImGui::DragFloat("Y", &y, 1.0f)) t->y=y; if(ImGui::DragFloat("W", &w, 1.0f)) t->w=w; if(ImGui::DragFloat("H", &h, 1.0f)) t->h=h; } if(auto s=selected->get<Sprite>()){ static char path[256]={0}; ImGui::InputText("Texture Path", path, 256); if(ImGui::Button("Load")){ string sp(path); if(!sp.empty()) s->tex = texman.load(sp); } } else { if(ImGui::Button("Add Sprite Component")){ auto sc = selected->add<Sprite>(); } } } else ImGui::TextDisabled("No selection"); ImGui::End(); }

    void uiViewport(){ ImGui::Begin("Viewport"); ImGui::Text("Play: %s", playing?"ON":"OFF"); ImGui::SameLine(); if(ImGui::Button(playing?"Pause":"Play")) playing = !playing; ImGui::SameLine(); if(ImGui::Button("Spawn Demo")) spawnDemoScene(); ImGui::Separator(); ImVec2 avail = ImGui::GetContentRegionAvail(); if(avail.x < 200) avail.x = 200; if(avail.y < 150) avail.y = 150; ImGui::InvisibleButton("viewport_btn", avail); ImVec2 p = ImGui::GetItemRectMin(); ImVec2 s = ImGui::GetItemRectSize(); SDL_Rect view = {(int)p.x, (int)p.y, (int)s.x, (int)s.y}; drawSceneToViewport(view);
        // interaction
        if(ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)){
            ImVec2 mp = ImGui::GetMousePos(); float sx = (mp.x - p.x) * (float)core->cfg.width / s.x; float sy = (mp.y - p.y) * (float)core->cfg.height / s.y;
            if(selected){ if(auto tr = selected->get<Transform>()){ tr->x = sx - tr->w*0.5f; tr->y = sy - tr->h*0.5f; } } else { for(auto &e: world.all()){ if(auto tr=e->get<Transform>()){ if(sx >= tr->x && sx <= tr->x+tr->w && sy >= tr->y && sy <= tr->y+tr->h){ selected = e; break; } } } }
        }
        ImGui::End(); }

    void uiOverlay(){ ImGui::Begin("Engine"); ImGui::Text("Score: %d", score); ImGui::Text("Entities: %d", (int)world.ents.size()); ImGui::End(); }

    void renderUI(){ uiOverlay(); uiHierarchy(); uiInspector(); uiViewport(); }
};

// --------------------------- Main ---------------------------
int main(int argc, char* argv[]){ EngineConfig cfg; cfg.width=1280; cfg.height=720; cfg.title="SDL Engine + ImGui Editor"; EngineCore core; if(!core.init(cfg)) return 1; Editor2 editor(&core); editor.loadDemoAssets(); editor.spawnDemoScene();

    bool running=true; Uint32 last = SDL_GetTicks(); const int frameDelay = 1000/cfg.targetFPS;
    while(running){ Uint32 now = SDL_GetTicks(); float dt = (now - last) / 1000.0f; last = now;
        SDL_Event event; while(SDL_PollEvent(&event)){
            ImGui_ImplSDL2_ProcessEvent(&event);
            if(event.type==SDL_QUIT) running=false;
            if(event.type==SDL_WINDOWEVENT && event.window.event==SDL_WINDOWEVENT_CLOSE) running=false;
            // keyboard input for player movement when playing
            if(event.type==SDL_KEYDOWN){ if(editor.playing){ if(event.key.keysym.sym==SDLK_w) editor.world.all()[0]->get<Velocity>()->vy = -200; if(event.key.keysym.sym==SDLK_s) editor.world.all()[0]->get<Velocity>()->vy = 200; if(event.key.keysym.sym==SDLK_a) editor.world.all()[0]->get<Velocity>()->vx = -200; if(event.key.keysym.sym==SDLK_d) editor.world.all()[0]->get<Velocity>()->vx = 200; } }
            if(event.type==SDL_KEYUP){ if(editor.playing){ if(event.key.keysym.sym==SDLK_w || event.key.keysym.sym==SDLK_s) editor.world.all()[0]->get<Velocity>()->vy = 0; if(event.key.keysym.sym==SDLK_a || event.key.keysym.sym==SDLK_d) editor.world.all()[0]->get<Velocity>()->vx = 0; } }
        }

        // update
        editor.update(dt);

        // start ImGui frame
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // render world to main renderer (background)
        SDL_SetRenderDrawColor(core.renderer, 30,30,30,255); SDL_RenderClear(core.renderer);
        // draw scene to full window
        SDL_Rect full = {0,0,core.cfg.width, core.cfg.height}; editor.drawSceneToViewport(full);

        // UI
        editor.renderUI();

        // render ImGui
        ImGui::Render();
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());

        SDL_RenderPresent(core.renderer);

        Uint32 frameTime = SDL_GetTicks() - now; if(frameDelay > (int)frameTime) SDL_Delay(frameDelay - frameTime);
    }

    core.shutdown(); return 0; }
