#include "aurora_fluids.h"

#include <Input.hpp>
#include <Vector2.hpp>
#include <Vector3.hpp>

namespace godot
{

void Fluids::_register_methods() {
    register_method("_process", &Fluids::_process);
    register_method("_draw", &Fluids::_draw);

    register_property<Fluids, float>("diffusion_coef", &Fluids::diffusion_coef, 0.1f);
    register_property<Fluids, float>("viscosity_coef", &Fluids::viscosity_coef, 0.1f);
    register_property<Fluids, Vector2>("RENDER_DEBUG_OFFSET", &Fluids::RENDER_DEBUG_OFFSET, Vector2(0,0));
    register_property<Fluids, float>("RENDER_DEBUG_SCALE", &Fluids::RENDER_DEBUG_SCALE, 4);
    register_property<Fluids, float>("WORLD_BLOCK_SIZE", &Fluids::WORLD_BLOCK_SIZE, 1);
    register_property<Fluids, float>("RENDER_DEBUG_VELOCITY_SCALE", &Fluids::RENDER_DEBUG_VELOCITY_SCALE, 1);
    register_property<Fluids, float>("m_worldStepDt", &Fluids::m_worldStepDt, 0.01f);
    register_property<Fluids, float>("RENDER_DEBUG_DENSITY", &Fluids::RENDER_DEBUG_DENSITY, 0.01f);
}


Fluids::Fluids()
{
    m_fluidBox = new FluidBox(250, 100, 150, true); // 51.2 km * 15 km

    //m_fluidBox->blockEdgeType[m_fluidBox->BlockIndex(0, 0)] = FluidBox::BlockEdge_LOOPING_TOP_LEFT_CORNER;
    //m_fluidBox->blockEdgeType[m_fluidBox->BlockIndex(m_fluidBox->m_blockCountX-1, 0)] = FluidBox::BlockEdge_LOOPING_TOP_RIGHT_CORNER;
    //m_fluidBox->blockEdgeType[m_fluidBox->BlockIndex(0, m_fluidBox->m_blockCountY-1)] = FluidBox::BlockEdge_LOOPING_BOTTOM_LEFT_CORNER;
    //m_fluidBox->blockEdgeType[m_fluidBox->BlockIndex(m_fluidBox->m_blockCountX - 1, m_fluidBox->m_blockCountY-1)] = FluidBox::BlockEdge_LOOPING_BOTTOM_RIGHT_CORNER;

    //for (int j = 1; j < m_fluidBox->m_blockCountY-1; j++)
    //{
    //    m_fluidBox->blockEdgeType[m_fluidBox->BlockIndex(0, j)] = FluidBox::BlockEdge_LOOPING_LEFT_EDGE;
    //    m_fluidBox->blockEdgeType[m_fluidBox->BlockIndex(m_fluidBox->m_blockCountX - 1, j)] = FluidBox::BlockEdge_LOOPING_RIGHT_EDGE;
    //}

    //for (int i = 1; i < m_fluidBox->m_blockCountX - 1; i++)
    //{
    //    m_fluidBox->blockEdgeType[m_fluidBox->BlockIndex(i, 0)] = FluidBox::BlockEdge_TOP_EDGE;
    //    m_fluidBox->blockEdgeType[m_fluidBox->BlockIndex(i, m_fluidBox->m_blockCountY - 1)] = FluidBox::BlockEdge_BOTTOM_EDGE;
    //}

    //for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
    //{
    //    m_fluidBox->blockEdgeType[m_fluidBox->Index(0, j)] = FluidBox::BlockEdge_LOOPING_LEFT_HORIZONTAL_PIPE;
    //    for (int i = 1; i < m_fluidBox->m_blockCountX - 1; i++)
    //    {
    //        m_fluidBox->blockEdgeType[m_fluidBox->Index(i, j)] = FluidBox::BlockEdge_HORIZONTAL_PIPE;
    //    }
    //    m_fluidBox->blockEdgeType[m_fluidBox->Index(m_fluidBox->m_blockCountX - 1, j)] = FluidBox::BlockEdge_LOOPING_RIGHT_HORIZONTAL_PIPE;
    //}

 /*   for (int i = 0; i < m_fluidBox->m_verticalVelocityCountX; i++)
    {
        m_fluidBox->m_verticalVelocityType[m_fluidBox->VerticalVelocityIndex(i, 0)] = FluidBox::Velocity_ZERO;
        m_fluidBox->m_verticalVelocityType[m_fluidBox->VerticalVelocityIndex(i, m_fluidBox->m_verticalVelocityCountY -1)] = FluidBox::Velocity_ZERO;
    }

    for (int j = 0; j < m_fluidBox->m_horizontalVelocityCountY + 1; j++)
    {
        m_fluidBox->m_horizontalVelocityType[m_fluidBox->HorizontalVelocityIndex(0, j)] = FluidBox::Velocity_LOOPING_LEFT;
    }
    
    for (int j = 0; j < m_fluidBox->m_verticalVelocityCountY + 1; j++)
    {
        m_fluidBox->m_verticalVelocityType[m_fluidBox->HorizontalVelocityIndex(0, j)] = FluidBox::Velocity_LOOPING_RIGHT;
    }*/

    // Vertical sep

    auto drawVSep = [&](int sepX, int sepY0, int sepY1)
    {
        //m_fluidBox->blockEdgeType[m_fluidBox->BlockIndex(sepX + 1, sepY0 - 1)] = FluidBox::BlockEdge_BOTTOM_EDGE;
        //m_fluidBox->blockEdgeType[m_fluidBox->BlockIndex(sepX + 1, sepY1)] = FluidBox::BlockEdge_TOP_EDGE;

        //m_fluidBox->velocityYType[m_fluidBox->BlockIndex(sepX + 1, sepY0)] = FluidBox::Velocity_ZERO;
        //m_fluidBox->velocityYType[m_fluidBox->BlockIndex(sepX + 1, sepY1)] = FluidBox::Velocity_ZERO;

        for (int j = sepY0; j < sepY1; j++)
        {
            //m_fluidBox->blockEdgeType[m_fluidBox->BlockIndex(sepX, j)] = FluidBox::BlockEdge_RIGHT_EDGE;
            m_fluidBox->m_blockEdgeType[m_fluidBox->BlockIndex(sepX + 1, j)] = FluidBox::BlockEdge_FILL;
            //m_fluidBox->blockEdgeType[m_fluidBox->BlockIndex(sepX + 2, j)] = FluidBox::BlockEdge_LEFT_EDGE;

            //m_fluidBox->velocityXType[m_fluidBox->BlockIndex(sepX + 1, j)] = FluidBox::Velocity_ZERO;
            //m_fluidBox->velocityXType[m_fluidBox->BlockIndex(sepX + 2, j)] = FluidBox::Velocity_ZERO;
        }
    };
    


    drawVSep(100, 15, 85);

    drawVSep(150, 0, 45);
    drawVSep(150, 55, 100);


    m_fluidBox->CompileGrid();

    //for (int j = 30; j < 90; j++)
    //{
    //    m_fluidBox->blockType[m_fluidBox->Index(100, j)] = 1;
    //    m_fluidBox->blockType[m_fluidBox->Index(101, j)] = 2;
    //}


    //for (int j = 0; j < 40; j++)
    //{
    //    m_fluidBox->blockType[m_fluidBox->Index(250, j)] = 1;
    //    m_fluidBox->blockType[m_fluidBox->Index(251, j)] = 2;
    //}

    //for (int j = 60; j < 100; j++)
    //{
    //    m_fluidBox->blockType[m_fluidBox->Index(250, j)] = 1;
    //    m_fluidBox->blockType[m_fluidBox->Index(251, j)] = 2;
    //}

}

Fluids::~Fluids() {
    delete m_fluidBox;
    m_fluidBox = nullptr;
}

void Fluids::_init()
{
    Godot::print("Fluids::_init");
    diffusion_coef = 0.1f;
    viscosity_coef = 0.1f;
    RENDER_DEBUG_OFFSET = Vector2();
    RENDER_DEBUG_SCALE = 4;
    WORLD_BLOCK_SIZE = 1;
    RENDER_DEBUG_VELOCITY_SCALE = 1;
    m_worldStepDt = 0.001f;
    RENDER_DEBUG_DENSITY = 0.01f;
}

void Fluids::_process(float delta)
{
    Input* input = Input::get_singleton();


    if (input->is_action_just_pressed("world_debug_toogle_sun"))
    {
        m_worldEnableSun = !m_worldEnableSun;
        Godot::print("world_enable_sun={0}", m_worldEnableSun);
    }

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
        update();
    }
        
