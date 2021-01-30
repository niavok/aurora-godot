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
    register_property<Fluids, float>("m_worldStepDt", &Fluids::m_worldStepDt, 0.01);
    register_property<Fluids, float>("RENDER_DEBUG_DENSITY", &Fluids::RENDER_DEBUG_DENSITY, 0.01);
}


#define NIX 200 // TODO FIX

#define IX(x, y) ((x) + (y) * NIX)

FluidBox* FluidBoxCreate(size_t size)
{
    FluidBox* box = new FluidBox();
    size_t N = size;
    size_t N2 = size * size;

    box->size = (int) size;
    for (int i = 0; i < 3; i++)
    {
        box->s[i] = new float[N2];
        box->density[i] = new float[N2];
    }

    box->Vx = new float[N2];
    box->Vy = new float[N2];

    box->Vx0 = new float[N2];
    box->Vy0 = new float[N2];


    for (int i = 0; i < N2; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            box->s[j][i] = 0.f;
            box->density[j][i] = 0.f;
        }

        box->Vx[i] = 0.f;
        box->Vy[i] = 0.f;

        box->Vx0[i] = 0.f;
        box->Vy0[i] = 0.f;
    }


    return box;
}

void FluidBoxFree(FluidBox* box)
{
    for (int i = 0; i < 3; i++)
    {
        delete[] box->s[i];
        delete[] box->density[i];
    }

    delete[] box->Vx;
    delete[] box->Vy;
    
    delete[] box->Vx0;
    delete[] box->Vy0;
    
    delete box;
}

void FluidBoxAddDensity(FluidBox* box, int x, int y, float amount, int color)
{
    int N = box->size;
    
    box->density[color][IX(x, y)] += amount;
    box->s[color][IX(x, y)] += amount;
}

void FluidBoxAddVelocity(FluidBox* box, int x, int y, float amountX, float amountY)
{
    int N = box->size;
    int index = IX(x, y);

    box->Vx[index] += amountX;
    box->Vy[index] += amountY;
    box->Vx0[index] += amountX;
    box->Vy0[index] += amountY;
}

static void set_bnd(int b, float* x, int N)
{
    for (int i = 1; i < N - 1; i++) {
        x[IX(i, 0)] = b == 2 ? -x[IX(i, 1)] : x[IX(i, 1)];
        x[IX(i, N - 1)] = b == 2 ? -x[IX(i, N - 2)] : x[IX(i, N - 2)];
    }
    
    for (int j = 1; j < N - 1; j++) {
        x[IX(0, j)] = b == 1 ? -x[IX(1, j)] : x[IX(1, j)];
        x[IX(N - 1, j)] = b == 1 ? -x[IX(N - 2, j)] : x[IX(N - 2, j)];
    }
   

    x[IX(0, 0)]     = 0.5f * (x[IX(1, 0)] + x[IX(0, 1)]);
    x[IX(0, N-1)]   = 0.5f * (x[IX(1, N - 1)] + x[IX(0, N - 2)]);
    x[IX(N-1, 0)]   = 0.5f * (x[IX(N - 2, 0)] + x[IX(N - 1, 1)]);
    x[IX(N-1, N-1)] = 0.5f * (x[IX(N - 2, N - 1)] + x[IX(N - 1, N - 2)]);
}

static void lin_solve(int b, float* x, float* x0, float a, float c, int iter, int N)
{
    float cRecip = 1.0f / c;
    for (int k = 0; k < iter; k++) {
        for (int j = 1; j < N - 1; j++) {
            for (int i = 1; i < N - 1; i++) {
                x[IX(i, j)] =
                    (x0[IX(i, j)] 
                        + a * (x[IX(i + 1, j)] + x[IX(i - 1, j)] + x[IX(i, j + 1)] + x[IX(i, j - 1)])
                    ) * cRecip;
            }
        }

        set_bnd(b, x, N);
    }
}

static void diffuse(int b, float* x, float* x0, float diff, float dt, int iter, int N)
{
    float a = dt * diff * (N - 2) * (N - 2);
    lin_solve(b, x, x0, a, 1 + 4 * a, iter, N);
}

static void project(float* velocX, float* velocY, float* p, float* div, int iter, int N)
{
    float h = 1.f / N;
    for (int j = 1; j < N - 1; j++) {
        for (int i = 1; i < N - 1; i++) {
            div[IX(i, j)] = -0.5f * h * (
                velocX[IX(i + 1, j)]
                - velocX[IX(i - 1, j)]
                + velocY[IX(i, j + 1)]
                - velocY[IX(i, j - 1)]
                );
            p[IX(i, j)] = 0;
        }
    }
 
    set_bnd(0, div, N);
    set_bnd(0, p, N);
    lin_solve(0, p, div, 1, 4, iter, N);

    for (int j = 1; j < N - 1; j++) {
        for (int i = 1; i < N - 1; i++) {
            velocX[IX(i, j)] -= 0.5f * (p[IX(i + 1, j)] - p[IX(i - 1, j)]) * N;
            velocY[IX(i, j)] -= 0.5f * (p[IX(i, j + 1)] - p[IX(i, j - 1)]) * N;
        }
    }
    
    set_bnd(1, velocX, N);
    set_bnd(2, velocY, N);
}

