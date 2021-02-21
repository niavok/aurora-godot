#include "aurora_fluid_box.h"

#include <math.h>
#include <cassert>
#include <Math.hpp>
#define DEBUG_PROFILIN_LOG 1

#if DEBUG_PROFILIN_LOG
#include <chrono>
#endif


namespace godot
{

FluidBox::FluidBox(int blockCountX, int blockCountY, float blockSize, bool isHorizontalLoop)
    //: m_blockCountXMask(0)
    : m_blockCountX(blockCountX)
    , m_blockCountY(blockCountY)
    , m_blockSize(blockSize)
    , m_diffuseMaxIter(100)
    , m_viscosityMaxIter(100)
    , m_projectMaxIter(10)
    , m_diffuseQualityThresold(1e-5f)
    , m_viscosityQualityThresold(1e-5f)
    , m_projectQualityThresold(0.1f)
    , m_isHorizontalLoop(isHorizontalLoop)
{
    m_activeVelocityBufferIndex = 0;
    m_inactiveVelocityBufferIndex = 1;
    m_blockCount = m_blockCountX * m_blockCountY;
    m_horizontalVelocityCountY = m_blockCountY + 1;
    m_verticalVelocityCountX = m_blockCountX;
    m_verticalVelocityCountY = m_blockCountY + 1;

    if (isHorizontalLoop)
    {
        //assert((m_blockCountX & (m_blockCountX - 1)) == 0); // Ensure sizeX is power of 2 if loop
        //m_blockCountXMask = m_blockCountX - 1;

        m_horizontalVelocityCountX = m_blockCountX;

        //m_blockCountXWithBounds = m_blockCountX;
    }
    else
    {
        m_horizontalVelocityCountX = m_blockCountX+1;
        //m_blockCountXWithBounds = m_blockCountX + 2;
    }

    m_horizontalVelocityCount = m_horizontalVelocityCountX * m_horizontalVelocityCountY;
    m_verticalVelocityCount = m_verticalVelocityCountX * m_verticalVelocityCountY;

    //m_blockCountYWithBounds = m_blockCountY + 2;
    //m_blockCountWithBounds = m_blockCountXWithBounds * m_blockCountYWithBounds;


    for (int i = 0; i < 3; i++)
    {
        tempDensity[i] = new float[m_blockCount];
        density[i] = new float[m_blockCount];
    }

    for (int i = 0; i < 2; i++)
    {
        m_horizontalVelocityBuffer[i] = new float[m_horizontalVelocityCount];
        m_verticalVelocityBuffer[i] = new float[m_verticalVelocityCount];

        memset(m_horizontalVelocityBuffer[i], 0, m_horizontalVelocityCount * sizeof(float));
        memset(m_verticalVelocityBuffer[i], 0, m_verticalVelocityCount * sizeof(float));
    }

    m_verticalVelocityType = new uint8_t[m_verticalVelocityCount];
    m_horizontalVelocityType = new uint8_t[m_horizontalVelocityCount];


    memset(m_verticalVelocityType, Velocity_FREE, m_horizontalVelocityCount * sizeof(uint8_t));
    memset(m_horizontalVelocityType, Velocity_FREE, m_verticalVelocityCount * sizeof(uint8_t));


    /*Vx = new float[m_vCountX];
    Vy = new float[m_verticalVelocityCount];

    Vx0 = new float[m_vCountX];
    Vy0 = new float[m_vCountY];*/

    p1 = new float[m_blockCount];
    p2 = new float[m_blockCount];
    m_divBuffer = new float[m_blockCount];

    m_blockEdgeType = new uint8_t[m_blockCount];


    for (int i = 0; i < m_blockCount; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            tempDensity[j][i] = 0.f;
            density[j][i] = 0.f;
        }

        p1[i] = 0.f;
        p2[i] = 0.f;

        m_blockEdgeType[i] = BlockEdge_VOID;
        m_divBuffer[i] = 0;
    }
}

FluidBox::~FluidBox()
{
    for (int i = 0; i < 3; i++)
    {
        delete[] tempDensity[i];
        delete[] density[i];
    }

    for (int i = 0; i < 2; i++)
    {
        delete[] m_horizontalVelocityBuffer[i];
        delete[] m_verticalVelocityBuffer[i];
    }

    delete[] m_horizontalVelocityType;
    delete[] m_verticalVelocityType;
    
    delete[] p1;
    delete[] p2;
    delete[] m_divBuffer;

    delete[] m_blockEdgeType;
}

int FluidBox::BlockIndex(int x, int y)
{
    return x + y * m_blockCountX;
}

//int FluidBox::BlockIndexLoop(int x, int y)
//{
//    return (x % m_blockCountX) + y * m_blockCountX;
//}

void FluidBox::AddDensity(int x, int y, float amount, int color)
{
    density[color][BlockIndex(x, y)] += amount;
    tempDensity[color][BlockIndex(x, y)] += amount;
}

//void FluidBox::AddVelocity(int x, int y, float amountX, float amountY)
//{
//    int index = Index(x, y);
//
//    Vx[index] += amountX;
//    Vy[index] += amountY;
//    Vx0[index] += amountX;
//    Vy0[index] += amountY;
//}
//
//void FluidBox::SetVelocity(int x, int y, float amountX, float amountY)
//{
//    int index = Index(x, y);
//
//    Vx[index] = amountX;
//    Vy[index] = amountY;
//    Vx0[index] = amountX;
//    Vy0[index] = amountY;
//}

void FluidBox::SetHorizontalVelocityAtLeft(int blockX, int blockY, float velocity)
{
    int velocityIndex = blockX + m_horizontalVelocityCountX * blockY;
    m_horizontalVelocityBuffer[m_activeVelocityBufferIndex][velocityIndex] = velocity;
}