    if (input->is_action_just_pressed("world_toogle_pause") || input->is_action_just_pressed("world_step"))
    {
        PrintWorldState();
    }

    update();

}


void Fluids::StepWorld(float dt)
{

    totalDuration += dt;
    static float yO = 0;

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

    if (m_worldEnableSun)
    {
        //for (int j = 10; j < m_fluidBox->m_blockCountY-1; j++)
        for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
        {
            m_fluidBox->SetHorizontalVelocityAtLeft(50, j, 100.f);
        }

        for (int j = 0; j < 20; j++)
        {
            for (int i = 0; i < 10; i++)
            {
                m_fluidBox->AddDensity(50 + i, 50 + j, 200.0f * dt, 0);
                
                Vector2 direction(1, yO / 20.f);


                float vel = 200.f * dt;
                //m_fluidBox->AddVelocity(50 + i, 50 + j, direction.x * vel, direction.y * vel);



            }
        }

        for (int j = 0; j < 8; j++)
        {
            for (int i = 0; i < 8; i++)
            {

                m_fluidBox->AddDensity(150 + i, 40 + j, 200.0f * dt, 1);

                Vector2 directionB(-1, yB / 10);
                //Vector2 directionB;
                //directionB.set_rotation(totalDuration * 5);
                float velB = 100.f * dt;

                //m_fluidBox->AddVelocity(150 + i, 40 + j, directionB.x * velB, directionB.y * velB);

            }
        }

        for (int j = 0; j < 8; j++)
        {
            for (int i = 0; i < 8; i++)
            {

                m_fluidBox->AddDensity(100 + i, 80 + j, 200.0f * dt, 2);

            }
        }
    }

    m_fluidBox->DecayDensity(0.99f);


    m_fluidBox->Step(dt, diffusion_coef /** 0.00001f*/, viscosity_coef /** 0.00001f*/);
}

