#ifndef AURORA_FLUID_BOX_H
#define AURORA_FLUID_BOX_H

#include <stdint.h>

namespace godot {


class FluidBox {
public:
    FluidBox(int blockCountX, int blockCountY, float blockSize,  bool isHorizontalLoop);
    ~FluidBox();

    void AddDensity(int x, int y, float amount, int color);
    void AddVelocity(int x, int y, float amountX, float amountY);
    void SetVelocity(int x, int y, float amountX, float amountY);
    void DecayDensity(float keepRatio);

    void Step(float dt, float diff, float visc);

    int Index(int x, int y);
    int IndexLoop(int x, int y);

    enum BlockEdgeType
    {
        BlockEdge_VOID,
        BlockEdge_FILL,
        BlockEdge_TOP_EDGE,
        BlockEdge_BOTTOM_EDGE,
        BlockEdge_LEFT_EDGE,
        BlockEdge_RIGHT_EDGE,
        BlockEdge_TOP_LEFT_CORNER,
        BlockEdge_TOP_RIGHT_CORNER,
        BlockEdge_BOTTOM_LEFT_CORNER,
        BlockEdge_BOTTOM_RIGHT_CORNER,
        BlockEdge_HORIZONTAL_PIPE,
        BlockEdge_LOOPING_LEFT_EDGE,
        BlockEdge_LOOPING_RIGHT_EDGE,
        BlockEdge_LOOPING_TOP_LEFT_CORNER,
        BlockEdge_LOOPING_TOP_RIGHT_CORNER,
        BlockEdge_LOOPING_BOTTOM_LEFT_CORNER,
        BlockEdge_LOOPING_BOTTOM_RIGHT_CORNER,
        BlockEdge_LOOPING_LEFT_HORIZONTAL_PIPE,
        BlockEdge_LOOPING_RIGHT_HORIZONTAL_PIPE,
    };

    enum VelocityType
    {
        Velocity_FREE,
        Velocity_ZERO,
        Velocity_LOOPING_LEFT,
        Velocity_LOOPING_RIGHT,
    };

    uint8_t* blockEdgeType;
    uint8_t* velocityXType;
    uint8_t* velocityYType;

    float* density[3];

    float* Vx;
    float* Vy;

    int m_blockCountX;
    int m_blockCountY;
    float m_blockSize;

private:
    
    void SetBound(int b, float* x);
    int LinearSolve(int b, float* x, float* x0, float a, float c, int maxIter, float qualityThresold);

    void Diffuse(int b, float* x, float* x0, float diff, float dt, int maxIter, float qualityThresold);
    void Project(float* velocX, float* velocY, float* p, float* div);
    void Advect(int b, float* d, float* d0, float* velocX, float* velocY, float dt);

    int m_diffuseMaxIter;
    int m_viscosityMaxIter;
    int m_projectMaxIter;
    float m_diffuseQualityThresold;
    float m_viscosityQualityThresold;
    float m_projectQualityThresold;


    bool m_isHorizontalLoop;
    float* tempDensity[3];
    float* p1;
    float* p2;
    float* divBuffer;

    int m_blockCountXMask;
    int m_blockCount;
    int m_blockCountWithBounds;
  
    int m_blockCountXWithBounds;
    int m_blockCountYWithBounds;

    float* Vx0;
    float* Vy0;
};



}

#endif