//
//void FluidBox::SetBound(int b, float* x)
//{
//    if (m_isHorizontalLoop)
//    {
//        for (int i = 0; i < m_blockCountX; i++) {
//            //x[Index(i, 0)] = b == 2 ? -x[Index(i, 1)] : x[Index(i, 1)];
//            //x[Index(i, m_blockCountY - 1)] = b == 2 ? -x[Index(i, m_blockCountY - 2)] : x[Index(i, m_blockCountY - 2)];
//            //x[Index(i, 0)] = b == 2 ? 0 : x[Index(i, 1)];
//            //x[Index(i, m_blockCountY - 1)] = b == 2 ? 0 : x[Index(i, m_blockCountY - 2)];
//
//            x[Index(i, 0)] = b == 0 ? x[Index(i, 1)] : 0;
//            x[Index(i, m_blockCountY - 1)] = b == 0 ? x[Index(i, m_blockCountY - 2)] : 0;
//
//            //x[Index(i, 0)] = 0;
//            //x[Index(i, m_blockCountY-1)] = 0;
//        }
//
//        //for (int j = 30; j < 90; j++)
//        //{
//        //    int i = 100;
//
//        //    //x[Index(i, j)] = b == 0 ? x[Index(i-1, j)] + x[Index(i+1, j)] + x[Index(i, j-1)] + x[Index(i, j+1)] : 0;
//        //    x[Index(i, j)] = b == 0 ? x[Index(i - 1, j)] : 0;
//        //    x[Index(i+1, j)] = b == 0 ? x[Index(i+2, j)] : 0;
//        //}
//
//    }
//    else
//    {
//        for (int i = 1; i < m_blockCountX - 1; i++) {
//            x[Index(i, 0)] = b == 2 ? -x[Index(i, 1)] : x[Index(i, 1)];
//            x[Index(i, m_blockCountY - 1)] = b == 2 ? -x[Index(i, m_blockCountY - 2)] : x[Index(i, m_blockCountY - 2)];
//        }
//
//        for (int j = 1; j < m_blockCountY - 1; j++) {
//            x[Index(0, j)] = b == 1 ? -x[Index(1, j)] : x[Index(1, j)];
//            x[Index(m_blockCountX - 1, j)] = b == 1 ? -x[Index(m_blockCountX - 2, j)] : x[Index(m_blockCountX - 2, j)];
//        }
//
//
//        x[Index(0, 0)] = 0.5f * (x[Index(1, 0)] + x[Index(0, 1)]);
//        x[Index(0, m_blockCountY - 1)] = 0.5f * (x[Index(1, m_blockCountY - 1)] + x[Index(0, m_blockCountY - 2)]);
//        x[Index(m_blockCountX - 1, 0)] = 0.5f * (x[Index(m_blockCountX - 2, 0)] + x[Index(m_blockCountX - 1, 1)]);
//        x[Index(m_blockCountX - 1, m_blockCountY - 1)] = 0.5f * (x[Index(m_blockCountX - 2, m_blockCountY - 1)] + x[Index(m_blockCountX - 1, m_blockCountY - 2)]);
//    }
//}
//
//int FluidBox::LinearSolve(int b, float* x, float* x0, float a, float c, int maxIter, float qualityThresold)
//{
//    float cRecip = 1.0f / c;
//
//    int k = 0;
//
//    if (m_isHorizontalLoop)
//    {
//        for (k = 0; k < maxIter; k++) {
//            float maxCorrection = 0;
//            for (int j = 1; j < m_blockCountY - 1; j++) {
//                for (int i = 0; i < m_blockCountX; i++) {
//
//                    uint8_t type = blockEdgeType[Index(i, j)];
//                    if (type != 0)
//                    {
//                        //x[Index(0, j)] = b == 1 ? -x[Index(1, j)] : x[Index(1, j)];
//                        //x[Index(m_blockCountX - 1, j)] = b == 1 ? -x[Index(m_blockCountX - 2, j)] : x[Index(m_blockCountX - 2, j)];
//
//                        if (type == 1)
//                        {
//                            // Left border
//                            //x[Index(i, j)] = x[Index(i-1, j)];
//                            x[Index(i, j)] = b == 1 ? -x[Index(i - 1, j)] : x[Index(i - 1, j)];
//                        }
//                        else if (type == 2)
//                        {
//                            x[Index(i, j)] = b == 1 ? -x[Index(i + 1, j)] : x[Index(i + 1, j)];
//                            //x[Index(i, j)] = -x[Index(i + 1, j)];
//                        }
//
//
//                        //x[Index(i, j)] = 0;
//                        /*if(b==0)
//                        x[Index(i, 0)] = b == 0 ? x[Index(i, 1)] : 0;
//                        x[Index(i, m_blockCountY - 1)] = b == 0 ? x[Index(i, m_blockCountY - 2)] : 0;*/
//                    }
//                    else
//                    {
//
//                        float newX = (x0[Index(i, j)]
//                            + a * (x[IndexLoop(i + 1, j)] + x[IndexLoop(i - 1, j)] + x[Index(i, j + 1)] + x[Index(i, j - 1)])
//                            ) * cRecip;
//                        float correction = abs(newX - x[Index(i, j)]);
//                        if (maxCorrection < correction)
//                        {
//                            maxCorrection = correction;
//                        }
//                        x[Index(i, j)] = newX;
//                    }
//                }
//            }
//
//            SetBound(b, x);
//
//            if (maxCorrection <= qualityThresold)
//            {
//                break;
//            }
//        }
//    }
//    else
//    {
//        for (k = 0; k < maxIter; k++) {
//            float maxCorrection = 0;
//            for (int j = 1; j < m_blockCountY - 1; j++) {
//                for (int i = 1; i < m_blockCountX - 1; i++) {
//                    float newX =
//                        (x0[Index(i, j)]
//                            + a * (x[Index(i + 1, j)] + x[Index(i - 1, j)] + x[Index(i, j + 1)] + x[Index(i, j - 1)])
//                            ) * cRecip;
//                    float correction = abs(newX - x[Index(i, j)]);
//                    if (maxCorrection < correction)
//                    {
//                        maxCorrection = correction;
//                    }
//                    x[Index(i, j)] = newX;
//                }
//            }
//
//            SetBound(b, x);
//
//
//            if (maxCorrection <= qualityThresold)
//            {
//                break;
//            }
//        }
//    }
//
//    return k;
//}

