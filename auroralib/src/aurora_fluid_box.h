#ifndef AURORA_FLUID_BOX_H
#define AURORA_FLUID_BOX_H

namespace godot {


class FluidBox {
public:
    FluidBox(int size);
    ~FluidBox();

    void AddDensity(int x, int y, float amount, int color);
    void AddVelocity(int x, int y, float amountX, float amountY);

    void Step(float dt, float diff, float visc);

    int Index(int x, int y);

    float* density[3];

    float* Vx;
    float* Vy;

private:
    

    void SetBound(int b, float* x);
    void LinearSolve(int b, float* x, float* x0, float a, float c);

    void Diffuse(int b, float* x, float* x0, float diff, float dt);
    void Project(float* velocX, float* velocY, float* p, float* div);
    void Advect(int b, float* d, float* d0, float* velocX, float* velocY, float dt);

    int m_size;
    int m_linearSolveIter;
    float* s[3];
  

    float* Vx0;
    float* Vy0;
};



}

#endif