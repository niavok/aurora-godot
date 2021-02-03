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
    m_fluidBox = new FluidBox(512, 100, 150, true); // 51.2 km * 15 km
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
        for (int j = 0; j < 20; j++)
        {
            for (int i = 0; i < 10; i++)
            {
                m_fluidBox->AddDensity(50 + i, 50 + j, 200.0f * dt, 0);
                
                Vector2 direction(1, yO / 20.f);


                float vel = 200.f * dt;
                m_fluidBox->AddVelocity(50 + i, 50 + j, direction.x * vel, direction.y * vel);



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

                m_fluidBox->AddVelocity(150 + i, 40 + j, directionB.x * velB, directionB.y * velB);

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
    if (true)
    {
        for (int j = 0; j < m_fluidBox->m_blockCountY; j += 2)
        {
            for (int i = 0; i < m_fluidBox->m_blockCountX; i += 2)
            {
                //Color tileColor(0.f, m_fluidBox->density[Index(i, j)] / 100.f, 0.f);
                //Color tileColor(m_fluidBox->Vx[Index(i, j)] / 10.f, m_fluidBox->density[Index(i, j)] / 100.f, m_fluidBox->Vy[Index(i, j)] / 10.f);
                //
                Vector2 velocity(m_fluidBox->Vx[m_fluidBox->Index(i, j)], m_fluidBox->Vy[m_fluidBox->Index(i, j)]);

                if (velocity.length_squared() < 1e-5)
                {
                    continue;
                }

                float velocityAngle = velocity.angle();

                float red = 0.5f * (1 + sinf(velocityAngle));
                float green = 0.5f * (1 + sinf(velocityAngle + 2 * float(Math_PI) / 3.0f)); //  + 60°
                float blue = 0.5f * (1 + sinf(velocityAngle + 4 * float(Math_PI) / 3.0f)); //  + 120°


                Color color(red, green, blue);





                Vector2 position((real_t)i, (real_t)j);
                Vector2 center_pos = position + Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * 0.5f;
                Vector2 velocity_pos = center_pos + velocity * RENDER_DEBUG_VELOCITY_SCALE;






                //draw_rect(Rect2(RENDER_DEBUG_OFFSET.x + i * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET.y + j * RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE), velocityColor, true);

                draw_line(RENDER_DEBUG_OFFSET + center_pos * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET + velocity_pos * RENDER_DEBUG_SCALE, color, 1);


            }
        }

        draw_rect(Rect2(RENDER_DEBUG_OFFSET.x + RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET.y + RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE * (m_fluidBox->m_blockCountX - 2), RENDER_DEBUG_SCALE * (m_fluidBox->m_blockCountY - 2)), Color(1.f, 0.f, 0.f), false);
        draw_rect(Rect2(RENDER_DEBUG_OFFSET.x, RENDER_DEBUG_OFFSET.y, RENDER_DEBUG_SCALE * m_fluidBox->m_blockCountX, RENDER_DEBUG_SCALE * m_fluidBox->m_blockCountY), Color(1.f, 1.f, 1.f), false);
    }
    //float offetX = RENDER_DEBUG_SCALE * m_fluidBox->m_blockCountX + 1;
    float offetX = 0;
    float offetY = RENDER_DEBUG_SCALE * m_fluidBox->m_blockCountY + 1;
    //float offetY = 0;


    float opacityCoeff = RENDER_DEBUG_DENSITY / (30 + totalDuration);

    for (int j = 0; j < m_fluidBox->m_blockCountY; j++)
    {
        for (int i = 0; i < m_fluidBox->m_blockCountX; i++)
        {
            //Color tileColor(0.f, m_fluidBox->density[Index(i, j)] / 100.f, 0.f);
            //float value = m_fluidBox->density[Index(i, j)] / 100.f;

            Vector3 composition(m_fluidBox->density[0][m_fluidBox->Index(i, j)], m_fluidBox->density[1][m_fluidBox->Index(i, j)], m_fluidBox->density[2][m_fluidBox->Index(i, j)]);

            float density = composition.x + composition.y + composition.z;

            Vector3 dyeColor = composition.normalized();


            Color tileColor(dyeColor.x, dyeColor.y, dyeColor.z, density * opacityCoeff);

            draw_rect(Rect2(RENDER_DEBUG_OFFSET.x + offetX + i * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET.y+ offetY + j * RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE), tileColor, true);
        }
    }

    draw_rect(Rect2(RENDER_DEBUG_OFFSET.x +offetX + RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET.y + offetY  + RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE * (m_fluidBox->m_blockCountX - 2), RENDER_DEBUG_SCALE * (m_fluidBox->m_blockCountY - 2)), Color(1.f, 0.f, 0.f), false);
    draw_rect(Rect2(RENDER_DEBUG_OFFSET.x + offetX, RENDER_DEBUG_OFFSET.y + offetY, RENDER_DEBUG_SCALE * m_fluidBox->m_blockCountX, RENDER_DEBUG_SCALE * m_fluidBox->m_blockCountY), Color(1.f, 1.f, 1.f), false);

}

}