void FluidBox::Step(float dt, float diff, float visc)
{


    memcpy(m_horizontalVelocityBuffer[m_inactiveVelocityBufferIndex], m_horizontalVelocityBuffer[m_activeVelocityBufferIndex], sizeof(float) * m_horizontalVelocityCount);
    memcpy(m_verticalVelocityBuffer[m_inactiveVelocityBufferIndex], m_verticalVelocityBuffer[m_activeVelocityBufferIndex], sizeof(float) * m_verticalVelocityCount);
#if 0
    Diffuse(1, Vx0, Vx, visc, dt, m_viscosityMaxIter, m_viscosityQualityThresold);
    Diffuse(2, Vy0, Vy, visc, dt, m_viscosityMaxIter, m_viscosityQualityThresold);
#else
    //memcpy(Vx, Vx0, sizeof(float) * m_blockCount);
    //memcpy(Vy, Vy0, sizeof(float) * m_blockCount);
#endif

#if 0
    Project(Vx0, Vy0, p1, divBuffer);
    Advect(1, Vx, Vx0, Vx0, Vy0, dt);
    Advect(2, Vy, Vy0, Vx0, Vy0, dt);
#endif

    Project(p1);

    //AdvectVelocity(dt);

    //Project(p2);

#if 0
    for (int i = 0; i < 3; i++)
    {
        float* color_s = tempDensity[i];
        float* color_density = density[i];

        memcpy(color_s, color_density, sizeof(float) * m_blockCount);
        Diffuse(0, color_s, color_density, diff, dt, m_diffuseMaxIter, m_diffuseQualityThresold);
        Advect(0, color_density, color_s, Vx, Vy, dt);
    }
#endif
}

void FluidBox::Diffuse(int b, float* x, float* x0, float diff, float dt, int maxIter, float qualityThresold)
{
    //float a = dt * diff * (m_blockCountX - 2) * (m_blockCountY - 2);
    float a = dt * diff / m_blockSize /** m_blockSize*/;
    //LinearSolve(b, x, x0, a, 1 + 4 * a, maxIter, qualityThresold);
}