static void advect(int b, float* d, float* d0, float* velocX, float* velocY, float dt, int N)
{
    float i0, i1, j0, j1;

    float dtx = dt * (N - 2);
    float dty = dt * (N - 2);

    float s0, s1, t0, t1;
    float tmp1, tmp2, x, y;

    float Nfloat = float(N);
    float ifloat, jfloat;
    int i, j;

    for (j = 1, jfloat = 1; j < N - 1; j++, jfloat++) {
        for (i = 1, ifloat = 1; i < N - 1; i++, ifloat++) {
            tmp1 = dtx * velocX[IX(i, j)];
            tmp2 = dty * velocY[IX(i, j)];
            x = ifloat - tmp1;
            y = jfloat - tmp2;

            if (x < 0.5f) x = 0.5f;
            if (x > Nfloat + 0.5f) x = Nfloat + 0.5f;
            i0 = floorf(x);
            i1 = i0 + 1.0f;
            if (y < 0.5f) y = 0.5f;
            if (y > Nfloat + 0.5f) y = Nfloat + 0.5f;
            j0 = floorf(y);
            j1 = j0 + 1.0f;

            s1 = x - i0;
            s0 = 1.0f - s1;
            t1 = y - j0;
            t0 = 1.0f - t1;

            int i0i = (int) i0;
            int i1i = (int) i1;
            int j0i = (int) j0;
            int j1i = (int) j1;

            d[IX(i, j)] =
                  s0 * (t0 * d0[IX(i0i, j0i)] + t1 * d0[IX(i0i, j1i)])
                + s1 * (t0 * d0[IX(i1i, j0i)] + t1 * d0[IX(i1i, j1i)]);
        }
    }
    set_bnd(b, d, N);
}

void FluidBoxStep(FluidBox* box, float dt, float diff, float visc)
{
    int N = box->size;
    float* Vx = box->Vx;
    float* Vy = box->Vy;
    float* Vx0 = box->Vx0;
    float* Vy0 = box->Vy0;
    

    diffuse(1, Vx0, Vx, visc, dt, 20, N);
    diffuse(2, Vy0, Vy, visc, dt, 20, N);
    
    project(Vx0, Vy0, Vx, Vy, 20, N);

    advect(1, Vx, Vx0, Vx0, Vy0, dt, N);
    advect(2, Vy, Vy0, Vx0, Vy0, dt, N);

    project(Vx, Vy, Vx0, Vy0, 20, N);

    for (int i = 0; i < 3; i++)
    {
        float* s = box->s[i];
        float* density = box->density[i];
        diffuse(0, s, density, diff, dt, 20, N);
        advect(0, density, s, Vx, Vy, dt, N);
    }
}



Fluids::Fluids()
    : m_size(200)
{
    m_fluidBox = FluidBoxCreate(m_size);


    //for (int j = 0; j < 20; j++)
    //{
    //    for (int i = 0; i < 10; i++)
    //    {
    //        FluidBoxAddDensity(m_fluidBox, 90+i, 10+j, 100.f);
    //        FluidBoxAddVelocity(m_fluidBox, 100+i, 100+j, 10.f, 10.f);
    //    }
    //}

   /* for (int j = 0; j < m_size; j++)
    {
        for (int i = 0; i < m_size; i++)
        {
            FluidBoxAddDensity(m_fluidBox, i, j, 50.f);
            FluidBoxAddDensity(m_fluidBox, i, j, (100.f * i) / m_size);
            FluidBoxAddVelocity(m_fluidBox, i, j, ((float)i) / m_size, ((float) j) / m_size);
        }
    }*/
}

Fluids::~Fluids() {
    FluidBoxFree(m_fluidBox);
    m_fluidBox = nullptr;

    
}

void Fluids::_init()
{
    Godot::print("Fluids::_init");
    diffusion_coef = 0.1;
    viscosity_coef = 0.1;
    RENDER_DEBUG_OFFSET = Vector2();
    RENDER_DEBUG_SCALE = 4;
    WORLD_BLOCK_SIZE = 1;
    RENDER_DEBUG_VELOCITY_SCALE = 1;
    m_worldStepDt = 0.001f;
    RENDER_DEBUG_DENSITY = 0.01;
}

