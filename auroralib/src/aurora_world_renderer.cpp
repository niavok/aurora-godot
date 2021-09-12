#include "aurora_world_renderer.h"

#include <Input.hpp>
#include <InputEventMouseButton.hpp>
#include <Vector2.hpp>
#include <Vector3.hpp>
#include <Viewport.hpp>
#include <GlobalConstants.hpp>
#include <cassert>

namespace godot
{


void WorldRenderer::_register_methods() {
    register_method("_process", &WorldRenderer::_process);
    register_method("_draw", &WorldRenderer::_draw);
    register_method("_input", &WorldRenderer::_input);

    register_property<WorldRenderer, Vector2>("RENDER_DEBUG_OFFSET", &WorldRenderer::RENDER_DEBUG_OFFSET, Vector2(0,0));
    register_property<WorldRenderer, float>("RENDER_DEBUG_SCALE", &WorldRenderer::RENDER_DEBUG_SCALE, 4);
    register_property<WorldRenderer, float>("WORLD_BLOCK_SIZE", &WorldRenderer::WORLD_BLOCK_SIZE, 1);
    register_property<WorldRenderer, float>("RENDER_DEBUG_VELOCITY_SCALE", &WorldRenderer::RENDER_DEBUG_VELOCITY_SCALE, 1);
    register_property<WorldRenderer, float>("m_worldStepDt", &WorldRenderer::m_worldStepDt, 0.01f);
    register_property<WorldRenderer, float>("RENDER_DEBUG_DENSITY", &WorldRenderer::RENDER_DEBUG_DENSITY, 0.01f);
}


WorldRenderer::WorldRenderer()
{
    m_world = new AuroraWorld();
}

void WorldRenderer::FillTile(bool fill, Vector2 mousePosition)
{

    Vector2 relativeWorldPosition = (mousePosition - RENDER_DEBUG_OFFSET) / RENDER_DEBUG_SCALE;

    AVectorI cursorCenterTilePosition(relativeWorldPosition / aurora::TILE_SIZE);

    
    AVectorI cursorCenterTilePositionLocal = m_localTileReferenceCoord + cursorCenterTilePosition;

    int cursorRadius = (m_cursorSize - 1) * 2 + 1;
    AVectorI cursorSize(cursorRadius);
        
    AVectorI cursorCenterTopLeft = cursorCenterTilePositionLocal - AVectorI(m_cursorSize - 1);
    ARectI cursorArea(cursorCenterTopLeft, cursorSize);


    m_world->SetTileMaterial(cursorArea, fill ? aurora::TileMaterial::Aerock : TileMaterial::Air);
}

WorldRenderer::~WorldRenderer() {
    delete m_world;
    m_world = nullptr;
}

void WorldRenderer::_init()
{
    Godot::print("WorldRenderer::_init");
    RENDER_DEBUG_OFFSET = Vector2();
    RENDER_DEBUG_SCALE = 4;
    WORLD_BLOCK_SIZE = 1;
    RENDER_DEBUG_VELOCITY_SCALE = 1;
    m_worldStepDt = 0.001f;
    RENDER_DEBUG_DENSITY = 0.01f;
}

void WorldRenderer::_input(Variant event)
{
    Ref<InputEventMouseButton> mouseButtonEvent = event;
    if (mouseButtonEvent->is_pressed())
    {
        if (mouseButtonEvent->get_button_index() == GlobalConstants::BUTTON_WHEEL_UP)
        {
            m_cursorSize++;
        }

        if (mouseButtonEvent->get_button_index() == GlobalConstants::BUTTON_WHEEL_DOWN)
        {
            if (m_cursorSize > 1)
            {
                m_cursorSize--;
            }
        }

       /* 

        if (emb.IsPressed()) {
            if (emb.ButtonIndex == (int)ButtonList.WheelUp) {
                GD.Print(emb.AsText());
            }
            if (emb.ButtonIndex == (int)ButtonList.WheelDown) {
                GD.Print(emb.AsText());
                */
    }
}


void WorldRenderer::_process(float delta)
{
    Input* input = Input::get_singleton();

    if (input->is_action_just_pressed("world_debug_increase_simulation_speed"))
    {
        m_worldStepDt *= 1.2f;
        Godot::print("m_worldStepDt={0}", m_worldStepDt);
    }

    if (input->is_action_just_pressed("world_debug_decrease_simulation_speed"))
    {
        m_worldStepDt /= 1.2f;
        Godot::print("m_worldStepDt={0}", m_worldStepDt);
    }

    if (input->is_action_just_pressed("world_toogle_pause"))
    {
        m_paused = !m_paused;
        Godot::print("m_paused={0}", m_paused);
    }

    if (!m_paused || input->is_action_just_pressed("world_step"))
    {
        StepWorld(m_worldStepDt);
        //update();
    }
        
    if (input->is_action_just_pressed("world_toogle_pause") || input->is_action_just_pressed("world_step"))
    {
        PrintWorldState();
    }

    if (input->is_mouse_button_pressed(1))
    {
        Viewport* viewport = get_viewport();
        Vector2 origin = viewport->get_canvas_transform().get_origin();
        

        Vector2 mousePosition = get_local_mouse_position() - origin;
        FillTile(true, mousePosition);
    }

    if (input->is_mouse_button_pressed(2))
    {
        Viewport* viewport = get_viewport();
        Vector2 origin = viewport->get_canvas_transform().get_origin();


        Vector2 mousePosition = get_local_mouse_position() - origin;
        FillTile(false, mousePosition);
    }

    update();

}


void WorldRenderer::StepWorld(float dt)
{
    m_totalDuration += dt;
    static float yO = 0;

    static int stepCount = 0;


    stepCount++;
    //yO += 10.f * dt;

    if (yO > 20)
    {
        yO = -50;
    }

    static float yB = 10;

    //yB += 1.f * dt;

    if (yB > 10)
    {
        yB = -10;
    }

    //for (int j = 0; j < 20; j++)
    //{
    //    for (int i = 0; i < 10; i++)
    //    {
    //        FluidBoxAddDensity(m_fluidBox, i, j, 100.f);
    //        FluidBoxAddVelocity(m_fluidBox, i, j, 0.1f, 0.1f + yO);
    //    }
    //}

    m_world->Step(dt);
}

void WorldRenderer::PrintWorldState()
{
    Godot::print("Step");
}

Vector2 WorldRenderer::WorldPositionToDraw(AVectorI worldPosition)
{
    AVectorI  positionInReference = worldPosition - m_localTileReferenceCoord;
    return RENDER_DEBUG_OFFSET + positionInReference * TILE_SIZE * RENDER_DEBUG_SCALE;
}

Vector2 WorldRenderer::WorldSizeToDraw(AVectorI worldSize)
{
    return worldSize * TILE_SIZE * RENDER_DEBUG_SCALE;
};

void WorldRenderer::DrawRect(AVectorI worldPosition, AVectorI worldSize, Color color, bool filled, real_t width)
{
    draw_rect(Rect2(WorldPositionToDraw(worldPosition), WorldSizeToDraw(worldSize)), color, filled, width);
};

void WorldRenderer::DrawLine(AVectorI worldPosition1, AVectorI worldPosition2, Color color, real_t width)
{
    draw_line(WorldPositionToDraw(worldPosition1), WorldPositionToDraw(worldPosition2), color, width);
};

void WorldRenderer::_draw()
{

    ARectI drawRect;
    drawRect.position = m_localTileReferenceCoord;
    drawRect.size = AVectorI(0,0);

    m_world->SelectChunks(drawRect, [this](AuroraWorldChunk& chunk, ARectI& localRect)
        {
            DrawChunk(chunk);
        });

    // Draw Cursor
    Viewport* viewport = get_viewport();
    Vector2 origin = viewport->get_canvas_transform().get_origin();


    Vector2 mousePosition = get_local_mouse_position() - origin;
    Vector2 relativeWorldPosition = (mousePosition - RENDER_DEBUG_OFFSET) / RENDER_DEBUG_SCALE;

    AVectorI cursorCenterTilePosition(relativeWorldPosition / aurora::TILE_SIZE);
    int cursorRadius = (m_cursorSize - 1) * 2 + 1;
    AVectorI cursorArea(cursorRadius, cursorRadius);
    AVectorI cursorCenterTopLeft = cursorCenterTilePosition - AVectorI(m_cursorSize-1, m_cursorSize-1);
    
    DrawRect(cursorCenterTopLeft, cursorArea, Color(0.f, 1.f, 0), false);

}


Color WorldRenderer::GetMaterialDebugColor(TileMaterial material, bool border)
{
    Color materialColor;
    switch (material)
    {
    case TileMaterial::Air:
        materialColor = Color(0.5f, 0.6f, 0.7f, 0.1f);
        break;
    case TileMaterial::Aerock:
        materialColor = Color(0.3f, 0.1f, 0.1f, 1.f);
        break;
    case TileMaterial::Vaccum:
        materialColor = Color(0.f, 0.f, 0.f, 0.f);
        break;
    default:
        assert(false);
        materialColor = Color(1.0f, 0.f, 0.f);
    }

    if (border)
    {
        materialColor *= 1.2f;
    }

    return materialColor;
}
;
Color chunkBorderColor(0.5f, 0.6f, 0.7f, 1.f);


void WorldRenderer::DrawChunk(AuroraWorldChunk& chunk)
{

    AVectorI chunkTilePosition = chunk.GetChunkCoord() * TILE_PER_CHUNK_LINE;
    
    if (chunk.IsHomogeneous())
    {
        Color chunkColor = GetMaterialDebugColor(chunk.GetChunkMaterial());
        Color chunkBorderColor = GetMaterialDebugColor(chunk.GetChunkMaterial(), true);

        AVectorI chunkTileSize(TILE_PER_CHUNK_LINE, TILE_PER_CHUNK_LINE);

        DrawRect(chunkTilePosition, chunkTileSize, chunkColor, true);
        DrawRect(chunkTilePosition, chunkTileSize, chunkBorderColor, false);
    }
    else
    {
        AVectorI blockCoord;
        for (blockCoord.x = 0; blockCoord.x < BLOCK_PER_CHUNK_LINE; blockCoord.x++)
        {
            for (blockCoord.y = 0; blockCoord.y < BLOCK_PER_CHUNK_LINE; blockCoord.y++)
            {
                AuroraWorldBlock& block = chunk.GetBlock(blockCoord);
                AVectorI blockTilePosition = chunkTilePosition + blockCoord * TILE_PER_BLOCK_LINE;
                

                DrawBlock(block, blockTilePosition);
            }
        }
    }
}

void WorldRenderer::DrawBlock(AuroraWorldBlock& block, AVectorI drawTilePosition)
{
    if (block.IsHomogeneous())
    {
        Color blockColor = GetMaterialDebugColor(block.GetBlockMaterial());
        Color blockBorderColor = GetMaterialDebugColor(block.GetBlockMaterial(), true);

        AVectorI blockTileSize(TILE_PER_BLOCK_LINE, TILE_PER_BLOCK_LINE);

        DrawRect(drawTilePosition, blockTileSize, blockColor, true);
        DrawRect(drawTilePosition, blockTileSize, blockBorderColor, false);
    }
    else
    {
        AVectorI tileCoord;
        for (tileCoord.x = 0; tileCoord.x < TILE_PER_BLOCK_LINE; tileCoord.x++)
        {
            for (tileCoord.y = 0; tileCoord.y < TILE_PER_BLOCK_LINE; tileCoord.y++)
            {
                AuroraWorldTile& tile = block.GetTile(tileCoord);
                AVectorI tileTilePosition = drawTilePosition + tileCoord;

                DrawTile(tile, tileTilePosition);
            }
        }
    }
}

void WorldRenderer::DrawTile(AuroraWorldTile& tile, AVectorI drawTilePosition)
{
    Color blockColor = GetMaterialDebugColor(tile.GetTileMaterial());
    Color blockBorderColor = GetMaterialDebugColor(tile.GetTileMaterial(), true);

    AVectorI tileSize(1, 1);

    DrawRect(drawTilePosition, tileSize, blockColor, true);
    //DrawRect(drawTilePosition, tileSize, blockBorderColor, false);
}


}