void FluidBox::Project(float* p)
{
    float* horizontalVelocity = m_horizontalVelocityBuffer[m_activeVelocityBufferIndex];
    float* verticalVelocity = m_verticalVelocityBuffer[m_activeVelocityBufferIndex];
    float* div = m_divBuffer;

    int horizontalVelocityCountX = m_horizontalVelocityCountX;
    int verticalVelocityCountX = m_verticalVelocityCountX;

    auto LeftHorizontalVelocity = [horizontalVelocityCountX, horizontalVelocity](int blockX, int blockY) -> float {
        int velocityIndex = blockX + horizontalVelocityCountX * blockY;
        return horizontalVelocity[velocityIndex];
    };

    auto RightHorizontalVelocity = [horizontalVelocityCountX, horizontalVelocity](int blockX, int blockY) -> float {
        int velocityIndex = blockX + horizontalVelocityCountX * blockY +1;
        return horizontalVelocity[velocityIndex];
    };

    auto RightHorizontalVelocityLooping = [horizontalVelocityCountX, horizontalVelocity](int blockX, int blockY) -> float {
        int velocityIndex = blockX + horizontalVelocityCountX * (blockY-1) + 1;
        return horizontalVelocity[velocityIndex];
    };

    auto TopVerticalVelocity = [verticalVelocityCountX, verticalVelocity](int blockX, int blockY) -> float {
        int velocityIndex = blockX + verticalVelocityCountX * blockY;
        return verticalVelocity[velocityIndex];
    };

    auto BottomVerticalVelocity = [verticalVelocityCountX, verticalVelocity](int blockX, int blockY) -> float {
        int velocityIndex = blockX + verticalVelocityCountX * (blockY+1);
        return verticalVelocity[velocityIndex];
    };


    for (int blockY = 0; blockY < m_blockCountY; blockY++)
    {
        for (int blockX = 0; blockX < m_blockCountX; blockX++)
        {
            int blockIndex = BlockIndex(blockX, blockY);
            uint8_t type = m_blockEdgeType[blockIndex];

            switch (type)
            {
            case BlockEdge_VOID:
                assert(blockX > 0 && blockY > 0 && blockX < m_blockCountX - 1 && blockY < m_blockCountY - 1);
                div[blockIndex] = RightHorizontalVelocity(blockX, blockY) - LeftHorizontalVelocity(blockX, blockY)
                    + BottomVerticalVelocity(blockX, blockY) - TopVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_FILL:
                continue;
                break;
            case BlockEdge_TOP_EDGE:
                assert(blockX > 0 && blockX < m_blockCountX - 1 && blockY < m_blockCountY - 1);
                div[blockIndex] = RightHorizontalVelocity(blockX, blockY) - LeftHorizontalVelocity(blockX, blockY)
                    + BottomVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_BOTTOM_EDGE:
                assert(blockX > 0 && blockY > 0 && blockX < m_blockCountX - 1);
                div[blockIndex] = RightHorizontalVelocity(blockX, blockY) - LeftHorizontalVelocity(blockX, blockY)
                    - TopVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_LEFT_EDGE:
                assert(blockY > 0 && blockX < m_blockCountX - 1 && blockY < m_blockCountY - 1);
                div[blockIndex] = RightHorizontalVelocity(blockX, blockY)
                    + BottomVerticalVelocity(blockX, blockY) - TopVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_RIGHT_EDGE:
                assert(blockX > 0 && blockY > 0 && blockY < m_blockCountY - 1);
                div[blockIndex] = -LeftHorizontalVelocity(blockX, blockY)
                    + BottomVerticalVelocity(blockX, blockY) - TopVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_TOP_LEFT_CORNER:
                assert(blockX < m_blockCountX - 1 && blockY < m_blockCountY - 1);
                div[blockIndex] = RightHorizontalVelocity(blockX, blockY)
                    + BottomVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_TOP_RIGHT_CORNER:
                assert(blockX > 0 && blockY < m_blockCountY - 1);
                div[blockIndex] = -LeftHorizontalVelocity(blockX, blockY)
                    + BottomVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_BOTTOM_LEFT_CORNER:
                assert(blockY > 0 && blockX < m_blockCountX - 1);
                div[blockIndex] = RightHorizontalVelocity(blockX, blockY)
                    - TopVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_BOTTOM_RIGHT_CORNER:
                assert(blockX > 0 && blockY > 0);
                div[blockIndex] = - LeftHorizontalVelocity(blockX, blockY)
                    - TopVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_HORIZONTAL_PIPE:
                assert(blockX > 0 && blockX < m_blockCountX - 1);
                div[blockIndex] = RightHorizontalVelocity(blockX, blockY) - LeftHorizontalVelocity(blockX, blockY);
                break;
            case BlockEdge_LOOPING_LEFT_VOID:
                assert(blockX == 0 && blockY > 0 && blockY < m_blockCountY - 1);
                div[blockIndex] = RightHorizontalVelocity(blockX, blockY) - LeftHorizontalVelocity(blockX, blockY)
                    + BottomVerticalVelocity(blockX, blockY) - TopVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_LOOPING_RIGHT_VOID:
                assert(blockX == m_blockCountX - 1 && blockY > 0 && blockY < m_blockCountY - 1);
                div[blockIndex] = RightHorizontalVelocityLooping(blockX, blockY) - LeftHorizontalVelocity(blockX, blockY)
                    + BottomVerticalVelocity(blockX, blockY) - TopVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_LOOPING_LEFT_TOP_EDGE:
                assert(blockX == 0 && blockY == 0);
                div[blockIndex] = RightHorizontalVelocity(blockX, blockY) - LeftHorizontalVelocity(blockX, blockY)
                    + BottomVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_LOOPING_RIGHT_TOP_EDGE:
                assert(blockX == m_blockCountX - 1 && blockY == 0);
                div[blockIndex] = RightHorizontalVelocityLooping(blockX, blockY) - LeftHorizontalVelocity(blockX, blockY)
                    + BottomVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_LOOPING_LEFT_BOTTOM_EDGE:
                assert(blockX == 0 && blockY == m_blockCountY - 1);
                div[blockIndex] = RightHorizontalVelocity(blockX, blockY) - LeftHorizontalVelocity(blockX, blockY)
                    - TopVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_LOOPING_RIGHT_BOTTOM_EDGE:
                assert(blockX == m_blockCountX - 1 && blockY == m_blockCountY - 1);
                div[blockIndex] = RightHorizontalVelocityLooping(blockX, blockY) - LeftHorizontalVelocity(blockX, blockY)
                    - TopVerticalVelocity(blockX, blockY);
                break;
            case BlockEdge_LOOPING_LEFT_HORIZONTAL_PIPE:
                assert(blockX == 0);
                div[blockIndex] = RightHorizontalVelocity(blockX, blockY) - LeftHorizontalVelocity(blockX, blockY);
                break;
            case BlockEdge_LOOPING_RIGHT_HORIZONTAL_PIPE:
                assert(blockX == m_blockCountX - 1);
                div[blockIndex] = RightHorizontalVelocityLooping(blockX, blockY) - LeftHorizontalVelocity(blockX, blockY);
                break;
            default:
                assert(false); // Invalid block edge type
            }
        }
    }

    int k;
    for (k = 0; k < m_projectMaxIter; k++)
    {
        float maxCorrection = 0;
        for (int blockIndex = 0; blockIndex < m_blockCount; blockIndex++)
        {
            uint8_t type = m_blockEdgeType[blockIndex];
            float newP;
            switch (type)
            {
            case BlockEdge_VOID:
                newP = 0.25f * (div[blockIndex]
                    + p[blockIndex + 1] + p[blockIndex - 1]
                    + p[blockIndex + m_blockCountX] + p[blockIndex - m_blockCountX]
                    );
                break;
            case BlockEdge_FILL:
                continue;
                break;
            case BlockEdge_TOP_EDGE:
                newP = (1.f / 3.f) * (div[blockIndex]
                    + p[blockIndex + 1] + p[blockIndex - 1]
                    + p[blockIndex + m_blockCountX]
                    );
                break;
            case BlockEdge_BOTTOM_EDGE:
                newP = (1.f / 3.f) * (div[blockIndex]
                    + p[blockIndex + 1] + p[blockIndex - 1]
                    + p[blockIndex - m_blockCountX]
                    );
                break;
            case BlockEdge_LEFT_EDGE:
                newP = (1.f / 3.f) * (div[blockIndex]
                    + p[blockIndex + 1]
                    + p[blockIndex + m_blockCountX] + p[blockIndex - m_blockCountX]
                    );
                break;
            case BlockEdge_RIGHT_EDGE:
                newP = (1.f / 3.f) * (div[blockIndex]
                    + p[blockIndex - 1]
                    + p[blockIndex + m_blockCountX] + p[blockIndex - m_blockCountX]
                    );
                break;
            case BlockEdge_TOP_LEFT_CORNER:
                newP = (1.f / 2.f) * (div[blockIndex]
                    + p[blockIndex + 1]
                    + p[blockIndex + m_blockCountX]
                    );
                break;
            case BlockEdge_TOP_RIGHT_CORNER:
                newP = (1.f / 2.f) * (div[blockIndex]
                    + p[blockIndex - 1]
                    + p[blockIndex + m_blockCountX]
                    );
                break;
            case BlockEdge_BOTTOM_LEFT_CORNER:
                newP = (1.f / 2.f) * (div[blockIndex]
                    + p[blockIndex + 1]
                    + p[blockIndex - m_blockCountX]
                    );
                break;
            case BlockEdge_BOTTOM_RIGHT_CORNER:
                newP = (1.f / 2.f) * (div[blockIndex]
                    + p[blockIndex - 1]
                    + p[blockIndex - m_blockCountX]
                    );
                break;
            case BlockEdge_HORIZONTAL_PIPE:
                newP = 0.5f * (div[blockIndex]
                    + p[blockIndex + 1] + p[blockIndex - 1]);
                break;
            case BlockEdge_LOOPING_LEFT_VOID:
                newP = 0.25f * (div[blockIndex]
                    + p[blockIndex + 1] + p[blockIndex - 1 + m_blockCountX]
                    + p[blockIndex + m_blockCountX] + p[blockIndex - m_blockCountX]
                    );
                break;
            case BlockEdge_LOOPING_RIGHT_VOID:
                newP = 0.25f * (div[blockIndex]
                    + p[blockIndex + 1 - m_blockCountX] + p[blockIndex - 1]
                    + p[blockIndex + m_blockCountX] + p[blockIndex - m_blockCountX]
                    );
                break;
            case BlockEdge_LOOPING_LEFT_TOP_EDGE:
                newP = (1.f / 3.f) * (div[blockIndex]
                    + p[blockIndex + 1] + p[blockIndex - 1 + m_blockCountX]
                    + p[blockIndex + m_blockCountX]
                    );
                break;
            case BlockEdge_LOOPING_RIGHT_TOP_EDGE:
                newP = (1.f / 3.f) * (div[blockIndex]
                    + p[blockIndex + 1 - m_blockCountX] + p[blockIndex - 1]
                    + p[blockIndex + m_blockCountX]
                    );
                break;
            case BlockEdge_LOOPING_LEFT_BOTTOM_EDGE:
                newP = (1.f / 3.f) * (div[blockIndex]
                    + p[blockIndex + 1] + p[blockIndex - 1 + m_blockCountX]
                    + p[blockIndex - m_blockCountX]
                    );
                break;
            case BlockEdge_LOOPING_RIGHT_BOTTOM_EDGE:
                newP = (1.f / 3.f) * (div[blockIndex]
                    + p[blockIndex + 1 - m_blockCountX] + p[blockIndex - 1]
                    + p[blockIndex - m_blockCountX]
                    );
                break;
            case BlockEdge_LOOPING_LEFT_HORIZONTAL_PIPE:
                newP = 0.5f * (div[blockIndex]
                    + p[blockIndex + 1] + p[blockIndex - 1 + m_blockCountX]);
                break;
            case BlockEdge_LOOPING_RIGHT_HORIZONTAL_PIPE:
                newP = 0.5f * (div[blockIndex]
                    + p[blockIndex + 1 - m_blockCountX] + p[blockIndex - 1]);
                break;
            default:
                newP = 0;
                assert(false); // Invalid block edge type
            }

            float oldP = p[blockIndex];

            float newPWithSOR = Math::lerp(oldP, newP, 1.5f);

            float correction = abs(newPWithSOR - p[blockIndex]);
            if (maxCorrection < correction)
            {
                maxCorrection = correction;
            }
                    
            p[blockIndex] = newPWithSOR;

            //uint8_t type = blockType[Index(i, j)];
            //if (type != 0 && false)
            //{
            //    //x[Index(0, j)] = b == 1 ? -x[Index(1, j)] : x[Index(1, j)];
            //    //x[Index(m_blockCountX - 1, j)] = b == 1 ? -x[Index(m_blockCountX - 2, j)] : x[Index(m_blockCountX - 2, j)];

            //    if (type == 1)
            //    {
            //        // Left border
            //        //x[Index(i, j)] = x[Index(i-1, j)];
            //        p[Index(i, j)] = p[Index(i - 1, j)];
            //    }
            //    else if (type == 2)
            //    {
            //        p[Index(i, j)] = p[Index(i + 1, j)];
            //        //x[Index(i, j)] = -x[Index(i + 1, j)];
            //    }


            //    //x[Index(i, j)] = 0;
            //    /*if(b==0)
            //    x[Index(i, 0)] = b == 0 ? x[Index(i, 1)] : 0;
            //    x[Index(i, m_blockCountY - 1)] = b == 0 ? x[Index(i, m_blockCountY - 2)] : 0;*/
            //}
            //else
            //{

            //    //float newP = - 0.25 * (div[Index(i, j)] + p[IndexLoop(i + 1, j)] + p[IndexLoop(i - 1, j)] + p[Index(i, j + 1)] + p[Index(i, j - 1)]);
            //    float newP = 0.5 * (div[Index(i, j)] + p[IndexLoop(i + 1, j)] + p[IndexLoop(i - 1, j)]);
            //    float correction = abs(newP - p[Index(i, j)]);
            //    if (maxCorrection < correction)
            //    {
            //        maxCorrection = correction;
            //    }
            //    p[Index(i, j)] = newP;
            //}
                
        }

        //SetBound(0, p);

        if (maxCorrection <= m_projectQualityThresold)
        {
            break;
        }
    }

    printf("output in %d iter\n", k);

    int blockCountX = m_blockCountX;
    auto UpperBlockIdx = [blockCountX, horizontalVelocityCountX](int vIndex) -> int {
        int vY = vIndex / horizontalVelocityCountX;
        int vX = vIndex % horizontalVelocityCountX;


        return vX + vY * blockCountX - blockCountX;
    };

    auto LowerBlockIdx = [blockCountX, horizontalVelocityCountX](int vIndex) -> int {
        int vY = vIndex / horizontalVelocityCountX;
        int vX = vIndex % horizontalVelocityCountX;

        return vX + vY * blockCountX;
    };

    auto LeftBlockIdx = [blockCountX, horizontalVelocityCountX](int vIndex) -> int {
        int vY = vIndex / horizontalVelocityCountX;
        int vX = vIndex % horizontalVelocityCountX;
        //int lineIdx = vIndex / (blockCountX + 1);
        //return vIndex - lineIdx - 1;
        return vX + vY * blockCountX- 1;
    };

    auto RightBlockIdx = [blockCountX, horizontalVelocityCountX](int vIndex) -> int {
        int vY = vIndex / horizontalVelocityCountX;
        int vX = vIndex % horizontalVelocityCountX;
        //int lineIdx = vIndex / (blockCountX + 1);
        //return vIndex -lineIdx;
        return vX + vY * blockCountX;
    };

    for (int vIndex = 0; vIndex < m_horizontalVelocityCount; vIndex++)
    {
        uint8_t vxType = m_horizontalVelocityType[vIndex];
        switch (vxType)
        {
        case Velocity_ZERO:
            horizontalVelocity[vIndex] = 0;
            break;
        case Velocity_FREE:
            horizontalVelocity[vIndex] += p[RightBlockIdx(vIndex)] - p[LeftBlockIdx(vIndex)];
            if (horizontalVelocity[vIndex] < 0)
            {
                int plop = 1;
            }
            break;
        case Velocity_LOOPING_LEFT:
            horizontalVelocity[vIndex] += p[RightBlockIdx(vIndex)] - p[LeftBlockIdx(vIndex) + m_blockCountX];
            break;
        default:
            assert(false); // invalid velocity type
        }
    }

    for (int vIndex = 0; vIndex < m_verticalVelocityCount; vIndex++)
    {
        uint8_t vyType = m_verticalVelocityType[vIndex];
        switch (vyType)
        {
        case Velocity_ZERO:
            verticalVelocity[vIndex] = 0;
            break;
        case Velocity_FREE:
        case Velocity_LOOPING_RIGHT:
            verticalVelocity[vIndex] += p[LowerBlockIdx(vIndex)] - p[UpperBlockIdx(vIndex)];
            break;
        default:
            assert(false); // invalid velocity type
        }
    }
}

