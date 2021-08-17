#include "aurora_world_renderer.h"

#include <Input.hpp>
#include <Vector2.hpp>
#include <Vector3.hpp>
#include <Viewport.hpp>

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
    m_world = new aurora::AuroraWorld();
}

void WorldRenderer::FillTile(bool fill, Vector2 mousePosition)
{

    Vector2 worldPosition = (mousePosition - RENDER_DEBUG_OFFSET) / RENDER_DEBUG_SCALE;

    float blockSize = m_fluidBox->m_blockSize;

    int i = int(worldPosition.x / blockSize);
    int j = int(worldPosition.y / blockSize) % m_fluidBox->m_blockCountY;

    if (i < 0 || j < 0 || i >= m_fluidBox->m_blockCountX || j >= m_fluidBox->m_blockCountY)
    {
        return;
    }

    m_fluidBox->m_blockConfig[m_fluidBox->BlockIndex(i, j)].isFill = fill;
    m_fluidBox->CompileGrid();
    m_fluidBox->SetContent(i, j, 0.f, 0.f, 0.f);
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


void Fluids::StepWorld(float dt)
{

    totalDuration += dt;
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

    if (m_worldEnableSun)
    {
        for (int j = 0; j < 100; j++)
        //for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
        {
            //m_fluidBox->AddHorizontalVelocityAtLeft(50, j, 1000.f);
            for (int i = 0; i < 50; i++)
            {
                m_fluidBox->SetHorizontalVelocityAtLeft(5+i, j, 200.f);
                m_fluidBox->SetVerticalVelocityAtBottom(5 + i, j, 0.f);
            }

            for (int xOffset = 0; xOffset < 5; xOffset++)
            {
                m_fluidBox->SetContent(xOffset, j, 0.f, 0.f, 0.f);
            }
        }

        //for (int j = 20; j < 35; j++)
        //    //for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
        //{
        //    m_fluidBox->AddHorizontalVelocityAtLeft(230, j, -500.f);
        //}

        for (int j = 0; j < 20; j++)
        {
            for (int i = 0; i < 10; i++)
            {
                //m_fluidBox->AddDensity(50 + i, 50 + j, 5.0f * dt, 0);
                
                Vector2 direction(1, yO / 20.f);


                float vel = 200.f * dt;
                //m_fluidBox->AddVelocity(50 + i, 50 + j, direction.x * vel, direction.y * vel);



            }
        }

        if ((stepCount / 2) % 40 == 0)
        {
            int kStep = 10;
            for (int k = 0; k < m_fluidBox->m_blockCountY - kStep / 2 - 4; k += kStep)
            {

                for (int j = 0; j < 4; j++)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        int color = (k / kStep) % 3;

                        m_fluidBox->AddDensity(10 + i, kStep / 2 + k + j, 2000.0f * dt, color);
                    }
                }
            }
        }


        if (false)
        {
            for (int j = 0; j < 8; j++)
            {
                for (int i = 0; i < 8; i++)
                {

                    m_fluidBox->AddDensity(170 + i, 40 + j, 5.0f * dt, 1);

                    Vector2 directionB(-1, yB / 10);
                    //Vector2 directionB;
                    //directionB.set_rotation(totalDuration * 5);
                    float velB = 100.f * dt;

                    //m_fluidBox->AddVelocity(150 + i, 40 + j, directionB.x * velB, directionB.y * velB);

                }
            }

            for (int j = 0; j < 20; j++)
            {
                for (int i = 0; i < 8; i++)
                {

                    m_fluidBox->AddDensity(120 + i, 80 + j, 5.0f * dt, 2);

                }
            }
        }
    }

    m_fluidBox->DecayDensity(0.999f);


    m_fluidBox->Step(dt, diffusion_coef /** 0.00001f*/, viscosity_coef /** 0.00001f*/);
}

void Fluids::PrintWorldState()
{
    Godot::print("Step");
}

void Fluids::_draw()
{
    Vector2 drawPosition = RENDER_DEBUG_OFFSET;

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
        return drawPosition + worldPosition * RENDER_DEBUG_SCALE;
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


    auto DrawBorder = [&]()
    {
        // First fill only
        for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
        {
            for (int i = 0; i < m_fluidBox->m_blockCountX; i++)
            {
                Vector2 position((real_t)i * blockSize, (real_t)j * blockSize);
                int index = m_fluidBox->BlockIndex(i, j);

                if(m_fluidBox->m_blockConfig[index].isFill)
                {
                    DrawRect(position, Vector2(blockSize, blockSize), Color(0.5f, 0.5f, 0.5f), true);
                }
            }
        }

        for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
        {
            for (int i = 0; i < m_fluidBox->m_blockCountX; i++)
            {
                Vector2 position((real_t)i * blockSize, (real_t)j * blockSize);
                int index = m_fluidBox->BlockIndex(i, j);

                FluidBox::BlockConfig& blockConfig = m_fluidBox->m_blockConfig[index];

                if (!blockConfig.isTopConnected)
                {
                    DrawLine(position, position + Vector2(blockSize, 0), Color(1.0f, 1.f, 1.0f), true);
                }

                if (!blockConfig.isBottomConnected)
                {
                    DrawLine(position + Vector2(0, blockSize), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
                }

                if (!blockConfig.isLeftConnected)
                {
                    DrawLine(position, position + Vector2(0, blockSize), Color(1.0f, 1.f, 1.0f), true);
                }

                if (!blockConfig.isRightConnected)
                {
                    DrawLine(position + Vector2(blockSize, 0), position + Vector2(blockSize, blockSize), Color(1.0f, 1.f, 1.0f), true);
                }

                /*uint8_t type = m_fluidBox->m_blockEdgeType[index];
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
                }*/
            }
        }

        if (false)
        {
            // Velocity type check
            for (int j = 0; j < m_fluidBox->m_horizontalVelocityCountY; j++)
            {
                for (int i = 0; i < m_fluidBox->m_horizontalVelocityCountX; i++)
                {
                    Vector2 worldPosition(blockSize * i, blockSize * 0.5f + blockSize * j);
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
    };

    if (true)
    {
        DrawBorder();
       

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
            int vStep = 2;
            for (int j = 0; j < m_fluidBox->m_blockCountY; j+= vStep)
            {
                for (int i = 0; i < m_fluidBox->m_blockCountX; i+=vStep)
                {
                    int index = m_fluidBox->BlockIndex(i, j);

                    if (m_fluidBox->m_blockConfig[index].isFill)
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

        drawPosition += WorldSizeToDraw(Vector2(0, m_fluidBox->m_blockCountY * m_fluidBox->m_blockSize));

        DrawBorder();

        // Content
        FluidBox::Content* content = m_fluidBox->m_contentBuffer[m_fluidBox->m_activeContentBufferIndex];
        float opacityCoeff = RENDER_DEBUG_DENSITY;

        for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
        {
            for (int i = 0; i < m_fluidBox->m_blockCountX; i++)
            {
                int index = m_fluidBox->BlockIndex(i, j);

                FluidBox::Content& blockContent = content[index];

                Vector3 composition(blockContent.totalContent.density0, blockContent.totalContent.density1, blockContent.totalContent.density2);

                float density = composition.x + composition.y + composition.z;

                Vector3 dyeColor = composition.normalized();


                Color tileColor(dyeColor.x, dyeColor.y, dyeColor.z, density * opacityCoeff);

                Vector2 position((real_t)i* blockSize, (real_t)j* blockSize);

                DrawRect(position, Vector2(blockSize, blockSize), tileColor, true);
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