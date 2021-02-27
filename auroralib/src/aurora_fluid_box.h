#ifndef AURORA_FLUID_BOX_H
#define AURORA_FLUID_BOX_H

#include <stdint.h>

namespace godot {


class FluidBox {
public:
    FluidBox(int blockCountX, int blockCountY, float blockSize,  bool isHorizontalLoop);
    ~FluidBox();

    void AddDensity(int x, int y, float amount, int color);
    //void AddVelocity(int x, int y, float amountX, float amountY);
    //void SetVelocity(int x, int y, float amountX, float amountY);
    void SetHorizontalVelocityAtLeft(int blockX, int blockY, float velocity);
    void DecayDensity(float keepRatio);

    void Step(float dt, float diff, float visc);

    int BlockIndex(int x, int y);
    int HorizontalVelocityIndex(int x, int y);
    int VerticalVelocityIndex(int x, int y);
    //int BlockIndexLoop(int x, int y);
    
    void CompileGrid();
    

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
        BlockEdge_LOOPING_LEFT_VOID,
        BlockEdge_LOOPING_LEFT_TOP_EDGE,
        BlockEdge_LOOPING_LEFT_BOTTOM_EDGE,
        BlockEdge_LOOPING_LEFT_HORIZONTAL_PIPE,
        BlockEdge_LOOPING_RIGHT_VOID,
        BlockEdge_LOOPING_RIGHT_TOP_EDGE,
        BlockEdge_LOOPING_RIGHT_BOTTOM_EDGE,    
        BlockEdge_LOOPING_RIGHT_HORIZONTAL_PIPE,
    };

    enum VelocityType
    {
        Velocity_FREE,
        Velocity_ZERO,
        Velocity_LOOPING_LEFT,
        Velocity_LOOPING_RIGHT,
    };

    uint8_t* m_blockEdgeType;
    uint8_t* m_horizontalVelocityType;
    uint8_t* m_verticalVelocityType;

    float* density[3];

    float* m_horizontalVelocityBuffer[2];
    float* m_verticalVelocityBuffer[2];

    float* m_targetHorizontalVelocityWeight;
    float* m_targetVerticalVelocityWeight;

    int m_activeVelocityBufferIndex;
    int m_inactiveVelocityBufferIndex;

    int m_blockCountX;
    int m_blockCountY;
    int m_horizontalVelocityCountX;
    int m_horizontalVelocityCountY;
    int m_horizontalVelocityCount;
    int m_verticalVelocityCountX;
    int m_verticalVelocityCountY;
    int m_verticalVelocityCount;
    float m_blockSize;
    float m_ooBlockSize;

    bool m_isHorizontalLoop;
private:
    
    //void SetBound(int b, float* x);
    //int LinearSolve(int b, float* x, float* x0, float a, float c, int maxIter, float qualityThresold);

    void Diffuse(int b, float* x, float* x0, float diff, float dt, int maxIter, float qualityThresold);
    void Project(float* p);
    void Advect(int b, float* d, float* d0, float* velocX, float* velocY, float dt);
    void AdvectVelocity(float dt);

    void SwapVelocityBuffers();


    

    int m_diffuseMaxIter;
    int m_viscosityMaxIter;
    int m_projectMaxIter;
    float m_diffuseQualityThresold;
    float m_viscosityQualityThresold;
    float m_projectQualityThresold;


    
    float* tempDensity[3];
    float* p1;
    float* p2;
    float* m_divBuffer;

    //int m_blockCountXMask;
    int m_blockCount;
    //int m_blockCountWithBounds;
  
    //int m_blockCountXWithBounds;
    //int m_blockCountYWithBounds;

    float* Vx0;
    float* Vy0;
};



}

#endif