void FluidBox::AdvectVelocity(float stepDt)
{
    // Get max velocity
    float maxVelocity = 0;

    float* initialHorizontalVelocity = m_horizontalVelocityBuffer[m_activeVelocityBufferIndex];
    float* initialVerticalVelocity = m_verticalVelocityBuffer[m_activeVelocityBufferIndex];

    

    for (int blockIndex = 0; blockIndex < m_blockCount; blockIndex++)
    {
        float vx = initialHorizontalVelocity[blockIndex];
        float vy = initialVerticalVelocity[blockIndex];

        if (vx > maxVelocity)
        {
            maxVelocity = vx;
        }
        if (vy > maxVelocity)
        {
            maxVelocity = vy;
        }
    }

    float maxDtPerSubStep = m_blockSize / maxVelocity;
    int subStepCount = int(ceilf(stepDt / maxDtPerSubStep));

    float dt = stepDt / subStepCount;

    for (int k = 0; k < subStepCount; k++)
    {
        float* horizontalVelocity = m_horizontalVelocityBuffer[m_activeVelocityBufferIndex];
        float* verticalVelocity = m_verticalVelocityBuffer[m_activeVelocityBufferIndex];

        float* targetHorizontalVelocity = m_horizontalVelocityBuffer[m_inactiveVelocityBufferIndex];
        float* targetVerticalVelocity = m_verticalVelocityBuffer[m_inactiveVelocityBufferIndex];


        // Advect one substep
        for (int vIndex = 0; vIndex < m_horizontalVelocityCount; vIndex++)
        {
            uint8_t horizontalVelocityType = m_horizontalVelocityType[vIndex];


            //if (horizontalVelocityType == Velocity_FREE)
            //{
            //    float vx = horizontalVelocity[vIndex];
            //    float vy;
            //    if (vx > 0)
            //    {
            //        vy =
            //            float dx = -vx * dt;
            //    }

            //}
            //else if (vxType != Velocity_ZERO)
            //{
            //    // 
            //}

        }

        SwapVelocityBuffers();

            //switch (vxType)
            //{
            //case Velocity_ZERO:
            //    velocX[blockIndex] = 0.f;
            //    break;
            //case Velocity_FREE:
            //    break;
            //case Velocity_LOOPING_RIGHT:
            //    velocX[blockIndex] += p[blockIndex] - p[blockIndex - 1];
            //    break;
            //case Velocity_LOOPING_LEFT:
            //    velocX[blockIndex] += p[blockIndex] - p[blockIndex - 1 + m_blockCountX];
            //    break;
            //default:
            //    assert(false); // invalid velocity type
            //}



        
    }
}

