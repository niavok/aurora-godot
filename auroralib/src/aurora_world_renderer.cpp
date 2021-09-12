#include "aurora_world_renderer.h"

#include <Input.hpp>
#include <Vector2.hpp>
#include <Vector3.hpp>
#include <Viewport.hpp>
#include <cassert>

namespace godot
{


void WorldRenderer::_register_methods() {
    register_method("_process", &WorldRenderer::_process);
    register_method("_draw", &WorldRenderer::_draw);

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

    int i = int(relativeWorldPosition.x / aurora::TILE_SIZE);
    int j = int(relativeWorldPosition.y / aurora::TILE_SIZE);

    AVectorI tileCoord = m_localTileReferenceCoord + AVectorI(i, j);

    if (j < 0 || j >= m_world->GetTileCount().y)
    {
        return;
    }

    m_world->SetTileMaterial(tileCoord, aurora::TileMaterial::Aerock);
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



    //    Vector2 relativeWorldPosition = (mousePosition - RENDER_DEBUG_OFFSET) / RENDER_DEBUG_SCALE;

    //float blockSize = m_world->GetTileSize();

    //int i = int(relativeWorldPosition.x / blockSize);
    //int j = int(relativeWorldPosition.y / blockSize);

    //AVectorI tileCoord = m_localTileReferenceCoord + AVectorI(i, j);


    //auto DrawBorder = [&]()
    //{
    //    // First fill only
    //    for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
    //    {
    //        for (int i = 0; i < m_fluidBox->m_blockCountX; i++)
    //        {
    //            Vector2 position((real_t)i * blockSize, (real_t)j * blockSize);
    //            int index = m_fluidBox->BlockIndex(i, j);

    //            if(m_fluidBox->m_blockConfig[index].isFill)
    //            {
    //                DrawRect(position, Vector2(blockSize, blockSize), Color(0.5f, 0.5f, 0.5f), true);
    //            }
    //        }
    //    }

    //    for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
    //    {
    //        for (int i = 0; i < m_fluidBox->m_blockCountX; i++)
    //        {
    //            Vector2 position((real_t)i * blockSize, (real_t)j * blockSize);
    //            int index = m_fluidBox->BlockIndex(i, j);

    //            FluidBox::BlockConfig& blockConfig = m_fluidBox->m_blockConfig[index];

    //            if (!blockConfig.isTopConnected)
    //            {
    //                DrawLine(position, position + Vector2(blockSize, 0), Color(1.0f, 1.f, 1.0f), true);
    //            }

    //            if (!blockConfig.isBottomConnected)
    //            {
    //                DrawLine(position + Vector2(0, blockSize), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //            }

    //            if (!blockConfig.isLeftConnected)
    //            {
    //                DrawLine(position, position + Vector2(0, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //            }

    //            if (!blockConfig.isRightConnected)
    //            {
    //                DrawLine(position + Vector2(blockSize, 0), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //            }

    //            /*uint8_t type = m_fluidBox->m_blockEdgeType[index];
    //            switch (type)
    //            {
    //            case FluidBox::BlockEdge_VOID:
    //            case FluidBox::BlockEdge_FILL:
    //                break;
    //            case FluidBox::BlockEdge_TOP_EDGE:
    //                DrawLine(position, position + Vector2(blockSize, 0), Color(1.0f, 1.f, 1.0f), true);
    //                break;
    //            case FluidBox::BlockEdge_BOTTOM_EDGE:
    //                DrawLine(position + Vector2(0, blockSize), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //                break;
    //            case FluidBox::BlockEdge_LEFT_EDGE:
    //                DrawLine(position, position + Vector2(0, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //                break;
    //            case FluidBox::BlockEdge_RIGHT_EDGE:
    //                DrawLine(position + Vector2(blockSize, 0), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //                break;
    //            case FluidBox::BlockEdge_LOOPING_LEFT_VOID:
    //                DrawLine(position, position + Vector2(0, blockSize), Color(0.3f, 0.3f, 0.3f), true);
    //                break;
    //            case FluidBox::BlockEdge_LOOPING_RIGHT_VOID:
    //                DrawLine(position + Vector2(blockSize, 0), position + Vector2(blockSize, blockSize), Color(0.3f, 0.3f, 0.3f), true);
    //                break;
    //            case FluidBox::BlockEdge_LOOPING_LEFT_TOP_EDGE:
    //                DrawLine(position, position + Vector2(0, blockSize), Color(0.3f, 0.3f, 0.3f), true);
    //                DrawLine(position, position + Vector2(blockSize, 0), Color(1.0f, 1.f, 1.0f), true);
    //                break;
    //            case FluidBox::BlockEdge_LOOPING_LEFT_BOTTOM_EDGE:
    //                DrawLine(position, position + Vector2(0, blockSize), Color(0.3f, 0.3f, 0.3f), true);
    //                DrawLine(position + Vector2(0, blockSize), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //                break;
    //            case FluidBox::BlockEdge_LOOPING_RIGHT_TOP_EDGE:
    //                DrawLine(position + Vector2(blockSize, 0), position + Vector2(blockSize, blockSize), Color(0.3f, 0.3f, 0.3f), true);
    //                DrawLine(position, position + Vector2(blockSize, 0), Color(1.0f, 1.f, 1.0f), true);
    //                break;
    //            case FluidBox::BlockEdge_LOOPING_RIGHT_BOTTOM_EDGE:
    //                DrawLine(position + Vector2(blockSize, 0), position + Vector2(blockSize, blockSize), Color(0.3f, 0.3f, 0.3f), true);
    //                DrawLine(position + Vector2(0, blockSize), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //                break;
    //            case FluidBox::BlockEdge_TOP_LEFT_CORNER:
    //                DrawLine(position, position + Vector2(blockSize, 0), Color(1.0f, 1.f, 1.0f), true);
    //                DrawLine(position, position + Vector2(0, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //                break;
    //            case FluidBox::BlockEdge_TOP_RIGHT_CORNER:
    //                DrawLine(position, position + Vector2(blockSize, 0), Color(1.0f, 1.f, 1.0f), true);
    //                DrawLine(position + Vector2(blockSize, 0), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //                break;
    //            case FluidBox::BlockEdge_BOTTOM_LEFT_CORNER:
    //                DrawLine(position + Vector2(0, blockSize), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //                DrawLine(position, position + Vector2(0, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //                break;
    //            case FluidBox::BlockEdge_BOTTOM_RIGHT_CORNER:
    //                DrawLine(position + Vector2(0, blockSize), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //                DrawLine(position + Vector2(blockSize, 0), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //                break;
    //            case FluidBox::BlockEdge_HORIZONTAL_PIPE:
    //                DrawLine(position, position + Vector2(blockSize, 0), Color(1.0f, 1.f, 1.0f), true);
    //                DrawLine(position + Vector2(0, blockSize), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
    //                break;
    //            default:
    //                DrawRect(position, Vector2(blockSize, blockSize), Color(1.f, 0.f, 0.f, 0.5f), true);
    //            }*/
    //        }
    //    }

    //    if (false)
    //    {
    //        // Velocity type check
    //        for (int j = 0; j < m_fluidBox->m_horizontalVelocityCountY; j++)
    //        {
    //            for (int i = 0; i < m_fluidBox->m_horizontalVelocityCountX; i++)
    //            {
    //                Vector2 worldPosition(blockSize * i, blockSize * 0.5f + blockSize * j);
    //                Vector2 drawPosition = WorldPositionToDraw(worldPosition);
    //                int index = i + j * m_fluidBox->m_horizontalVelocityCountX;
    //                Color color;

    //                uint8_t type = m_fluidBox->m_horizontalVelocityType[index];

    //                if (type == FluidBox::Velocity_FREE)
    //                {
    //                    continue;
    //                }
    //                else if (type == FluidBox::Velocity_ZERO)
    //                {
    //                    color = Color(1.f, 0.f, 0.f, 1.0);
    //                }
    //                else if (type == FluidBox::Velocity_LOOPING_LEFT)
    //                {
    //                    color = Color(0.f, 1.f, 0.f, 1.0);
    //                }
    //                else if (type == FluidBox::Velocity_LOOPING_RIGHT)
    //                {
    //                    color = Color(1.f, 0.f, 1.f, 1.0);
    //                }

    //                draw_line(drawPosition + Vector2(-1, 0), drawPosition + Vector2(1, 0), color, 1.f);
    //            }
    //        }

    //        for (int j = 0; j < m_fluidBox->m_verticalVelocityCountY; j++)
    //        {
    //            for (int i = 0; i < m_fluidBox->m_verticalVelocityCountX; i++)
    //            {
    //                Vector2 worldPosition(blockSize * 0.5f + blockSize * i, blockSize * j);
    //                Vector2 drawPosition = WorldPositionToDraw(worldPosition);
    //                int index = i + j * m_fluidBox->m_verticalVelocityCountX;
    //                Color color;

    //                uint8_t type = m_fluidBox->m_verticalVelocityType[index];

    //                if (type == FluidBox::Velocity_FREE)
    //                {
    //                    continue;
    //                }
    //                else if (type == FluidBox::Velocity_ZERO)
    //                {
    //                    color = Color(1.f, 0.f, 0.f, 1.0);
    //                }
    //                else if (type == FluidBox::Velocity_LOOPING_LEFT)
    //                {
    //                    color = Color(0.f, 1.f, 0.f, 1.0);
    //                }
    //                else if (type == FluidBox::Velocity_LOOPING_RIGHT)
    //                {
    //                    color = Color(1.f, 0.f, 1.f, 1.0);
    //                }

    //                draw_line(drawPosition + Vector2(-1, 0), drawPosition + Vector2(1, 0), color, 1.f);
    //            }
    //        }
    //    }
    //};

    //if (true)
    //{
    //    DrawBorder();
    //   

    //    if (true)
    //    {
    //        float* horizontalVelocity = m_fluidBox->m_horizontalVelocityBuffer[m_fluidBox->m_activeVelocityBufferIndex];
    //        float* verticalVelocity = m_fluidBox->m_verticalVelocityBuffer[m_fluidBox->m_activeVelocityBufferIndex];
    //        int horizontalVelocityCountX = m_fluidBox->m_horizontalVelocityCountX;
    //        int verticalVelocityCountX = m_fluidBox->m_verticalVelocityCountX;

    //        auto LeftHorizontalVelocity = [horizontalVelocityCountX, horizontalVelocity](int blockX, int blockY) -> float {
    //            int velocityIndex = blockX + horizontalVelocityCountX * blockY;
    //            return horizontalVelocity[velocityIndex];
    //        };

    //        auto RightHorizontalVelocity = [horizontalVelocityCountX, horizontalVelocity](int blockX, int blockY) -> float {
    //            int velocityIndex = blockX + horizontalVelocityCountX * blockY + 1;
    //            return horizontalVelocity[velocityIndex];
    //        };

    //        auto RightHorizontalVelocityLooping = [horizontalVelocityCountX, horizontalVelocity](int blockX, int blockY) -> float {
    //            int velocityIndex = blockX + horizontalVelocityCountX * (blockY - 1) + 1;
    //            return horizontalVelocity[velocityIndex];
    //        };

    //        auto TopVerticalVelocity = [verticalVelocityCountX, verticalVelocity](int blockX, int blockY) -> float {
    //            int velocityIndex = blockX + verticalVelocityCountX * blockY;
    //            return verticalVelocity[velocityIndex];
    //        };

    //        auto BottomVerticalVelocity = [verticalVelocityCountX, verticalVelocity](int blockX, int blockY) -> float {
    //            int velocityIndex = blockX + verticalVelocityCountX * (blockY + 1);
    //            return verticalVelocity[velocityIndex];
    //        };

    //        // Velocity
    //        int vStep = 2;
    //        for (int j = 0; j < m_fluidBox->m_blockCountY; j+= vStep)
    //        {
    //            for (int i = 0; i < m_fluidBox->m_blockCountX; i+=vStep)
    //            {
    //                int index = m_fluidBox->BlockIndex(i, j);

    //                if (m_fluidBox->m_blockConfig[index].isFill)
    //                {
    //                    continue;
    //                }

    //                Vector2 position((real_t)i* blockSize, (real_t)j* blockSize);
    //                Vector2 center_pos = position + Vector2(blockSize, blockSize) * 0.5f;


    //                float leftVelocity = LeftHorizontalVelocity(i, j);
    //                float rightVelocity;
    //                if (i == m_fluidBox->m_blockCountX - 1 && m_fluidBox->m_isHorizontalLoop)
    //                {
    //                    rightVelocity  = RightHorizontalVelocityLooping(i, j);
    //                }
    //                else
    //                {
    //                    rightVelocity = RightHorizontalVelocity(i, j);
    //                }

    //                float topVelocity = TopVerticalVelocity(i, j);
    //                float bottomVelocity = BottomVerticalVelocity(i, j);
    //                
    //                Vector2 velocity(0.5f * (leftVelocity + rightVelocity), 0.5f * (topVelocity + bottomVelocity));


    //                //if (velocity.length_squared() > 100)
    //                //{
    //                //    int plop = 1;
    //                //    for (int x = 0; x < 50; x++)
    //                //    {
    //                //        printf("%d: %10.15g -> %10.15g \n", x, RightHorizontalVelocity(i + x, j), RightHorizontalVelocity(i + x, j + 1));
    //                //    }


    //                //}

    //                if (velocity.length_squared() < 1e-5)
    //                {
    //                    continue;
    //                }

    //                float velocityAngle = velocity.angle();

    //                float red = 0.5f * (1 + sinf(velocityAngle));
    //                float green = 0.5f * (1 + sinf(velocityAngle + 2 * float(Math_PI) / 3.0f)); //  + 60°
    //                float blue = 0.5f * (1 + sinf(velocityAngle + 4 * float(Math_PI) / 3.0f)); //  + 120°


    //                Color color(red, green, blue);

    //                Vector2 velocity_pos = center_pos + velocity * RENDER_DEBUG_VELOCITY_SCALE;

    //                if (WorldSizeToDraw(velocity * RENDER_DEBUG_VELOCITY_SCALE).length_squared() > 1.f)
    //                {
    //                    DrawLine(center_pos, velocity_pos, color, 1);
    //                }
    //            }
    //        }
    //    }

    //    drawPosition += WorldSizeToDraw(Vector2(0, m_fluidBox->m_blockCountY * m_fluidBox->m_blockSize));

    //    DrawBorder();

    //    // Content
    //    FluidBox::Content* content = m_fluidBox->m_contentBuffer[m_fluidBox->m_activeContentBufferIndex];
    //    float opacityCoeff = RENDER_DEBUG_DENSITY;

    //    for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
    //    {
    //        for (int i = 0; i < m_fluidBox->m_blockCountX; i++)
    //        {
    //            int index = m_fluidBox->BlockIndex(i, j);

    //            FluidBox::Content& blockContent = content[index];

    //            Vector3 composition(blockContent.totalContent.density0, blockContent.totalContent.density1, blockContent.totalContent.density2);

    //            float density = composition.x + composition.y + composition.z;

    //            Vector3 dyeColor = composition.normalized();


    //            Color tileColor(dyeColor.x, dyeColor.y, dyeColor.z, density * opacityCoeff);

    //            Vector2 position((real_t)i* blockSize, (real_t)j* blockSize);

    //            DrawRect(position, Vector2(blockSize, blockSize), tileColor, true);
    //        }
    //    }
    //}

}


Color WorldRenderer::GetMaterialDebugColor(TileMaterial material, bool border)
{
    Color materialColor;
    switch (material)
    {
    case TileMaterial::Air:
        materialColor = Color(0.5f, 0.6f, 0.7f, 0.1);
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