void Fluids::PrintWorldState()
{
    Godot::print("Step");
}

void Fluids::_draw()
{
    Vector2 basePosition = get_position();

    //float offetX = 0;
    //float offetY = 0;
    //float scale = 4;
    //
    //float WORLD_BLOCK_SIZE = 1;
    //float RENDER_DEBUG_VELOCITY_SCALE = 1;
    //float RENDER_DEBUG_SCALE = scale;
    //Vector2 RENDER_DEBUG_OFFSET(offetX, offetY);

    auto WorldPositionToDraw = [&](Vector2 worldPosition)
    {
        return RENDER_DEBUG_OFFSET + worldPosition * RENDER_DEBUG_SCALE;
    };

    auto WorldSizeToDraw = [&](Vector2 worldSize)
    {
        return worldSize * RENDER_DEBUG_SCALE;
    };

    auto DrawRect = [&](Vector2 worldPosition, Vector2 worldSize, Color color, bool filled, real_t width = 1.f)
    {
        draw_rect(Rect2(WorldPositionToDraw(worldPosition), WorldSizeToDraw(worldSize)), color, filled, width);
    };

    auto DrawLine = [&](Vector2 worldPosition1, Vector2 worldPosition2, Color color, real_t width = 1.f)
    {
        draw_line(WorldPositionToDraw(worldPosition1), WorldPositionToDraw(worldPosition2), color, width);
    };

    float blockSize = m_fluidBox->m_blockSize;

    if (true)
    {
        //DrawRect(Vector2(0,0), Vector2(blockSize * m_fluidBox->m_blockCountX, blockSize * m_fluidBox->m_blockCountY), Color(1.f, 1.f, 1.f), false);


        // First fill only
        for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
        {
            for (int i = 0; i < m_fluidBox->m_blockCountX; i++)
            {
                Vector2 position((real_t)i * blockSize, (real_t)j * blockSize);
                int index = m_fluidBox->BlockIndex(i, j);

                uint8_t type = m_fluidBox->m_blockEdgeType[index];
                switch (type)
                {
                case FluidBox::BlockEdge_FILL:
                    DrawRect(position, Vector2(blockSize, blockSize), Color(0.5f, 0.5f, 0.5f), true);
                    break;
                }
            }
        }

        for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
        {
            for (int i = 0; i < m_fluidBox->m_blockCountX; i++)
            {
                Vector2 position((real_t)i * blockSize, (real_t)j * blockSize);
                int index = m_fluidBox->BlockIndex(i, j);

                uint8_t type = m_fluidBox->m_blockEdgeType[index];
                switch (type)
                {
                case FluidBox::BlockEdge_VOID:
                case FluidBox::BlockEdge_FILL:
                    break;
                case FluidBox::BlockEdge_TOP_EDGE:
                    DrawLine(position, position + Vector2(blockSize, 0), Color(1.0f, 1.f, 1.0f), true);
                    break;
                case FluidBox::BlockEdge_BOTTOM_EDGE:
                    DrawLine(position + Vector2(0, blockSize), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
                    break;
                case FluidBox::BlockEdge_LEFT_EDGE:
                    DrawLine(position, position + Vector2(0, blockSize), Color(1.0f, 1.f, 1.0f), true);
                    break;
                case FluidBox::BlockEdge_RIGHT_EDGE:
                    DrawLine(position + Vector2(blockSize, 0), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
                    break;
                case FluidBox::BlockEdge_LOOPING_LEFT_VOID:
                    DrawLine(position, position + Vector2(0, blockSize), Color(0.3f, 0.3f, 0.3f), true);
                    break;
                case FluidBox::BlockEdge_LOOPING_RIGHT_VOID:
                    DrawLine(position + Vector2(blockSize, 0), position + Vector2(blockSize, blockSize), Color(0.3f, 0.3f, 0.3f), true);
                    break;
                case FluidBox::BlockEdge_LOOPING_LEFT_TOP_EDGE:
                    DrawLine(position, position + Vector2(0, blockSize), Color(0.3f, 0.3f, 0.3f), true);
                    DrawLine(position, position + Vector2(blockSize, 0), Color(1.0f, 1.f, 1.0f), true);
                    break;
                case FluidBox::BlockEdge_LOOPING_LEFT_BOTTOM_EDGE:
                    DrawLine(position, position + Vector2(0, blockSize), Color(0.3f, 0.3f, 0.3f), true);
                    DrawLine(position + Vector2(0, blockSize), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
                    break;
                case FluidBox::BlockEdge_LOOPING_RIGHT_TOP_EDGE:
                    DrawLine(position + Vector2(blockSize, 0), position + Vector2(blockSize, blockSize), Color(0.3f, 0.3f, 0.3f), true);
                    DrawLine(position, position + Vector2(blockSize, 0), Color(1.0f, 1.f, 1.0f), true);
                    break;
                case FluidBox::BlockEdge_LOOPING_RIGHT_BOTTOM_EDGE:
                    DrawLine(position + Vector2(blockSize, 0), position + Vector2(blockSize, blockSize), Color(0.3f, 0.3f, 0.3f), true);
                    DrawLine(position + Vector2(0, blockSize), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
                    break;
                case FluidBox::BlockEdge_TOP_LEFT_CORNER:
                    DrawLine(position, position + Vector2(blockSize, 0), Color(1.0f, 1.f, 1.0f), true);
                    DrawLine(position, position + Vector2(0, blockSize), Color(1.0f, 1.f, 1.0f), true);
                    break;
                case FluidBox::BlockEdge_TOP_RIGHT_CORNER:
                    DrawLine(position, position + Vector2(blockSize, 0), Color(1.0f, 1.f, 1.0f), true);
                    DrawLine(position + Vector2(blockSize, 0), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
                    break;
                case FluidBox::BlockEdge_BOTTOM_LEFT_CORNER:
                    DrawLine(position + Vector2(0, blockSize), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
                    DrawLine(position, position + Vector2(0, blockSize), Color(1.0f, 1.f, 1.0f), true);
                    break;
                case FluidBox::BlockEdge_BOTTOM_RIGHT_CORNER:
                    DrawLine(position + Vector2(0, blockSize), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
                    DrawLine(position + Vector2(blockSize, 0), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
                    break;
                case FluidBox::BlockEdge_HORIZONTAL_PIPE:
                    DrawLine(position, position + Vector2(blockSize, 0), Color(1.0f, 1.f, 1.0f), true);
                    DrawLine(position + Vector2(0, blockSize), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
                    break;
                default:
                    DrawRect(position, Vector2(blockSize, blockSize), Color(1.f, 0.f, 0.f, 0.5f), true);
                }
                //if (type != FluidBox::BlockEdge_FILL)
                //{
                //    if (type == FluidBox::BlockEdge_LEFT_EDGE)
                //    {
                //        draw_line(RENDER_DEBUG_OFFSET + position * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET + (position + Vector2(0, WORLD_BLOCK_SIZE)) * RENDER_DEBUG_SCALE, Color(1.f, 1.f, 1.0f), 1);
                //    }
                //    else if (type == FluidBox::BlockEdge_RIGHT_EDGE)
                //    {
                //        draw_line(RENDER_DEBUG_OFFSET + (position + Vector2(WORLD_BLOCK_SIZE, 0)) * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET + (position + Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE)) * RENDER_DEBUG_SCALE, Color(1.f, 1.f, 1.0f), 1);
                //    }
                //}
            }
        }

        if (false)
        {
            // Velocity type check
            for (int j = 0; j < m_fluidBox->m_horizontalVelocityCountY; j++)
            {
                for (int i = 0; i < m_fluidBox->m_horizontalVelocityCountX; i++)
                {
                    Vector2 worldPosition(blockSize * i, blockSize * 0.5f + blockSize* j);
                    Vector2 drawPosition = WorldPositionToDraw(worldPosition);
                    int index = i + j * m_fluidBox->m_horizontalVelocityCountX;
                    Color color;
                    
                    uint8_t type = m_fluidBox->m_horizontalVelocityType[index];

                    if (type == FluidBox::Velocity_FREE)
                    {
                        continue;
                    }
                    else if (type == FluidBox::Velocity_ZERO)
                    {
                        color = Color(1.f, 0.f, 0.f, 1.0);
                    }
                    else if (type == FluidBox::Velocity_LOOPING_LEFT)
                    {
                        color = Color(0.f, 1.f, 0.f, 1.0);
                    }
                    else if (type == FluidBox::Velocity_LOOPING_RIGHT)
                    {
                        color = Color(1.f, 0.f, 1.f, 1.0);
                    }

                    draw_line(drawPosition + Vector2(-1, 0), drawPosition + Vector2(1, 0), color, 1.f);
                }
            }

            for (int j = 0; j < m_fluidBox->m_verticalVelocityCountY; j++)
            {
                for (int i = 0; i < m_fluidBox->m_verticalVelocityCountX; i++)
                {
                    Vector2 worldPosition(blockSize * 0.5f + blockSize * i, blockSize * j);
                    Vector2 drawPosition = WorldPositionToDraw(worldPosition);
                    int index = i + j * m_fluidBox->m_verticalVelocityCountX;
                    Color color;

                    uint8_t type = m_fluidBox->m_verticalVelocityType[index];

                    if (type == FluidBox::Velocity_FREE)
                    {
                        continue;
                    }
                    else if (type == FluidBox::Velocity_ZERO)
                    {
                        color = Color(1.f, 0.f, 0.f, 1.0);
                    }
                    else if (type == FluidBox::Velocity_LOOPING_LEFT)
                    {
                        color = Color(0.f, 1.f, 0.f, 1.0);
                    }
                    else if (type == FluidBox::Velocity_LOOPING_RIGHT)
                    {
                        color = Color(1.f, 0.f, 1.f, 1.0);
                    }

                    draw_line(drawPosition + Vector2(-1, 0), drawPosition + Vector2(1, 0), color, 1.f);
                }
            }
        }

        if (true)
        {
            float* horizontalVelocity = m_fluidBox->m_horizontalVelocityBuffer[m_fluidBox->m_activeVelocityBufferIndex];
            float* verticalVelocity = m_fluidBox->m_verticalVelocityBuffer[m_fluidBox->m_activeVelocityBufferIndex];
            int horizontalVelocityCountX = m_fluidBox->m_horizontalVelocityCountX;
            int verticalVelocityCountX = m_fluidBox->m_verticalVelocityCountX;

            auto LeftHorizontalVelocity = [horizontalVelocityCountX, horizontalVelocity](int blockX, int blockY) -> float {
                int velocityIndex = blockX + horizontalVelocityCountX * blockY;
                return horizontalVelocity[velocityIndex];
            };

            auto RightHorizontalVelocity = [horizontalVelocityCountX, horizontalVelocity](int blockX, int blockY) -> float {
                int velocityIndex = blockX + horizontalVelocityCountX * blockY + 1;
                return horizontalVelocity[velocityIndex];
            };

            auto RightHorizontalVelocityLooping = [horizontalVelocityCountX, horizontalVelocity](int blockX, int blockY) -> float {
                int velocityIndex = blockX + horizontalVelocityCountX * (blockY - 1) + 1;
                return horizontalVelocity[velocityIndex];
            };

            auto TopVerticalVelocity = [verticalVelocityCountX, verticalVelocity](int blockX, int blockY) -> float {
                int velocityIndex = blockX + verticalVelocityCountX * blockY;
                return verticalVelocity[velocityIndex];
            };

            auto BottomVerticalVelocity = [verticalVelocityCountX, verticalVelocity](int blockX, int blockY) -> float {
                int velocityIndex = blockX + verticalVelocityCountX * (blockY + 1);
                return verticalVelocity[velocityIndex];
            };

            // Velocity
            for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
            {
                for (int i = 0; i < m_fluidBox->m_blockCountX; i++)
                {
                    int index = m_fluidBox->BlockIndex(i, j);

                    uint8_t type = m_fluidBox->m_blockEdgeType[index];
                    if (type == FluidBox::BlockEdge_FILL)
                    {
                        continue;
                    }

                    Vector2 position((real_t)i* blockSize, (real_t)j* blockSize);
                    Vector2 center_pos = position + Vector2(blockSize, blockSize) * 0.5f;


                    float leftVelocity = LeftHorizontalVelocity(i, j);
                    float rightVelocity;
                    if (i == m_fluidBox->m_blockCountX - 1 && m_fluidBox->m_isHorizontalLoop)
                    {
                        rightVelocity  = RightHorizontalVelocityLooping(i, j);
                    }
                    else
                    {
                        rightVelocity = RightHorizontalVelocity(i, j);
                    }

                    float topVelocity = TopVerticalVelocity(i, j);
                    float bottomVelocity = BottomVerticalVelocity(i, j);
                    
                    Vector2 velocity(0.5f * (leftVelocity + rightVelocity), 0.5f * (topVelocity + bottomVelocity));


                    //if (velocity.length_squared() > 100)
                    //{
                    //    int plop = 1;
                    //    for (int x = 0; x < 50; x++)
                    //    {
                    //        printf("%d: %10.15g -> %10.15g \n", x, RightHorizontalVelocity(i + x, j), RightHorizontalVelocity(i + x, j + 1));
                    //    }


                    //}

                    if (velocity.length_squared() < 1e-5)
                    {
                        continue;
                    }

                    float velocityAngle = velocity.angle();

                    float red = 0.5f * (1 + sinf(velocityAngle));
                    float green = 0.5f * (1 + sinf(velocityAngle + 2 * float(Math_PI) / 3.0f)); //  + 60°
                    float blue = 0.5f * (1 + sinf(velocityAngle + 4 * float(Math_PI) / 3.0f)); //  + 120°


                    Color color(red, green, blue);

                    Vector2 velocity_pos = center_pos + velocity * RENDER_DEBUG_VELOCITY_SCALE;

                    if (WorldSizeToDraw(velocity * RENDER_DEBUG_VELOCITY_SCALE).length_squared() > 1.f)
                    {
                        DrawLine(center_pos, velocity_pos, color, 1);
                    }
                }
            }
        }

    }

    //    for (int j = 0; j < m_fluidBox->m_blockCountY; j += 2)
    //    {
    //        for (int i = 0; i < m_fluidBox->m_blockCountX; i += 2)
    //        {
    //            //Color tileColor(0.f, m_fluidBox->density[Index(i, j)] / 100.f, 0.f);
    //            //Color tileColor(m_fluidBox->Vx[Index(i, j)] / 10.f, m_fluidBox->density[Index(i, j)] / 100.f, m_fluidBox->Vy[Index(i, j)] / 10.f);
    //            //

    //            int index = m_fluidBox->Index(i, j);

    //            Vector2 velocity(m_fluidBox->Vx[index], m_fluidBox->Vy[index]);

    //            if (velocity.length_squared() < 1e-5)
    //            {
    //                continue;
    //            }

    //            float velocityAngle = velocity.angle();

    //            float red = 0.5f * (1 + sinf(velocityAngle));
    //            float green = 0.5f * (1 + sinf(velocityAngle + 2 * float(Math_PI) / 3.0f)); //  + 60°
    //            float blue = 0.5f * (1 + sinf(velocityAngle + 4 * float(Math_PI) / 3.0f)); //  + 120°


    //            Color color(red, green, blue);





    //            Vector2 position((real_t)i, (real_t)j);
    //            Vector2 center_pos = position + Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * 0.5f;
    //            Vector2 velocity_pos = center_pos + velocity * RENDER_DEBUG_VELOCITY_SCALE;

    //     



    //            //draw_rect(Rect2(RENDER_DEBUG_OFFSET.x + i * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET.y + j * RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE), velocityColor, true);

    //            draw_line(RENDER_DEBUG_OFFSET + center_pos * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET + velocity_pos * RENDER_DEBUG_SCALE, color, 1);


    //        }
    //    }

    //    draw_rect(Rect2(RENDER_DEBUG_OFFSET.x + RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET.y + RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE * (m_fluidBox->m_blockCountX - 2), RENDER_DEBUG_SCALE * (m_fluidBox->m_blockCountY - 2)), Color(1.f, 0.f, 0.f), false);
    //    draw_rect(Rect2(RENDER_DEBUG_OFFSET.x, RENDER_DEBUG_OFFSET.y, RENDER_DEBUG_SCALE * m_fluidBox->m_blockCountX, RENDER_DEBUG_SCALE * m_fluidBox->m_blockCountY), Color(1.f, 1.f, 1.f), false);
    //}
    ////float offetX = RENDER_DEBUG_SCALE * m_fluidBox->m_blockCountX + 1;
    //float offetX = 0;
    //float offetY = RENDER_DEBUG_SCALE * m_fluidBox->m_blockCountY + 1;
    ////float offetY = 0;


    //float opacityCoeff = RENDER_DEBUG_DENSITY;


    //for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
    //{
    //    for (int i = 0; i < m_fluidBox->m_blockCountX; i++)
    //    {
    //        Vector2 position((real_t)i, (real_t)j);
    //        int index = m_fluidBox->Index(i, j);

    //        uint8_t type = m_fluidBox->blockEdgeType[index];
    //        if (type != 0)
    //        {
    //            if (type == 1)
    //            {
    //                draw_line(RENDER_DEBUG_OFFSET + position * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET + (position + Vector2(0, WORLD_BLOCK_SIZE)) * RENDER_DEBUG_SCALE, Color(1.f, 1.f, 1.0f), 1);
    //            }
    //            else if (type == 2)
    //            {
    //                draw_line(RENDER_DEBUG_OFFSET + (position + Vector2(WORLD_BLOCK_SIZE, 0)) * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET + (position + Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE)) * RENDER_DEBUG_SCALE, Color(1.f, 1.f, 1.0f), 1);
    //            }
    //        }
    //    }
    //}

    //for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
    //{
    //    for (int i = 0; i < m_fluidBox->m_blockCountX; i++)
    //    {
    //        //Color tileColor(0.f, m_fluidBox->density[Index(i, j)] / 100.f, 0.f);
    //        //float value = m_fluidBox->density[Index(i, j)] / 100.f;

    //        Vector3 composition(m_fluidBox->density[0][m_fluidBox->Index(i, j)], m_fluidBox->density[1][m_fluidBox->Index(i, j)], m_fluidBox->density[2][m_fluidBox->Index(i, j)]);

    //        float density = composition.x + composition.y + composition.z;

    //        Vector3 dyeColor = composition.normalized();


    //        Color tileColor(dyeColor.x, dyeColor.y, dyeColor.z, density * opacityCoeff);

    //        draw_rect(Rect2(RENDER_DEBUG_OFFSET.x + offetX + i * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET.y+ offetY + j * RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE), tileColor, true);
    //    }
    //}

    //draw_rect(Rect2(RENDER_DEBUG_OFFSET.x +offetX + RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET.y + offetY  + RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE * (m_fluidBox->m_blockCountX - 2), RENDER_DEBUG_SCALE * (m_fluidBox->m_blockCountY - 2)), Color(1.f, 0.f, 0.f), false);
    //draw_rect(Rect2(RENDER_DEBUG_OFFSET.x + offetX, RENDER_DEBUG_OFFSET.y + offetY, RENDER_DEBUG_SCALE * m_fluidBox->m_blockCountX, RENDER_DEBUG_SCALE * m_fluidBox->m_blockCountY), Color(1.f, 1.f, 1.f), false);

}

}