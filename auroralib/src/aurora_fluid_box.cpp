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
    : m_blockCountX(blockCountX)
    , m_blockCountY(blockCountY)
    , m_blockSize(blockSize)
    , m_blockDepth(1)
    , m_blockSection(m_blockSize * m_blockDepth)
    , m_blockVolume(m_blockSize* m_blockSize* m_blockDepth)
    , m_ooBlockSize(1.f / blockSize)
    , m_diffuseMaxIter(100)
    , m_viscosityMaxIter(20)
    , m_projectMaxIter(200)
    , m_diffuseQualityThresold(1e-5f)
    , m_viscosityQualityThresold(1e-5f)
    , m_projectQualityThresold(0.1f)
    , m_isHorizontalLoop(isHorizontalLoop)
{
    m_activeVelocityBufferIndex = 0;
    m_inactiveVelocityBufferIndex = 1;
    m_activeContentBufferIndex = 0;
    m_inactiveContentBufferIndex = 1;
    m_blockCount = m_blockCountX * m_blockCountY;
    m_horizontalVelocityCountY = m_blockCountY;
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
        //tempDensity[i] = new float[m_blockCount];
        //density[i] = new float[m_blockCount];
    }

    for (int i = 0; i < 2; i++)
    {
        m_horizontalVelocityBuffer[i] = new float[m_horizontalVelocityCount];
        m_verticalVelocityBuffer[i] = new float[m_verticalVelocityCount];

        memset(m_horizontalVelocityBuffer[i], 0, m_horizontalVelocityCount * sizeof(float));
        memset(m_verticalVelocityBuffer[i], 0, m_verticalVelocityCount * sizeof(float));

        m_contentBuffer[i] = new Content[m_blockCount];

        memset(m_contentBuffer[i], 0, m_blockCount * sizeof(Content));
    }

    m_verticalVelocityType = new uint8_t[m_verticalVelocityCount];
    m_horizontalVelocityType = new uint8_t[m_horizontalVelocityCount];

    memset(m_horizontalVelocityType, Velocity_FREE, m_horizontalVelocityCount * sizeof(uint8_t));
    memset(m_verticalVelocityType, Velocity_FREE, m_verticalVelocityCount * sizeof(uint8_t));

    


    /*Vx = new float[m_vCountX];
    Vy = new float[m_verticalVelocityCount];

    Vx0 = new float[m_vCountX];
    Vy0 = new float[m_vCountY];*/

    p1 = new float[m_blockCount];
    p2 = new float[m_blockCount];
    m_divBuffer = new float[m_blockCount];

    m_blockConfig.resize(m_blockCount);

    ///m_blockEdgeType = new uint8_t[m_blockCount];


    for (int i = 0; i < m_blockCount; i++)
    {
        //for (int j = 0; j < 3; j++)
        //{
        //    tempDensity[j][i] = 0.f;
        //    density[j][i] = 0.f;
        //}

        p1[i] = 0.f;
        p2[i] = 0.f;

        //m_blockEdgeType[i] = BlockEdge_VOID;
        m_divBuffer[i] = 0;

        m_blockConfig[i].Init();
    }

    for (int j = 0; j < m_blockCountY; j++)
    {
        for (int i = 0; i < m_blockCountX; i++)
        {
            SetContent(i, j, 0.f, 0.f, 0.f);
        }
    }
}

void FluidBox::BlockConfig::Init()
{
    isFill = 0;
    isLeftConnected = 0;
    isRightConnected = 0;
    isTopConnected = 0;
    isBottomConnected = 0;

    leftBlockIndex = -1;
    rightBlockIndex = -1;
    topBlockIndex = -1;
    bottomBlockIndex = -1;

    topLeftBlockIndex = -1;
    topRightBlockIndex = -1;
    bottomLeftBlockIndex = -1;
    bottomRightBlockIndex = -1;

    leftVelocityIndex = -1;
    rightVelocityIndex = -1;
    topVelocityIndex = -1;
    bottomVelocityIndex = -1;
}

FluidBox::~FluidBox()
{
    /*for (int i = 0; i < 3; i++)
    {
        delete[] tempDensity[i];
        delete[] density[i];
    }*/

    for (int i = 0; i < 2; i++)
    {
        delete[] m_horizontalVelocityBuffer[i];
        delete[] m_verticalVelocityBuffer[i];
        delete[] m_contentBuffer[i];
    }

    delete[] m_horizontalVelocityType;
    delete[] m_verticalVelocityType;

    delete[] p1;
    delete[] p2;
    delete[] m_divBuffer;

    //delete[] m_blockEdgeType;
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
    Content& content = GetActiveBlockContent(x + y * m_blockCountX);

    SubContent& inputSubContent = content.GetInputSubContent();

    if (color == 0)
    {
        inputSubContent.density0 += amount;
    }
    else if (color == 1)
    {
        inputSubContent.density1 += amount;
    }
    else if (color == 2)
    {
        inputSubContent.density2 += amount;
    }
}

FluidBox::Content& FluidBox::GetActiveBlockContent(int bIndex)
{
    return m_contentBuffer[m_activeContentBufferIndex][bIndex];
}

void FluidBox::SetContent(int x, int y, float amount0, float amount1, float amount2)
{
    Content& content = GetActiveBlockContent(x + y * m_blockCountX);

    SubContent& inputSubContent = content.GetInputSubContent();
    SubContent& outputSubContent = content.GetOutputSubContent();

    inputSubContent.N = 1;
    outputSubContent.N = 0;

    inputSubContent.density0 = amount0;
    outputSubContent.density0 = 0;

    inputSubContent.density1 = amount1;
    outputSubContent.density1 = 0;

    inputSubContent.density2 += amount2;
    outputSubContent.density2 = 0;

    content.UpdateCache();
}