void Fluids::_process(float delta)
{
    Input* input = Input::get_singleton();


    if (input->is_action_just_pressed("world_debug_toogle_sun"))
    {
        m_worldEnableSun = !m_worldEnableSun;
        Godot::print("world_enable_sun=", String(m_worldEnableSun));
    }

    if (input->is_action_just_pressed("world_debug_increase_simulation_speed"))
    {
        m_worldStepDt *= 1.2f;
        Godot::print("m_worldStepDt=", String(m_worldStepDt));
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


    for (int j = 0; j < 20; j++)
    {
        for (int i = 0; i < 10; i++)
        {
            FluidBoxAddDensity(m_fluidBox, 50 + i, 100 + j, 200.0f * dt, 0);

            Vector2 direction(1, yO / 20.f);
            

            float vel = 20000.f * dt;
            FluidBoxAddVelocity(m_fluidBox, 50 + i, 100 + j, direction.x * vel, direction.y * vel);


     
        }
    }

    for (int j = 0; j < 8; j++)
    {
        for (int i = 0; i < 8; i++)
        {

            FluidBoxAddDensity(m_fluidBox, 150 + i, 80 + j, 200.0f * dt, 1);

            Vector2 directionB(-1, yB / 10);
            //Vector2 directionB;
            //directionB.set_rotation(totalDuration * 5);
            float velB = 10000.f * dt;

            FluidBoxAddVelocity(m_fluidBox, 150 + i, 80 + j, directionB.x * velB, directionB.y * velB);

        }
    }

    for (int j = 0; j < 8; j++)
    {
        for (int i = 0; i < 8; i++)
        {

            FluidBoxAddDensity(m_fluidBox, 100 + i, 150 + j, 200.0f * dt, 2);

        }
    }

    FluidBoxStep(m_fluidBox, dt * 0.001, diffusion_coef * 0.00001, viscosity_coef * 0.00001);
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
    
    for (int j = 0; j < m_size; j+=3)
    {
        for (int i = 0; i < m_size; i+= 3)
        {
            //Color tileColor(0.f, m_fluidBox->density[IX(i, j)] / 100.f, 0.f);
            //Color tileColor(m_fluidBox->Vx[IX(i, j)] / 10.f, m_fluidBox->density[IX(i, j)] / 100.f, m_fluidBox->Vy[IX(i, j)] / 10.f);
            //
            Vector2 velocity(m_fluidBox->Vx[IX(i, j)], m_fluidBox->Vy[IX(i, j)]);

            float velocityAngle = velocity.angle();

            float red = 0.5f * (1 + sinf(velocityAngle));
            float green = 0.5f * (1 + sinf(velocityAngle + 2 * float(Math_PI) / 3.0f)); //  + 60°
            float blue = 0.5f * (1 + sinf(velocityAngle + 4 * float(Math_PI) / 3.0f)); //  + 120°

            Color velocityColor(red, green, blue, velocity.length() / 10.f);


            Color color(red, green, blue);

          



            Vector2 position((real_t) i, (real_t) j);
            Vector2 center_pos = position + Vector2(WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE) * 0.5f;
            Vector2 velocity_pos = center_pos + velocity * RENDER_DEBUG_VELOCITY_SCALE;



            


            //draw_rect(Rect2(RENDER_DEBUG_OFFSET.x + i * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET.y + j * RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE), velocityColor, true);
            
            draw_line(RENDER_DEBUG_OFFSET + center_pos * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET + velocity_pos * RENDER_DEBUG_SCALE, color, 1);


        }
    }

    draw_rect(Rect2(RENDER_DEBUG_OFFSET.x + RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET.y + RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE * (m_size - 2), RENDER_DEBUG_SCALE * (m_size - 2)), Color(1.f, 0.f, 0.f), false);
    draw_rect(Rect2(RENDER_DEBUG_OFFSET.x, RENDER_DEBUG_OFFSET.y, RENDER_DEBUG_SCALE * m_size, RENDER_DEBUG_SCALE * m_size), Color(1.f, 1.f, 1.f), false);

    float offetX = RENDER_DEBUG_SCALE * m_size + 1;


    float opacityCoeff = RENDER_DEBUG_DENSITY / (30 + totalDuration);

    for (int j = 0; j < m_size; j++)
    {
        for (int i = 0; i < m_size; i++)
        {
            //Color tileColor(0.f, m_fluidBox->density[IX(i, j)] / 100.f, 0.f);
            //float value = m_fluidBox->density[IX(i, j)] / 100.f;

            Vector3 composition(m_fluidBox->density[0][IX(i, j)], m_fluidBox->density[1][IX(i, j)], m_fluidBox->density[2][IX(i, j)]);

            float density = composition.x + composition.y + composition.z;

            Vector3 dyeColor = composition.normalized();


            Color tileColor(dyeColor.x, dyeColor.y, dyeColor.z, density * opacityCoeff);

            draw_rect(Rect2(RENDER_DEBUG_OFFSET.x + offetX + i * RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET.y + j * RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE), tileColor, true);
        }
    }

    draw_rect(Rect2(RENDER_DEBUG_OFFSET.x +offetX + RENDER_DEBUG_SCALE, RENDER_DEBUG_OFFSET.y + RENDER_DEBUG_SCALE, RENDER_DEBUG_SCALE * (m_size - 2), RENDER_DEBUG_SCALE * (m_size - 2)), Color(1.f, 0.f, 0.f), false);
    draw_rect(Rect2(RENDER_DEBUG_OFFSET.x + offetX, RENDER_DEBUG_OFFSET.y, RENDER_DEBUG_SCALE * m_size, RENDER_DEBUG_SCALE * m_size), Color(1.f, 1.f, 1.f), false);

}

}