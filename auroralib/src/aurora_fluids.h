#ifndef AURORA_FLUIDS_H
#define AURORA_FLUIDS_H

#include <Godot.hpp>
#include <Sprite.hpp>

#include "aurora_fluid_box.h"

namespace godot {

    class Fluids : public Sprite {
    GODOT_CLASS(Fluids, Sprite)

public:
    static void _register_methods();

    Fluids();
    ~Fluids();

    void _init(); // our initializer called by Godot

    void _process(float delta);

    void _draw();
private:
    void StepWorld(float dt);
    void PrintWorldState();
    void FillTile(bool fill, Vector2 mousePosition);

    
    FluidBox* m_fluidBox;
    bool m_worldEnableSun = true;
    bool m_paused = true;
    
    float totalDuration = 0;


    float m_worldStepDt;
    float diffusion_coef;
    float viscosity_coef;
    Vector2 RENDER_DEBUG_OFFSET;
    float RENDER_DEBUG_SCALE;
    float WORLD_BLOCK_SIZE;
    float RENDER_DEBUG_VELOCITY_SCALE;
    float RENDER_DEBUG_DENSITY;
};



}

#endif