void FluidBox::SetHorizontalVelocityAtLeft(int blockX, int blockY, float velocity)
{
    int velocityIndex = blockX + m_horizontalVelocityCountX * blockY;
    m_horizontalVelocityBuffer[m_activeVelocityBufferIndex][velocityIndex] = velocity;
}

void FluidBox::SetVerticalVelocityAtBottom(int blockX, int blockY, float velocity)
{
    int velocityIndex = blockX + m_verticalVelocityCountX * (blockY + 1);
    m_verticalVelocityBuffer[m_activeVelocityBufferIndex][velocityIndex] = velocity;
}


void FluidBox::AddHorizontalVelocityAtLeft(int blockX, int blockY, float velocity)
{
    int velocityIndex = blockX + m_horizontalVelocityCountX * blockY;
    m_horizontalVelocityBuffer[m_activeVelocityBufferIndex][velocityIndex] += velocity;
}

void FluidBox::Step(float dt, float diff, float visc)
{
    //memcpy(m_horizontalVelocityBuffer[m_inactiveVelocityBufferIndex], m_horizontalVelocityBuffer[m_activeVelocityBufferIndex], sizeof(float) * m_horizontalVelocityCount);
    //memcpy(m_verticalVelocityBuffer[m_inactiveVelocityBufferIndex], m_verticalVelocityBuffer[m_activeVelocityBufferIndex], sizeof(float) * m_verticalVelocityCount);
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

    ComputeViscosity(visc, dt);
    
    Project(p1);
    
    AdvectVelocity(dt);
 
    Project(p2);

    AdvectContent(dt);

    /*for (int i = 0; i < 3; i++)
    {
        float* color_s = tempDensity[i];
        float* color_density = density[i];

        memcpy(color_s, color_density, sizeof(float) * m_blockCount);
        Diffuse(0, color_s, color_density, diff, dt, m_diffuseMaxIter, m_diffuseQualityThresold);
        Advect(0, color_density, color_s, Vx, Vy, dt);
    }*/

}

void FluidBox::Diffuse(int b, float* x, float* x0, float diff, float dt, int maxIter, float qualityThresold)
{
    //float a = dt * diff * (m_blockCountX - 2) * (m_blockCountY - 2);
    float a = dt * diff / m_blockSize /** m_blockSize*/;
    //LinearSolve(b, x, x0, a, 1 + 4 * a, maxIter, qualityThresold);
}

void FluidBox::ComputeViscosity(float visc, float dt)
{
    float* horizontalVelocity = m_horizontalVelocityBuffer[m_activeVelocityBufferIndex];
    float* verticalVelocity = m_verticalVelocityBuffer[m_activeVelocityBufferIndex];

    float* targetHorizontalVelocity = m_horizontalVelocityBuffer[m_inactiveVelocityBufferIndex];
    float* targetVerticalVelocity = m_verticalVelocityBuffer[m_inactiveVelocityBufferIndex];

    // Copy velocity in target buffer
    memcpy(targetHorizontalVelocity, horizontalVelocity, sizeof(float) * m_horizontalVelocityCount);
    memcpy(targetVerticalVelocity, verticalVelocity, sizeof(float) * m_verticalVelocityCount);
    
    int horizontalVelocityCountX = m_horizontalVelocityCountX;
    int verticalVelocityCountX = m_verticalVelocityCountX;

    float a = dt * visc / m_blockSize /** m_blockSize*/;
    float c4 = 1 + 4 * a;
    float ooC4 = 1.f / c4;

    float c3 = 1 + 3 * a;
    float ooC3 = 1.f / c3;

    auto HIndex = [horizontalVelocityCountX](int hi, int hj) -> int
    {
        return (hi + horizontalVelocityCountX) % horizontalVelocityCountX + hj * horizontalVelocityCountX;
    };

    auto VIndex = [verticalVelocityCountX](int vi, int vj) -> int
    {
        return (vi + verticalVelocityCountX) % verticalVelocityCountX + vj * verticalVelocityCountX;
    };

    // linear solve for x
    for (int k = 0; k < m_viscosityMaxIter; k++) {
        float maxCorrection = 0;
        for (int j = 0; j < m_horizontalVelocityCountY; j++) {
            for (int i = 0; i < horizontalVelocityCountX; i++) {

                int vIndex = HIndex(i, j);

                VelocityType vType = (VelocityType) m_horizontalVelocityType[vIndex];
                if (vType == Velocity_ZERO)
                {
                    targetHorizontalVelocity[vIndex] = 0.f;
                }
                else if (j == 0)
                {
                    float newV = (horizontalVelocity[vIndex]
                        + a * (targetHorizontalVelocity[HIndex(i + 1, j)] + targetHorizontalVelocity[HIndex(i - 1, j)] + targetHorizontalVelocity[HIndex(i, j + 1)])
                        ) * ooC3;
                    float correction = abs(newV - targetHorizontalVelocity[vIndex]);
                    if (maxCorrection < correction)
                    {
                        maxCorrection = correction;
                    }
                    targetHorizontalVelocity[vIndex] = newV;
                }
                else if (j == m_horizontalVelocityCountY -1)
                {
                    float newV = (horizontalVelocity[vIndex]
                        + a * (targetHorizontalVelocity[HIndex(i + 1, j)] + targetHorizontalVelocity[HIndex(i - 1, j)] + targetHorizontalVelocity[HIndex(i, j - 1)])
                        ) * ooC3;
                    float correction = abs(newV - targetHorizontalVelocity[vIndex]);
                    if (maxCorrection < correction)
                    {
                        maxCorrection = correction;
                    }
                    targetHorizontalVelocity[vIndex] = newV;
                }
                else
                {
                    float newV = (horizontalVelocity[vIndex]
                        + a * (targetHorizontalVelocity[HIndex(i + 1, j)] + targetHorizontalVelocity[HIndex(i - 1, j)] + targetHorizontalVelocity[HIndex(i, j + 1)] + targetHorizontalVelocity[HIndex(i, j - 1)])
                        ) * ooC4;
                    float correction = abs(newV - targetHorizontalVelocity[vIndex]);
                    if (maxCorrection < correction)
                    {
                        maxCorrection = correction;
                    }
                    targetHorizontalVelocity[vIndex] = newV;
                }
            }
        }
        
        if (maxCorrection <= m_viscosityQualityThresold)
        {
            break;
        }
    }

    // linear solve for y
    for (int k = 0; k < m_viscosityMaxIter; k++) {
        float maxCorrection = 0;
        for (int j = 0; j < m_verticalVelocityCountY; j++) {
            for (int i = 0; i < m_verticalVelocityCountX; i++) {

                int vIndex = VIndex(i, j);

                VelocityType vType = (VelocityType)m_verticalVelocityType[vIndex];
                if (vType == Velocity_ZERO)
                {
                    targetVerticalVelocity[vIndex] = 0.f;
                }
                else
                {
                    float newV = (verticalVelocity[vIndex]
                        + a * (targetVerticalVelocity[VIndex(i + 1, j)] + targetVerticalVelocity[VIndex(i - 1, j)] + targetVerticalVelocity[VIndex(i, j + 1)] + targetVerticalVelocity[VIndex(i, j - 1)])
                        ) * ooC4;
                    float correction = abs(newV - targetVerticalVelocity[vIndex]);
                    if (maxCorrection < correction)
                    {
                        maxCorrection = correction;
                    }
                    targetVerticalVelocity[vIndex] = newV;
                }
            }
        }

        if (maxCorrection <= m_viscosityQualityThresold)
        {
            break;
        }
    }

    SwapVelocityBuffers();
}