void FluidBox::SwapVelocityBuffers()
{
    std::swap(m_activeVelocityBufferIndex, m_inactiveVelocityBufferIndex);
    //m_activeVelocityBufferIndex = m_inactiveVelocityBufferIndex;
}

//
//void FluidBox::Advect(int b, float* d, float* d0, float* velocX, float* velocY, float dt)
//{
//    float i0, i1, j0, j1;
//
//    float dtx = dt;
//    float dty = dt;
//
//    float s0, s1, t0, t1;
//    float tmp1, tmp2, x, y;
//
//    float NfloatX = float(m_blockCountX);
//    float NfloatY = float(m_blockCountY);
//    float ifloat, jfloat;
//    int i, j;
//
//    if (!m_isHorizontalLoop)
//    {
//        for (j = 1, jfloat = 1; j < m_blockCountY - 1; j++, jfloat++) {
//            for (i = 1, ifloat = 1; i < m_blockCountX - 1; i++, ifloat++) {
//                tmp1 = dtx * velocX[Index(i, j)];
//                tmp2 = dty * velocY[Index(i, j)];
//                x = ifloat - tmp1;
//                y = jfloat - tmp2;
//
//                if (x < 0.5f) x = 0.5f;
//                if (x > NfloatX + 0.5f) x = NfloatX + 0.5f;
//                i0 = floorf(x);
//                i1 = i0 + 1.0f;
//                if (y < 0.5f) y = 0.5f;
//                if (y > NfloatY + 0.5f) y = NfloatY + 0.5f;
//                j0 = floorf(y);
//                j1 = j0 + 1.0f;
//
//                s1 = x - i0;
//                s0 = 1.0f - s1;
//                t1 = y - j0;
//                t0 = 1.0f - t1;
//
//                int i0i = (int)i0;
//                int i1i = (int)i1;
//                int j0i = (int)j0;
//                int j1i = (int)j1;
//
//                d[Index(i, j)] =
//                    s0 * (t0 * d0[Index(i0i, j0i)] + t1 * d0[Index(i0i, j1i)])
//                    + s1 * (t0 * d0[Index(i1i, j0i)] + t1 * d0[Index(i1i, j1i)]);
//            }
//        }
//    }
//    else
//    {
//        for (j = 1, jfloat = 1; j < m_blockCountY - 1; j++, jfloat++) {
//            for (i = 0, ifloat = 0; i < m_blockCountX; i++, ifloat++) {
//                tmp1 = dtx * velocX[Index(i, j)];
//                tmp2 = dty * velocY[Index(i, j)];
//                x = ifloat - tmp1;
//                y = jfloat - tmp2;
//
//                //if (x < 0.5f) x = 0.5f;
//                //if (x > NfloatX + 0.5f) x = NfloatX + 0.5f;
//                i0 = floorf(x);
//                i1 = i0 + 1.0f;
//                if (y < 0.5f) y = 0.5f;
//                if (y > NfloatY + 0.5f) y = NfloatY + 0.5f;
//                j0 = floorf(y);
//                j1 = j0 + 1.0f;
//
//                s1 = x - i0;
//                s0 = 1.0f - s1;
//                t1 = y - j0;
//                t0 = 1.0f - t1;
//
//                int i0i = (int)i0;
//                int i1i = (int)i1;
//                int j0i = (int)j0;
//                int j1i = (int)j1;
//
//                d[Index(i, j)] =
//                    s0 * (t0 * d0[IndexLoop(i0i, j0i)] + t1 * d0[IndexLoop(i0i, j1i)])
//                    + s1 * (t0 * d0[IndexLoop(i1i, j0i)] + t1 * d0[IndexLoop(i1i, j1i)]);
//            }
//        }
//    }
//
//    SetBound(b, d);
//}

