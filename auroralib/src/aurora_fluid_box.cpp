#include "aurora_fluid_box.h"

#include <math.h>
#include <cassert>

namespace godot
{

FluidBox::FluidBox(int blockCountX, int blockCountY, float blockSize, bool isHorizontalLoop)
    : m_blockCountX(blockCountX)
    , m_blockCountY(blockCountY)
    , m_blockSize(blockSize)
    , m_diffuseMaxIter(100)
    , m_viscosityMaxIter(100)
    , m_projectMaxIter(200)
    , m_diffuseQualityThresold(1e-5)
    , m_viscosityQualityThresold(1e-5)
    , m_projectQualityThresold(0.1f)
    , m_isHorizontalLoop(isHorizontalLoop)
{
    if(isHorizontalLoop)
    {
        assert((m_blockCountX & (m_blockCountX - 1)) == 0); // Ensure sizeX is power of 2 if loop
        m_blockCountXMask = m_blockCountX - 1;
    }

    m_blockCount = m_blockCountX * m_blockCountY;

    for (int i = 0; i < 3; i++)
    {
        s[i] = new float[m_blockCount];
        density[i] = new float[m_blockCount];
    }

    Vx = new float[m_blockCount];
    Vy = new float[m_blockCount];

    Vx0 = new float[m_blockCount];
    Vy0 = new float[m_blockCount];

    p1 = new float[m_blockCount];
    p2 = new float[m_blockCount];


    for (int i = 0; i < m_blockCount; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            s[j][i] = 0.f;
            density[j][i] = 0.f;
        }

        Vx[i] = 0.f;
        Vy[i] = 0.f;

        Vx0[i] = 0.f;
        Vy0[i] = 0.f;

        p1[i] = 0.f;
        p2[i] = 0.f;
    }
}

FluidBox::~FluidBox()
{
    for (int i = 0; i < 3; i++)
    {
        delete[] s[i];
        delete[] density[i];
    }

    delete[] Vx;
    delete[] Vy;

    delete[] Vx0;
    delete[] Vy0;
}

int FluidBox::Index(int x, int y)
{
	return x + y * m_blockCountX;
}

int FluidBox::IndexLoop(int x, int y)
{
    return (x & m_blockCountXMask) + y * m_blockCountX;
}

void FluidBox::AddDensity(int x, int y, float amount, int color)
{
    density[color][Index(x, y)] += amount;
    s[color][Index(x, y)] += amount;
}

void FluidBox::AddVelocity(int x, int y, float amountX, float amountY)
{
    int index = Index(x, y);

    Vx[index] += amountX;
    Vy[index] += amountY;
    Vx0[index] += amountX;
    Vy0[index] += amountY;
}

void FluidBox::SetBound(int b, float* x)
{
    if (m_isHorizontalLoop)
    {
        for (int i = 0; i < m_blockCountX; i++) {
            x[Index(i, 0)] = b == 2 ? -x[Index(i, 1)] : x[Index(i, 1)];
            x[Index(i, m_blockCountY - 1)] = b == 2 ? -x[Index(i, m_blockCountY - 2)] : x[Index(i, m_blockCountY - 2)];
        }
    }
    else
    {
        for (int i = 1; i < m_blockCountX - 1; i++) {
            x[Index(i, 0)] = b == 2 ? -x[Index(i, 1)] : x[Index(i, 1)];
            x[Index(i, m_blockCountY - 1)] = b == 2 ? -x[Index(i, m_blockCountY - 2)] : x[Index(i, m_blockCountY - 2)];
        }
    
        for (int j = 1; j < m_blockCountY - 1; j++) {
            x[Index(0, j)] = b == 1 ? -x[Index(1, j)] : x[Index(1, j)];
            x[Index(m_blockCountX - 1, j)] = b == 1 ? -x[Index(m_blockCountX - 2, j)] : x[Index(m_blockCountX - 2, j)];
        }


        x[Index(0, 0)] = 0.5f * (x[Index(1, 0)] + x[Index(0, 1)]);
        x[Index(0, m_blockCountY - 1)] = 0.5f * (x[Index(1, m_blockCountY - 1)] + x[Index(0, m_blockCountY - 2)]);
        x[Index(m_blockCountX - 1, 0)] = 0.5f * (x[Index(m_blockCountX - 2, 0)] + x[Index(m_blockCountX - 1, 1)]);
        x[Index(m_blockCountX - 1, m_blockCountY - 1)] = 0.5f * (x[Index(m_blockCountX - 2, m_blockCountY - 1)] + x[Index(m_blockCountX - 1, m_blockCountY - 2)]);
    }
}