void FluidBox::Project(float* p)
{
    float* horizontalVelocity = m_horizontalVelocityBuffer[m_activeVelocityBufferIndex];
    float* verticalVelocity = m_verticalVelocityBuffer[m_activeVelocityBufferIndex];
    float* div = m_divBuffer;

    int horizontalVelocityCountX = m_horizontalVelocityCountX;
    int verticalVelocityCountX = m_verticalVelocityCountX;

    auto BottomVerticalVelocity = [verticalVelocityCountX, verticalVelocity](int blockX, int blockY) -> float {
        int velocityIndex = blockX + verticalVelocityCountX * (blockY+1);
        return verticalVelocity[velocityIndex];
    };


    for (int blockY = 0; blockY < m_blockCountY; blockY++)
    {
        for (int blockX = 0; blockX < m_blockCountX; blockX++)
        {
            int blockIndex = BlockIndex(blockX, blockY);
            
            BlockConfig& blockConfig = m_blockConfig[blockIndex];

            float blockDiv = 0;

            if (blockConfig.isLeftConnected)
            {
                blockDiv += -horizontalVelocity[blockConfig.leftVelocityIndex];
            }
            if (blockConfig.isRightConnected)
            {
                blockDiv += horizontalVelocity[blockConfig.rightVelocityIndex];
            }
            if (blockConfig.isTopConnected)
            {
                blockDiv += -verticalVelocity[blockConfig.topVelocityIndex];
            }
            if (blockConfig.isBottomConnected)
            {
                blockDiv += verticalVelocity[blockConfig.bottomVelocityIndex];
            }

            div[blockIndex] = blockDiv;

  
        }
    }

    int k;
    for (k = 0; k < m_projectMaxIter; k++)
    {
        float maxCorrection = 0;
        for (int blockIndex = 0; blockIndex < m_blockCount; blockIndex++)
        {
            

            //uint8_t type = m_blockEdgeType[blockIndex];

            BlockConfig& blockConfig = m_blockConfig[blockIndex];

            if (blockConfig.connectionCount == 0)
            {
                continue;
            }

            float newPBase = div[blockIndex];

            if (blockConfig.isLeftConnected)
            {
                newPBase += p[blockConfig.leftBlockIndex];
            }
            if (blockConfig.isRightConnected)
            {
                newPBase += p[blockConfig.rightBlockIndex];
            }
            if (blockConfig.isTopConnected)
            {
                newPBase += p[blockConfig.topBlockIndex];
            }
            if (blockConfig.isBottomConnected)
            {
                newPBase += p[blockConfig.bottomBlockIndex];
            }

            float divisor;

            switch (blockConfig.connectionCount)
            {
            case 1:
                divisor = 1.f;
                break;
            case 2:
                divisor = 0.5f;
                break;
            case 3:
                divisor = 1.f/3.f;
                break;
            case 4:
                divisor = 0.25f;
                break;
            default:
                assert(false);
            }

            float newP = divisor * newPBase;



            float oldP = p[blockIndex];

            float newPWithSOR = Math::lerp(oldP, newP, 1.5f);

            float correction = abs(newPWithSOR - p[blockIndex]);
            if (maxCorrection < correction)
            {
                maxCorrection = correction;
            }
                    
            p[blockIndex] = newPWithSOR;

         
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

    for (int hIndex = 0; hIndex < m_horizontalVelocityCount; hIndex++)
    {
        uint8_t vxType = m_horizontalVelocityType[hIndex];
        if (hIndex == 24686)
        {
            int plop = 1;
        }
        switch (vxType)
        {
        case Velocity_ZERO:
            horizontalVelocity[hIndex] = 0;
            break;
        case Velocity_FREE:
            horizontalVelocity[hIndex] += p[RightBlockIdx(hIndex)] - p[LeftBlockIdx(hIndex)];
            break;
        case Velocity_LOOPING_LEFT:
            horizontalVelocity[hIndex] += p[RightBlockIdx(hIndex)] - p[LeftBlockIdx(hIndex) + m_blockCountX];
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
    int verticalVelocityCountX = m_verticalVelocityCountX;
    int horizontalVelocityCountX = m_horizontalVelocityCountX;

    int verticalVelocityCountY = m_verticalVelocityCountY;
    int horizontalVelocityCountY = m_horizontalVelocityCountY;

    float maxVelocity = 0;

    float* initialHorizontalVelocity = m_horizontalVelocityBuffer[m_activeVelocityBufferIndex];
    float* initialVerticalVelocity = m_verticalVelocityBuffer[m_activeVelocityBufferIndex];

    for (int vIndex = 0; vIndex < m_verticalVelocityCount; vIndex++)
    {
        float vy = abs(initialVerticalVelocity[vIndex]);
        if (vy > maxVelocity)
        {
            maxVelocity = vy;
        }
    }

    for (int hIndex = 0; hIndex < m_horizontalVelocityCount; hIndex++)
    {
        float vx = abs(initialHorizontalVelocity[hIndex]);
        if (vx > maxVelocity)
        {
            maxVelocity = vx;
        }
    }

    float maxDtPerSubStep = m_blockSize / maxVelocity;
    int subStepCount = int(ceilf(stepDt / maxDtPerSubStep));

    float dt = stepDt / subStepCount;


    auto HIndex = [horizontalVelocityCountX](int hi, int hj) -> int
    {
        return (hi + horizontalVelocityCountX) % horizontalVelocityCountX + hj * horizontalVelocityCountX;
    };

    auto VIndex = [verticalVelocityCountX](int vi, int vj) -> int
    {
        return (vi+ verticalVelocityCountX) % verticalVelocityCountX + vj * verticalVelocityCountX;
    };

    auto VIndexNeighbour = [verticalVelocityCountX, VIndex](int hi, int hj, int di, int dj) -> int {
        // Give vertical velocity neighbour relative to a i, j hVelocity 0 or 1
        int vi = hi + di - 1;
        int vj = hj + dj;
        return VIndex(vi, vj);
    };

    auto HIndexNeighbour = [horizontalVelocityCountX, HIndex](int vi, int vj, int di, int dj) -> int {
        // Give horizontal velocity neighbour relative to a i, j vVelocity 0 or 1
        int hi = vi + di;
        int hj = vj + dj - 1;
        return HIndex(hi, hj);
    };

    for (int k = 0; k < subStepCount; k++)
    {
        float* horizontalVelocity = m_horizontalVelocityBuffer[m_activeVelocityBufferIndex];
        float* verticalVelocity = m_verticalVelocityBuffer[m_activeVelocityBufferIndex];

        float* targetHorizontalVelocity = m_horizontalVelocityBuffer[m_inactiveVelocityBufferIndex];
        float* targetVerticalVelocity = m_verticalVelocityBuffer[m_inactiveVelocityBufferIndex];

        // Advect one substep
        for (int j = 0; j < m_horizontalVelocityCountY; j++)
        {
            for (int i = 0; i < m_horizontalVelocityCountX; i++)
            {
                int hIndex = i + j * m_horizontalVelocityCountX;

                uint8_t horizontalVelocityType = m_horizontalVelocityType[hIndex];

                if (horizontalVelocityType == Velocity_ZERO)
                {
                    targetHorizontalVelocity[hIndex] = 0;
                    continue;
                }
                else
                {
                    float vx = horizontalVelocity[hIndex];
                    float vy = 0.25f * (
                        verticalVelocity[VIndexNeighbour(i, j, 0,0)]
                        + verticalVelocity[VIndexNeighbour(i, j, 1, 0)]
                        + verticalVelocity[VIndexNeighbour(i, j, 0, 1)]
                        + verticalVelocity[VIndexNeighbour(i, j, 1, 1)]);

                    float dx = - vx * dt;
                    float dy = - vy * dt;

                    float x = i + dx * m_ooBlockSize;
                    float y = j + dy * m_ooBlockSize;
                                        
                    float i0 = floorf(x);
                    float i1 = i0 + 1.0f;
                    
                    float j0 = floorf(y);
                    float j1 = j0 + 1.0f;
                    
                    float s1 = x - i0;
                    float s0 = 1.0f - s1;
                    float t1 = y - j0;
                    float t0 = 1.0f - t1;
                    
                    int i0i = (int)i0;
                    int i1i = (int)i1;
                    int j0i = (int)j0;
                    int j1i = (int)j1;

                    if (j0i < 0)
                    {
                        j0i = 0;
                    }

                    if (j1i > horizontalVelocityCountY - 1)
                    {
                        j1i = horizontalVelocityCountY - 1;
                    }

                    assert(j1i >= 0);
                    assert(j0i < horizontalVelocityCountY);


                    int tlIndex = HIndex(i0i, j0i);
                    int trIndex = HIndex(i1i, j0i);
                    int blIndex = HIndex(i0i, j1i);
                    int brIndex = HIndex(i1i, j1i);
                    
                    targetHorizontalVelocity[hIndex] = s0 * (t0 * horizontalVelocity[tlIndex] + t1 * horizontalVelocity[blIndex])
                        + s1 * (t0 * horizontalVelocity[trIndex] + t1 * horizontalVelocity[brIndex]);
                }
            }
        }

        for (int j = 0; j < verticalVelocityCountY; j++)
        {
            for (int i = 0; i < verticalVelocityCountX; i++)
            {
                int vIndex = i + j * verticalVelocityCountX;

                uint8_t verticalVelocityType = m_verticalVelocityType[vIndex];

                if (verticalVelocityType == Velocity_ZERO)
                {
                    targetVerticalVelocity[vIndex] = 0;
                    continue;
                }
                else
                {
                    float vy = verticalVelocity[vIndex];
                    float vx = 0.25f * (
                        horizontalVelocity[HIndexNeighbour(i, j, 0, 0)]
                        + horizontalVelocity[HIndexNeighbour(i, j, 1, 0)]
                        + horizontalVelocity[HIndexNeighbour(i, j, 0, 1)]
                        + horizontalVelocity[HIndexNeighbour(i, j, 1, 1)]);

                    float dx = -vx * dt;
                    float dy = -vy * dt;

                    float x = i + dx * m_ooBlockSize;
                    float y = j + dy * m_ooBlockSize;

                    float i0 = floorf(x);
                    float i1 = i0 + 1.0f;

                    float j0 = floorf(y);
                    float j1 = j0 + 1.0f;

                    float s1 = x - i0;
                    float s0 = 1.0f - s1;
                    float t1 = y - j0;
                    float t0 = 1.0f - t1;

                    int i0i = (int)i0;
                    int i1i = (int)i1;
                    int j0i = (int)j0;
                    int j1i = (int)j1;

                    if (j0i < 0)
                    {
                        j0i = 0;
                    }

                    if (j1i > verticalVelocityCountY - 1)
                    {
                        j1i = verticalVelocityCountY - 1;
                    }

                    assert(j1i >= 0);
                    assert(j0i < verticalVelocityCountY);

                    int tlIndex = VIndex(i0i, j0i);
                    int trIndex = VIndex(i1i, j0i);
                    int blIndex = VIndex(i0i, j1i);
                    int brIndex = VIndex(i1i, j1i);

                    targetVerticalVelocity[vIndex] = s0 * (t0 * verticalVelocity[tlIndex] + t1 * verticalVelocity[blIndex])
                        + s1 * (t0 * verticalVelocity[trIndex] + t1 * verticalVelocity[brIndex]);
                }
            }

        }

        SwapVelocityBuffers();
    }
}

void FluidBox::SwapVelocityBuffers()
{
    std::swap(m_activeVelocityBufferIndex, m_inactiveVelocityBufferIndex);
}

void FluidBox::SwapContentBuffers()
{
    std::swap(m_activeContentBufferIndex, m_inactiveContentBufferIndex);

    Content* activeContentBuffer = m_contentBuffer[m_activeContentBufferIndex];

    // Compute cache
    for (int bIndex = 0; bIndex < m_blockCount; bIndex++)
    {
        Content &blockContent = activeContentBuffer[bIndex];
        blockContent.UpdateCache();    
    }
}

FluidBox::SubContent& FluidBox::Content::GetInputSubContent()
{
    return subContent[inputBufferId];
}

FluidBox::SubContent& FluidBox::Content::GetOutputSubContent()
{
    return subContent[(inputBufferId+1) & 0x1];
}

void FluidBox::Content::UpdateCache()
{
    if (requestSwapBuffer)
    {
        inputBufferId = (inputBufferId + 1) & 0x1;
        requestSwapBuffer = false;
    }

    SubContent& inputSubContent = subContent[inputBufferId];
    SubContent& outputSubContent = subContent[(inputBufferId + 1) & 0x1];

    float inputN = inputSubContent.N;
    float outputN = outputSubContent.N;

    totalContent.N = inputN + outputN;
    totalContent.density0 = inputSubContent.density0 + outputSubContent.density0;
    totalContent.density1 = inputSubContent.density1 + outputSubContent.density1;
    totalContent.density2 = inputSubContent.density2 + outputSubContent.density2;

    outputContentRatio = outputN / (inputN + outputN);
    assert(!isnan(outputContentRatio));
}



void FluidBox::AdvectContent(float stepDt)
{
    // Get max velocity
    int verticalVelocityCountX = m_verticalVelocityCountX;
    int horizontalVelocityCountX = m_horizontalVelocityCountX;

    int verticalVelocityCountY = m_verticalVelocityCountY;
    int horizontalVelocityCountY = m_horizontalVelocityCountY;

    float maxVelocity = 0;

    float* horizontalVelocity = m_horizontalVelocityBuffer[m_activeVelocityBufferIndex];
    float* verticalVelocity = m_verticalVelocityBuffer[m_activeVelocityBufferIndex];

    
    for (int vIndex = 0; vIndex < m_verticalVelocityCount; vIndex++)
    {
        float vy = abs(verticalVelocity[vIndex]);
        if (vy > maxVelocity)
        {
            maxVelocity = vy;
        }
    }

    for (int hIndex = 0; hIndex < m_horizontalVelocityCount; hIndex++)
    {
        float vx = abs(horizontalVelocity[hIndex]);
        if (vx > maxVelocity)
        {
            maxVelocity = vx;
        }
    }

    float maxDtPerSubStep = 0.5f * m_blockSize / maxVelocity; // TODO check exit velocity sum
    int subStepCount = int(ceilf(stepDt / maxDtPerSubStep));

    float dt = stepDt / subStepCount;

    
    int blockCountX = m_blockCountX;
    int blockCountY = m_blockCountY;
    float blockSection = m_blockSection;
    float blockVolume = m_blockVolume;
    float ooBlockVolume = 1.f / blockVolume;

    //auto BIndex = [blockCountX](int bi, int bj) -> int
    //{
    //    return (bi + blockCountX) % blockCountX + bj * blockCountX;
    //};

    auto HIndex = [horizontalVelocityCountX](int bi, int bj, int di) -> int
    {
        return (bi + di + horizontalVelocityCountX) % horizontalVelocityCountX + bj * horizontalVelocityCountX;
    };

    auto VIndex = [verticalVelocityCountX](int bi, int bj, int dj) -> int
    {
        return (bi + verticalVelocityCountX) % verticalVelocityCountX + (bj + dj) * verticalVelocityCountX;
    };


    for (int k = 0; k < subStepCount; k++)
    {
        Content* currentContent = m_contentBuffer[m_activeContentBufferIndex];
        Content* targetContent = m_contentBuffer[m_inactiveContentBufferIndex];

        memset(targetContent, 0, m_blockCount * sizeof(Content));

        for (int bIndex = 0; bIndex < m_blockCount; bIndex++)
        {

        //for (int j = 0; j < m_blockCountY; j++)
        //{
         //   for (int i = 0; i < m_blockCountX; i++)
            {
                //int bIndex = BIndex(i, j);
                Content& currentBlockcontent = currentContent[bIndex];
                BlockConfig& blockConfig = m_blockConfig[bIndex];

                int vIndexTop = blockConfig.topVelocityIndex;
                int vIndexBottom = blockConfig.bottomBlockIndex;
                int vIndexLeft = blockConfig.leftVelocityIndex;
                int vIndexRight = blockConfig.rightBlockIndex;


                /*int vIndexTop = VIndex(i, j, 0);
                int vIndexBottom = VIndex(i, j, 1);
                int vIndexLeft = HIndex(i, j, 0);
                int vIndexRight = HIndex(i, j, 1);*/

                float vTop = verticalVelocity[vIndexTop];
                float vBottom = verticalVelocity[vIndexBottom];
                float vLeft = horizontalVelocity[vIndexLeft];
                float vRight = horizontalVelocity[vIndexRight];

                //vTop = 0.f;
                //vBottom = 0.f;
                //vLeft = 0.f;
                //vRight = 40.f;

                bool divTop = vTop < 0;
                bool divBottom = vBottom > 0;
                bool divLeft = vLeft < 0;
                bool divRight = vRight > 0;


                float vDivTop = 0.f;
                float vDivBottom = 0.f;
                float vDivLeft = 0.f;
                float vDivRight = 0.f;


                if (bIndex == 6)
                {
                    int plop = 1;
                }

                int divergenceCount = 0;
                if(divTop)
                {
                    vDivTop = -vTop;
                    divergenceCount++;
                }

                if (divBottom)
                {
                    vDivBottom = vBottom;
                    divergenceCount++;
                }

                if (divLeft)
                {
                    vDivLeft = -vLeft;
                    divergenceCount++;
                }

                if (divRight)
                {
                    vDivRight = vRight;
                    divergenceCount++;
                }

                bool horizontalDivergence = divLeft && divRight;
                bool verticalDivergence = divTop && divBottom;

                int hNeighbourIndex;
                int vNeighbourIndex;
                int diagNeighbourIndex;
                float vy = 0.f;
                float vx = 0.f;

                auto IsDiagonalBlockAvailable = [&]() -> bool
                {
                    if (divTop)
                    {
                        vNeighbourIndex = blockConfig.topBlockIndex;
                        vy = vDivTop;
                        if (divLeft)
                        {
                            diagNeighbourIndex = blockConfig.topLeftBlockIndex;
                            hNeighbourIndex = blockConfig.leftBlockIndex;
                            vx = vDivLeft;
                        }
                        else
                        {
                            diagNeighbourIndex = blockConfig.topRightBlockIndex;
                            hNeighbourIndex = blockConfig.rightBlockIndex;
                            vx = vDivRight;
                        }
                    }
                    else
                    {
                        vNeighbourIndex = blockConfig.bottomBlockIndex;
                        vy = vDivBottom;

                        if (divLeft)
                        {
                            diagNeighbourIndex = blockConfig.bottomLeftBlockIndex;
                            hNeighbourIndex = blockConfig.leftBlockIndex;
                            vx = vDivLeft;
                        }
                        else
                        {
                            diagNeighbourIndex = blockConfig.bottomRightBlockIndex;
                            hNeighbourIndex = blockConfig.rightBlockIndex;
                            vx = vDivRight;
                        }
                    }

                    
                    if (diagNeighbourIndex < 0)
                    {
                        return false;
                    }

                  /*  int di = divTop ? -1 : 1;
                    int dj = divLeft ? -1 : 1;



                    int diagIndex = BIndex(i + di, j + dj);*/

                    //BlockEdgeType type = (BlockEdgeType ) m_blockEdgeType[diagIndex];

                    //return type == BlockEdge_FILL;
                    return !m_blockConfig[diagNeighbourIndex].isFill;
                };

                if (divergenceCount == 0)
                {
                    // 4 direction convergences : no movement
                    GiveContent(currentBlockcontent, targetContent[bIndex], 1.f, 0.f, true);
                }
                else if (divergenceCount == 2 && !horizontalDivergence && !verticalDivergence && IsDiagonalBlockAvailable())
                {
                    float dx = vx * dt;
                    float dy = vy * dt;

                    float di = dx * m_ooBlockSize;
                    float dj = dy * m_ooBlockSize;

                    float horizontalMovingRatio = di;
                    float verticalMovingRatio = dj;

                    float horizontalStayRatio = 1.f - horizontalMovingRatio;
                    float verticalStayRatio = 1.f - verticalMovingRatio;


                    //float i0 = floorf(x);
                    //float i1 = i0 + 1.0f;

                    //float j0 = floorf(y);
                    //float j1 = j0 + 1.0f;

                    //float s1 = x - i0;
                    //float s0 = 1.0f - s1;
                    //float t1 = y - j0;
                    //float t0 = 1.0f - t1;

                    //int i0i = (int)i0;
                    //int i1i = (int)i1;
                    //int j0i = (int)j0;
                    //int j1i = (int)j1;

                    //int tlBindex = BIndex(i0i, j0i);
                    //int trBindex = BIndex(i1i, j0i);
                    //int blBindex = BIndex(i0i, j1i);
                    //int brBindex = BIndex(i1i, j1i);

                    //float totalMovingRatio = 1.f
                    //    - (tlBindex == bIndex ? s0 * t0 : 0.f)
                    //    - (trBindex == bIndex ? s1 * t0 : 0.f)
                    //    - (blBindex == bIndex ? s0 * t1 : 0.f)
                    //    - (brBindex == bIndex ? s1 * t1 : 0.f);

                    
                    float stayRatio = horizontalStayRatio * verticalStayRatio;

                    float totalMovingRatio = 1.f - stayRatio;

                    GiveContent(currentBlockcontent, targetContent[bIndex], stayRatio, totalMovingRatio, true);
                    
                    if (totalMovingRatio > 0.f)
                    {
                        GiveContent(currentBlockcontent, targetContent[hNeighbourIndex], horizontalMovingRatio * verticalStayRatio, totalMovingRatio, false);
                        GiveContent(currentBlockcontent, targetContent[vNeighbourIndex], horizontalStayRatio * verticalMovingRatio, totalMovingRatio, false);
                        GiveContent(currentBlockcontent, targetContent[diagNeighbourIndex], horizontalMovingRatio * verticalMovingRatio, totalMovingRatio, false);
                    }
                    //GiveContent(currentBlockcontent, targetContent[tlBindex], s0 * t0, totalMovingRatio, tlBindex == bIndex);
                    //GiveContent(currentBlockcontent, targetContent[trBindex], s1 * t0, totalMovingRatio, trBindex == bIndex);
                    //GiveContent(currentBlockcontent, targetContent[blBindex], s0 * t1, totalMovingRatio, blBindex == bIndex);
                    //GiveContent(currentBlockcontent, targetContent[brBindex], s1 * t1, totalMovingRatio, brBindex == bIndex);
                }
                else
                {
                    // 4 direction potential independente divergence : give to 4 neigbourg if div

                   float velocitySum = vDivRight + vDivLeft + vDivBottom + vDivTop;
                   float movedVolume = velocitySum * dt * blockSection;

                   if (movedVolume > blockVolume)
                   {
                       assert(false);
                   }

                   float stayVolume = blockVolume - movedVolume;

                   float stayRatio = stayVolume * ooBlockVolume;
                   float totalMovingRatio = 1.f - stayVolume * ooBlockVolume;

                   GiveContent(currentBlockcontent, targetContent[bIndex], stayRatio, totalMovingRatio, true);

                   if (totalMovingRatio > 0.f)
                   {
                       if (divTop)
                       {
                           //assert(BIndex(i, j - 1) >= 0 && BIndex(i, j - 1) < m_blockCount);
                           GiveContent(currentBlockcontent, targetContent[blockConfig.topBlockIndex], vDivTop * dt * blockSection * ooBlockVolume, totalMovingRatio, false);
                       }
                       if (divBottom)
                       {
                           //assert(BIndex(i, j + 1) >= 0 && BIndex(i, j + 1) < m_blockCount);
                           GiveContent(currentBlockcontent, targetContent[blockConfig.bottomBlockIndex], vBottom * dt * blockSection * ooBlockVolume, totalMovingRatio, false);
                       }
                       if (divLeft)
                       {
                           //assert(BIndex(i - 1, j) >= 0 && BIndex(i - 1, j) < m_blockCount);
                           GiveContent(currentBlockcontent, targetContent[blockConfig.leftBlockIndex], vDivLeft * dt * blockSection * ooBlockVolume, totalMovingRatio, false);
                       }
                       if (divRight)
                       {
                           //assert(BIndex(i + 1, j) >= 0 && BIndex(i + 1, j) < m_blockCount);
                           GiveContent(currentBlockcontent, targetContent[blockConfig.rightBlockIndex], vDivRight * dt * blockSection * ooBlockVolume, totalMovingRatio, false);
                       }
                   }
                }
            }
        }

        SwapContentBuffers();
    }

}

void FluidBox::GiveSubContent(SubContent& sourceSubContent, SubContent& targetSubContent, float ratio)
{
    targetSubContent.N += sourceSubContent.N * ratio;
    targetSubContent.density0 += sourceSubContent.density0 * ratio;
    targetSubContent.density1 += sourceSubContent.density1 * ratio;
    targetSubContent.density2 += sourceSubContent.density2 * ratio;
    assert(!isnan(targetSubContent.N));
}

void FluidBox::GiveContent(Content& sourceContent, Content& targetContent, float ratio, float totalMovingRatio, bool stay)
{
    if (ratio == 0.f)
    {
        return;
    }

    if (stay)
    {
        float totalStayRatio = 1.f - totalMovingRatio;
        float inputContentRatio = 1.f - sourceContent.outputContentRatio;

        assert(std::abs(ratio - totalStayRatio) < 1e-5);

        if (inputContentRatio > totalStayRatio)
        {
            // Take all from input buffer to target input buffer
            float ratioToTakeFromInput = ratio / inputContentRatio;
            GiveSubContent(sourceContent.subContent[sourceContent.inputBufferId], targetContent.subContent[targetContent.inputBufferId], ratioToTakeFromInput);
        }
        else
        {
            // Copy input
            float inputRatio = 1.f;
            GiveSubContent(sourceContent.subContent[sourceContent.inputBufferId], targetContent.subContent[targetContent.inputBufferId], inputRatio);


            float alreadyTaken = inputContentRatio;
            float remainingToTake = ratio - alreadyTaken;
            if (remainingToTake != 0.f)
            {
                float outputRatio = remainingToTake / sourceContent.outputContentRatio;
                assert(!isnan(outputRatio));

                GiveSubContent(sourceContent.subContent[(sourceContent.inputBufferId + 1) & 0x1], targetContent.subContent[targetContent.inputBufferId], outputRatio);
            }
        }
    }
    else
    {
        if (sourceContent.outputContentRatio > totalMovingRatio)
        {
            // Take all from ouput buffer to target input buffer
            float ratioToTakeFromOutput = ratio / sourceContent.outputContentRatio;
            GiveSubContent(sourceContent.subContent[(sourceContent.inputBufferId + 1) & 0x1], targetContent.subContent[targetContent.inputBufferId],ratioToTakeFromOutput);
        }
        else
        {
            // Take a part of the the output
            float outputRatio = ratio / totalMovingRatio;
            targetContent.requestSwapBuffer = true;
            GiveSubContent(sourceContent.subContent[(sourceContent.inputBufferId + 1) & 0x1], targetContent.subContent[targetContent.inputBufferId], outputRatio);

            // Take the rest on the input

            float alreadyTaken = outputRatio * sourceContent.outputContentRatio;
            float remainingToTake = ratio - alreadyTaken;

            float inputRatio = remainingToTake / (1.f - sourceContent.outputContentRatio);
            GiveSubContent(sourceContent.subContent[sourceContent.inputBufferId], targetContent.subContent[targetContent.inputBufferId], inputRatio);
        }
    }
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

    Content* content = m_contentBuffer[m_activeContentBufferIndex];

    for (int bIndex = 0; bIndex < m_blockCount; bIndex++)
    {
        Content& blockContent = content[bIndex];

        SubContent& inputSubContent = blockContent.GetInputSubContent();
        SubContent& outputSubContent = blockContent.GetInputSubContent();

        inputSubContent.density0 *= keepRatio;
        inputSubContent.density1 *= keepRatio;
        inputSubContent.density2 *= keepRatio;
        outputSubContent.density0 *= keepRatio;
        outputSubContent.density1 *= keepRatio;
        outputSubContent.density2 *= keepRatio;
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
            BlockConfig& blockConfig = m_blockConfig[blockIndex];
            //uint8_t& type = m_blockEdgeType[blockIndex];



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


            blockConfig.leftVelocityIndex = LeftHorizontalVelocityIndex(blockX, blockY);
            blockConfig.rightVelocityIndex = RightHorizontalVelocityIndex(blockX, blockY);
            blockConfig.topVelocityIndex = TopVerticalVelocityIndex(blockX, blockY);
            blockConfig.bottomVelocityIndex = BottomVerticalVelocityIndex(blockX, blockY);

            
            if (blockX == 0)
            {
                blockConfig.leftBlockIndex = m_isHorizontalLoop ? blockIndex - 1 + m_blockCountX : -1;
                blockConfig.topLeftBlockIndex = m_isHorizontalLoop ? blockIndex - 1 : -1;
                blockConfig.bottomLeftBlockIndex = m_isHorizontalLoop ? blockIndex - 1 + m_blockCountX + m_blockCountX : -1;
            }
            else
            {
                blockConfig.leftBlockIndex = blockIndex - 1;
                blockConfig.topLeftBlockIndex = blockIndex - 1 - m_blockCountX;
                blockConfig.bottomLeftBlockIndex = blockIndex - 1 + m_blockCountX;
            }

            //bool isRightFilled;
            if (blockX == m_blockCountX - 1)
            {
                blockConfig.rightBlockIndex = m_isHorizontalLoop ? blockIndex + 1 - m_blockCountX : -1;
                blockConfig.topRightBlockIndex = m_isHorizontalLoop ? blockIndex + 1 - m_blockCountX - m_blockCountX : -1;
                blockConfig.bottomRightBlockIndex = m_isHorizontalLoop ? blockIndex + 1  : -1;
            }
            else
            {
                blockConfig.rightBlockIndex = blockIndex + 1;
                blockConfig.topRightBlockIndex = blockIndex + 1 - m_blockCountX;
                blockConfig.bottomRightBlockIndex = blockIndex + 1 + m_blockCountX;
            }

            if (blockY == 0)
            {
                blockConfig.topBlockIndex = -1;
                blockConfig.topLeftBlockIndex = -1;
                blockConfig.topRightBlockIndex = -1;
            }
            else
            {
                blockConfig.topBlockIndex = blockIndex - m_blockCountX;
            }

            if (blockY == m_blockCountY - 1)
            {
                blockConfig.bottomBlockIndex = -1;
                blockConfig.bottomLeftBlockIndex = -1;
                blockConfig.bottomRightBlockIndex = -1;
            }
            else
            {
                blockConfig.bottomBlockIndex = blockIndex + m_blockCountX;
            }


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


            //if (type == BlockEdge_FILL)
            if (blockConfig.isFill)
            {
                ConfigureVelocities(Velocity_ZERO, Velocity_ZERO, Velocity_ZERO, Velocity_ZERO);
                blockConfig.isLeftConnected = false;
                blockConfig.isRightConnected = false;
                blockConfig.isTopConnected = false;
                blockConfig.isBottomConnected = false;
                blockConfig.connectionCount = 0;

                continue;
            }

            

            
            blockConfig.isLeftConnected = blockConfig.leftBlockIndex >= 0 && !m_blockConfig[blockConfig.leftBlockIndex].isFill;
            blockConfig.isRightConnected = blockConfig.rightBlockIndex >= 0 && !m_blockConfig[blockConfig.rightBlockIndex].isFill;
            blockConfig.isTopConnected = blockConfig.topBlockIndex >= 0 && !m_blockConfig[blockConfig.topBlockIndex].isFill;
            blockConfig.isBottomConnected = blockConfig.bottomBlockIndex >= 0 && !m_blockConfig[blockConfig.bottomBlockIndex].isFill;
            
            blockConfig.connectionCount = blockConfig.isLeftConnected + blockConfig.isRightConnected+ blockConfig.isTopConnected + blockConfig.isBottomConnected;


            VelocityType topType = blockConfig.isTopConnected ? Velocity_FREE : Velocity_ZERO;
            VelocityType bottomType = blockConfig.isBottomConnected ? Velocity_FREE : Velocity_ZERO;
            VelocityType leftType = blockConfig.isLeftConnected ? Velocity_FREE : Velocity_ZERO;
            VelocityType rightType = blockConfig.isRightConnected ? Velocity_FREE : Velocity_ZERO;

            if (blockX == 0 && leftType == Velocity_FREE)
            {
                leftType = Velocity_LOOPING_LEFT;
            }
            else if (blockX == m_blockCountX - 1 && rightType == Velocity_FREE)
            {
                rightType =  Velocity_LOOPING_RIGHT;
            }

            ConfigureVelocities(topType, bottomType, leftType, rightType);        
        }
    }

}




}