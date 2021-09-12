#ifndef AURORA_WORLD_RENDERER_H
#define AURORA_WORLD_RENDERER_H

#include <Godot.hpp>
#include <Sprite.hpp>

#include "aurora_world.h"

namespace godot {


using namespace aurora;


class WorldRenderer : public Sprite {
GODOT_CLASS(WorldRenderer, Sprite)

public:
    static void _register_methods();

    WorldRenderer();
    ~WorldRenderer();

    void _init(); // our initializer called by Godot

    void _process(float delta);

    void _input(Variant event);


    void _draw();
private:
    void StepWorld(float dt);
    void PrintWorldState();
    void FillTile(bool fill, Vector2 mousePosition);


    Vector2 WorldPositionToDraw(AVectorI worldPosition);
    Vector2 WorldSizeToDraw(AVectorI worldSize);

    void DrawRect(AVectorI worldPosition, AVectorI worldSize, Color color, bool filled, real_t width = 1.f);
    void DrawLine(AVectorI worldPosition1, AVectorI worldPosition2, Color color, real_t width = 1.f);

    Color GetMaterialDebugColor(TileMaterial material, bool border = false);


    void DrawChunk(AuroraWorldChunk& chunk);
    void DrawBlock(AuroraWorldBlock& block, AVectorI drawTilePosition);
    void DrawTile(AuroraWorldTile& tile, AVectorI drawTilePosition);

    AuroraWorld* m_world;
    AVectorI m_localTileReferenceCoord;

    bool m_paused = true;
    
    float m_totalDuration = 0;
    int m_cursorSize = 1;


    float m_worldStepDt;
    Vector2 RENDER_DEBUG_OFFSET;
    float RENDER_DEBUG_SCALE;
    float WORLD_BLOCK_SIZE;
    float RENDER_DEBUG_VELOCITY_SCALE;
    float RENDER_DEBUG_DENSITY;
};



}

#endif