int FluidBox::LinearSolve(int b, float* x, float* x0, float a, float c, int maxIter, float qualityThresold)
{
    float cRecip = 1.0f / c;

    int k = 0;

    if (m_isHorizontalLoop)
    {
        for (k = 0; k < maxIter; k++) {
            float maxCorrection = 0;
            for (int j = 1; j < m_blockCountY - 1; j++) {
                for (int i = 0; i < m_blockCountX; i++) {
                    float newX = (x0[Index(i, j)]
                        + a * (x[IndexLoop(i + 1, j)] + x[IndexLoop(i - 1, j)] + x[Index(i, j + 1)] + x[Index(i, j - 1)])
                        ) * cRecip;
                    float correction = abs(newX - x[Index(i, j)]);
                    if (maxCorrection < correction)
                    {
                        maxCorrection = correction;
                    }
                    x[Index(i, j)] = newX;
                        
                }
            }

            SetBound(b, x);

            if(maxCorrection <= qualityThresold)
            {
                break;
            }
        }
    }
    else
    {
        for (k = 0; k < maxIter; k++) {
            float maxCorrection = 0;
            for (int j = 1; j < m_blockCountY - 1; j++) {
                for (int i = 1; i < m_blockCountX - 1; i++) {
                    float newX =
                        (x0[Index(i, j)]
                            + a * (x[Index(i + 1, j)] + x[Index(i - 1, j)] + x[Index(i, j + 1)] + x[Index(i, j - 1)])
                            ) * cRecip;
                    float correction = abs(newX - x[Index(i, j)]);
                    if (maxCorrection < correction)
                    {
                        maxCorrection = correction;
                    }
                    x[Index(i, j)] = newX;
                }
            }

            SetBound(b, x);


            if (maxCorrection <= qualityThresold)
            {
                break;
            }
        }
    }

    return k;
}

void FluidBox::Step(float dt, float diff, float visc)
{
    memcpy(Vx0, Vx, sizeof(float) * m_blockCount);
    memcpy(Vy0, Vy, sizeof(float) * m_blockCount);
    Diffuse(1, Vx0, Vx, visc, dt, m_viscosityMaxIter, m_diffuseQualityThresold);
    Diffuse(2, Vy0, Vy, visc, dt, m_viscosityMaxIter, m_diffuseQualityThresold);

    Project(Vx0, Vy0, p1, Vy);
    Advect(1, Vx, Vx0, Vx0, Vy0, dt);
    Advect(2, Vy, Vy0, Vx0, Vy0, dt);

    Project(Vx, Vy, p2, Vy0);

    for (int i = 0; i < 3; i++)
    {
        float* color_s = s[i];
        float* color_density = density[i];

        memcpy(color_s, color_density, sizeof(float) * m_blockCount);
        Diffuse(0, color_s, color_density, diff, dt, m_diffuseMaxIter, m_diffuseQualityThresold);
        Advect(0, color_density, color_s, Vx, Vy, dt);
    }
}

void FluidBox::Diffuse(int b, float* x, float* x0, float diff, float dt, int maxIter, float qualityThresold)
{
    //float a = dt * diff * (m_blockCountX - 2) * (m_blockCountY - 2);
    float a = dt * diff / m_blockSize /** m_blockSize*/;
    LinearSolve(b, x, x0, a, 1 + 4 * a, maxIter, qualityThresold);
}