void FluidBox::DecayDensity(float keepRatio)
{
    for (int color = 0; color < 3; color++)
    {
        float* d = density[color];
        float* tempsD = tempDensity[color];

        for (int i = 0; i < m_blockCount; i++)
        {
            d[i] *= keepRatio;
            tempsD[i] *= keepRatio;
        }
    }
}

void FluidBox::CompileGrid()
{
    // Center blocks
    for (int blockY = 0; blockY < m_blockCountY; blockY++)
    {
        for (int blockX = 0; blockX < m_blockCountX; blockX++)
        {
            int blockIndex = BlockIndex(blockX, blockY);
            uint8_t& type = m_blockEdgeType[blockIndex];

            if (type == BlockEdge_FILL)
            {
                continue;
            }

            auto LeftHorizontalVelocityIndex = [this](int blockX, int blockY) -> int {
                return blockX + m_horizontalVelocityCountX * blockY;
            };

            auto RightHorizontalVelocityIndex = [this](int blockX, int blockY) -> int {
                if (blockY == m_blockCountY - 1 && m_isHorizontalLoop)
                {
                    return blockX + m_horizontalVelocityCountX * (blockY - 1) + 1;
                }
                else
                {
                    return blockX + m_horizontalVelocityCountX * blockY + 1;
                }
            };

            auto TopVerticalVelocityIndex = [this](int blockX, int blockY) -> int {
                return blockX + m_verticalVelocityCountX * blockY;
            };

            auto BottomVerticalVelocityIndex = [this](int blockX, int blockY) -> int {
                return blockX + m_verticalVelocityCountX * (blockY + 1);
            };

            auto ConfigureVelocities = [&](VelocityType topType, VelocityType bottomType, VelocityType leftType, VelocityType rightType)
            {
                m_horizontalVelocityType[LeftHorizontalVelocityIndex(blockX, blockY)] = leftType;
                if (rightType != Velocity_LOOPING_RIGHT)
                {
                    m_horizontalVelocityType[RightHorizontalVelocityIndex(blockX, blockY)] = rightType;
                }

                m_verticalVelocityType[TopVerticalVelocityIndex(blockX, blockY)] = topType;
                m_verticalVelocityType[BottomVerticalVelocityIndex(blockX, blockY)] = bottomType;               
            };

            bool isTopFilled = blockY == 0 || m_blockEdgeType[blockIndex - m_blockCountX] == BlockEdge_FILL;
            bool isBottomFilled = blockY == m_blockCountY-1 || m_blockEdgeType[blockIndex + m_blockCountX] == BlockEdge_FILL;
            bool isLeftFilled;
            if (blockX == 0)
            {
                isLeftFilled = m_isHorizontalLoop ? m_blockEdgeType[blockIndex - 1 + m_blockCountX] == BlockEdge_FILL : true;
            }
            else
            {
                isLeftFilled = m_blockEdgeType[blockIndex - 1] == BlockEdge_FILL;
            }
            
            bool isRightFilled;
            if (blockX == m_blockCountX-1)
            {
                isRightFilled = m_isHorizontalLoop ? m_blockEdgeType[blockIndex + 1 - m_blockCountX] == BlockEdge_FILL : true;
            }
            else
            {
                isRightFilled = m_blockEdgeType[blockIndex + 1] == BlockEdge_FILL;
            }

            if (isTopFilled)
            {
                if (isBottomFilled)
                {
                    if (isLeftFilled)
                    {
                        if (isRightFilled)
                        {
                            //  #
                            // #x#
                            //  #
                            assert(false); // Not handled yet
                        }
                        else
                        {
                            //  #
                            // #x-
                            //  #
                            assert(false); // Not handled yet
                        }
                    }
                    else
                    {
                        if (isRightFilled)
                        {
                            //  #
                            // -x#
                            //  #
                            assert(false); // Not handled yet
                        }
                        else
                        {
                            //  #
                            // -x-
                            //  #
                            if (blockX == 0)
                            {
                                type = BlockEdge_LOOPING_LEFT_HORIZONTAL_PIPE;
                                ConfigureVelocities(Velocity_ZERO, Velocity_ZERO, Velocity_LOOPING_LEFT, Velocity_FREE);
                            }
                            else if (blockX == m_blockCountX - 1)
                            {
                                type = BlockEdge_LOOPING_RIGHT_HORIZONTAL_PIPE;
                                ConfigureVelocities(Velocity_ZERO, Velocity_ZERO, Velocity_FREE, Velocity_LOOPING_RIGHT);
                            }
                            else
                            {
                                type = BlockEdge_HORIZONTAL_PIPE;
                                ConfigureVelocities(Velocity_ZERO, Velocity_ZERO, Velocity_FREE, Velocity_FREE);
                            }
                        }
                    }
                }
                else
                {
                    if (isLeftFilled)
                    {
                        if (isRightFilled)
                        {
                            //  #
                            // #x#
                            //  -
                            assert(false); // Not handled yet
                        }
                        else
                        {
                            //   #
                             // #x-
                             //  -
                            type = BlockEdge_TOP_LEFT_CORNER;
                            ConfigureVelocities(Velocity_ZERO, Velocity_FREE, Velocity_ZERO, Velocity_FREE);
                        }
                    }
                    else
                    {
                        if (isRightFilled)
                        {
                            //  #
                            // -x#
                            //  -
                            type = BlockEdge_TOP_RIGHT_CORNER;
                            ConfigureVelocities(Velocity_ZERO, Velocity_FREE, Velocity_FREE, Velocity_ZERO);
                        }
                        else
                        {
                            //  #
                            // -x-
                            //  -
                            if (blockX == 0)
                            {
                                type = BlockEdge_LOOPING_LEFT_TOP_EDGE;
                                ConfigureVelocities(Velocity_ZERO, Velocity_FREE, Velocity_LOOPING_LEFT, Velocity_FREE);
                            }
                            else if (blockX == m_blockCountX - 1)
                            {
                                type = BlockEdge_LOOPING_RIGHT_TOP_EDGE;
                                ConfigureVelocities(Velocity_ZERO, Velocity_LOOPING_RIGHT, Velocity_FREE, Velocity_LOOPING_RIGHT);
                            }
                            else
                            {
                                type = BlockEdge_TOP_EDGE;
                                ConfigureVelocities(Velocity_ZERO, Velocity_FREE, Velocity_FREE, Velocity_FREE);
                            }
                            
                        }
                    }
                }
            }
            else
            {
                if (isBottomFilled)
                {
                    if (isLeftFilled)
                    {
                        if (isRightFilled)
                        {
                            //  -
                            // #x#
                            //  #
                            assert(false); // Not handled yet
                        }
                        else
                        {
                            //  -
                            // #x-
                            //  #                            
                            type = BlockEdge_BOTTOM_LEFT_CORNER;
                            ConfigureVelocities(Velocity_FREE, Velocity_ZERO, Velocity_ZERO, Velocity_FREE);
                        }
                    }
                    else
                    {
                        if (isRightFilled)
                        {
                            //  -
                            // -x#
                            //  #
                            type = BlockEdge_BOTTOM_RIGHT_CORNER;
                            ConfigureVelocities(Velocity_FREE, Velocity_ZERO, Velocity_FREE, Velocity_ZERO);
                        }
                        else
                        {
                            //  -
                            // -x-
                            //  #
                            if (blockX == 0)
                            {
                                type = BlockEdge_LOOPING_LEFT_BOTTOM_EDGE;
                                ConfigureVelocities(Velocity_FREE, Velocity_ZERO, Velocity_LOOPING_LEFT, Velocity_FREE);
                            }
                            else if (blockX == m_blockCountX - 1)
                            {
                                type = BlockEdge_LOOPING_RIGHT_BOTTOM_EDGE;
                                ConfigureVelocities(Velocity_LOOPING_RIGHT, Velocity_ZERO, Velocity_FREE, Velocity_LOOPING_RIGHT);
                            }
                            else
                            {
                                type = BlockEdge_BOTTOM_EDGE;
                                ConfigureVelocities(Velocity_FREE, Velocity_ZERO, Velocity_FREE, Velocity_FREE);
                            }
                        }
                    }
                }
                else
                {
                    if (isLeftFilled)
                    {
                        if (isRightFilled)
                        {
                            //  -
                            // #x#
                            //  -
                            assert(false); // Not handled yet
                        }
                        else
                        {
                            //  -
                            // #x-
                            //  -
                            type = BlockEdge_LEFT_EDGE;
                            ConfigureVelocities(Velocity_FREE, Velocity_FREE, Velocity_ZERO, Velocity_FREE);
                        }
                    }
                    else
                    {
                        if (isRightFilled)
                        {
                            //  -
                            // -x#
                            //  -
                            type = BlockEdge_RIGHT_EDGE;
                            ConfigureVelocities(Velocity_FREE, Velocity_FREE, Velocity_FREE, Velocity_ZERO);
                        }
                        else
                        {
                            //  -
                            // -x-
                            //  -
                            if (blockX == 0)
                            {
                                type = BlockEdge_LOOPING_LEFT_VOID;
                                ConfigureVelocities(Velocity_FREE, Velocity_FREE, Velocity_LOOPING_LEFT, Velocity_FREE);
                            }
                            else if (blockX == m_blockCountX - 1)
                            {
                                type = BlockEdge_LOOPING_RIGHT_VOID;
                                ConfigureVelocities(Velocity_LOOPING_RIGHT, Velocity_LOOPING_RIGHT, Velocity_FREE, Velocity_LOOPING_RIGHT);
                            }
                            else
                            {
                                type = BlockEdge_VOID;
                                ConfigureVelocities(Velocity_FREE, Velocity_FREE, Velocity_FREE, Velocity_FREE);
                            }
                        }
                    }
                }
            }           
        }
    }

}




}