void FluidBox::Project(float* velocX, float* velocY, float* p, float* div)
{
    // If not stable, restore h scale from original code

    if (!m_isHorizontalLoop)
    {
        for (int j = 1; j < m_blockCountY - 1; j++) {
            for (int i = 1; i < m_blockCountX - 1; i++) {
                float localDiv = -0.5f * (
                    velocX[Index(i + 1, j)]
                    - velocX[Index(i - 1, j)]
                    + velocY[Index(i, j + 1)]
                    - velocY[Index(i, j - 1)]
                    );
                 div[Index(i, j)] = localDiv;
            }
        }

        SetBound(0, div);
        SetBound(0, p);
        LinearSolve(0, p, div, 1, 4, m_projectMaxIter, m_projectQualityThresold);

        for (int j = 1; j < m_blockCountY - 1; j++) {
            for (int i = 1; i < m_blockCountX - 1; i++) {
                velocX[Index(i, j)] -= 0.5f * (p[Index(i + 1, j)] - p[Index(i - 1, j)]);
                velocY[Index(i, j)] -= 0.5f * (p[Index(i, j + 1)] - p[Index(i, j - 1)]);
            }
        }
    }
    else
    {
        for (int j = 1; j < m_blockCountY - 1; j++) {
            for (int i = 0; i < m_blockCountX; i++) {
                float localDiv = -0.5f * (
                    velocX[IndexLoop(i + 1, j)]
                    - velocX[IndexLoop(i - 1, j)]
                    + velocY[Index(i, j + 1)]
                    - velocY[Index(i, j - 1)]
                    );
                div[Index(i, j)] = localDiv;
            }
        }

        SetBound(0, div);
        SetBound(0, p);
        LinearSolve(0, p, div, 1, 4, m_projectMaxIter, m_projectQualityThresold);

        for (int j = 1; j < m_blockCountY - 1; j++) {
            for (int i = 0; i < m_blockCountX; i++) {
                velocX[Index(i, j)] -= 0.5f * (p[IndexLoop(i + 1, j)] - p[IndexLoop(i - 1, j)]);
                velocY[Index(i, j)] -= 0.5f * (p[Index(i, j + 1)] - p[Index(i, j - 1)]);
            }
        }
    }

    SetBound(1, velocX);
    SetBound(2, velocY);
}

void FluidBox::Advect(int b, float* d, float* d0, float* velocX, float* velocY, float dt)
{
    float i0, i1, j0, j1;

    float dtx = dt;
    float dty = dt;

    float s0, s1, t0, t1;
    float tmp1, tmp2, x, y;

    float NfloatX = float(m_blockCountX);
    float NfloatY = float(m_blockCountY);
    float ifloat, jfloat;
    int i, j;

    if (!m_isHorizontalLoop)
    {
        for (j = 1, jfloat = 1; j < m_blockCountY - 1; j++, jfloat++) {
            for (i = 1, ifloat = 1; i < m_blockCountX - 1; i++, ifloat++) {
                tmp1 = dtx * velocX[Index(i, j)];
                tmp2 = dty * velocY[Index(i, j)];
                x = ifloat - tmp1;
                y = jfloat - tmp2;

                if (x < 0.5f) x = 0.5f;
                if (x > NfloatX + 0.5f) x = NfloatX + 0.5f;
                i0 = floorf(x);
                i1 = i0 + 1.0f;
                if (y < 0.5f) y = 0.5f;
                if (y > NfloatY + 0.5f) y = NfloatY + 0.5f;
                j0 = floorf(y);
                j1 = j0 + 1.0f;

                s1 = x - i0;
                s0 = 1.0f - s1;
                t1 = y - j0;
                t0 = 1.0f - t1;

                int i0i = (int)i0;
                int i1i = (int)i1;
                int j0i = (int)j0;
                int j1i = (int)j1;

                d[Index(i, j)] =
                    s0 * (t0 * d0[Index(i0i, j0i)] + t1 * d0[Index(i0i, j1i)])
                    + s1 * (t0 * d0[Index(i1i, j0i)] + t1 * d0[Index(i1i, j1i)]);
            }
        }
    }
    else
    {
        for (j = 1, jfloat = 1; j < m_blockCountY - 1; j++, jfloat++) {
            for (i = 0, ifloat = 0; i < m_blockCountX; i++, ifloat++) {
                tmp1 = dtx * velocX[Index(i, j)];
                tmp2 = dty * velocY[Index(i, j)];
                x = ifloat - tmp1;
                y = jfloat - tmp2;

                //if (x < 0.5f) x = 0.5f;
                //if (x > NfloatX + 0.5f) x = NfloatX + 0.5f;
                i0 = floorf(x);
                i1 = i0 + 1.0f;
                if (y < 0.5f) y = 0.5f;
                if (y > NfloatY + 0.5f) y = NfloatY + 0.5f;
                j0 = floorf(y);
                j1 = j0 + 1.0f;

                s1 = x - i0;
                s0 = 1.0f - s1;
                t1 = y - j0;
                t0 = 1.0f - t1;

                int i0i = (int)i0;
                int i1i = (int)i1;
                int j0i = (int)j0;
                int j1i = (int)j1;

                d[Index(i, j)] =
                    s0 * (t0 * d0[IndexLoop(i0i, j0i)] + t1 * d0[IndexLoop(i0i, j1i)])
                    + s1 * (t0 * d0[IndexLoop(i1i, j0i)] + t1 * d0[IndexLoop(i1i, j1i)]);
            }
        }
    }

    SetBound(b, d);
}



}