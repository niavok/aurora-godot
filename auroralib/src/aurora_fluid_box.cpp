#include "aurora_fluid_box.h"

#include <math.h>

namespace godot
{

FluidBox::FluidBox(int sizeX, int sizeY)
    : m_sizeX(sizeX)
    , m_sizeY(sizeY)
    , m_linearSolveIter(15)
{
    int area = m_sizeX * m_sizeY;

    for (int i = 0; i < 3; i++)
    {
        s[i] = new float[area];
        density[i] = new float[area];
    }

    Vx = new float[area];
    Vy = new float[area];

    Vx0 = new float[area];
    Vy0 = new float[area];


    for (int i = 0; i < area; i++)
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
	return x + y * m_sizeX;
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
    for (int i = 1; i < m_sizeX - 1; i++) {
        x[Index(i, 0)] = b == 2 ? -x[Index(i, 1)] : x[Index(i, 1)];
        x[Index(i, m_sizeY - 1)] = b == 2 ? -x[Index(i, m_sizeY - 2)] : x[Index(i, m_sizeY - 2)];
    }

    for (int j = 1; j < m_sizeY - 1; j++) {
        x[Index(0, j)] = b == 1 ? -x[Index(1, j)] : x[Index(1, j)];
        x[Index(m_sizeX - 1, j)] = b == 1 ? -x[Index(m_sizeX - 2, j)] : x[Index(m_sizeX - 2, j)];
    }


    x[Index(0, 0)] = 0.5f * (x[Index(1, 0)] + x[Index(0, 1)]);
    x[Index(0, m_sizeY - 1)] = 0.5f * (x[Index(1, m_sizeY - 1)] + x[Index(0, m_sizeY - 2)]);
    x[Index(m_sizeX - 1, 0)] = 0.5f * (x[Index(m_sizeX - 2, 0)] + x[Index(m_sizeX - 1, 1)]);
    x[Index(m_sizeX - 1, m_sizeY - 1)] = 0.5f * (x[Index(m_sizeX - 2, m_sizeY - 1)] + x[Index(m_sizeX - 1, m_sizeY - 2)]);
}

void FluidBox::LinearSolve(int b, float* x, float* x0, float a, float c)
{
    float cRecip = 1.0f / c;
    for (int k = 0; k < m_linearSolveIter; k++) {
        for (int j = 1; j < m_sizeY - 1; j++) {
            for (int i = 1; i < m_sizeX - 1; i++) {
                x[Index(i, j)] =
                    (x0[Index(i, j)]
                        + a * (x[Index(i + 1, j)] + x[Index(i - 1, j)] + x[Index(i, j + 1)] + x[Index(i, j - 1)])
                        ) * cRecip;
            }
        }

        SetBound(b, x);
    }
}

void FluidBox::Step(float dt, float diff, float visc)
{
    Diffuse(1, Vx0, Vx, visc, dt);
    Diffuse(2, Vy0, Vy, visc, dt);

    Project(Vx0, Vy0, Vx, Vy);

    Advect(1, Vx, Vx0, Vx0, Vy0, dt);
    Advect(2, Vy, Vy0, Vx0, Vy0, dt);

    Project(Vx, Vy, Vx0, Vy0);

    for (int i = 0; i < 3; i++)
    {
        float* color_s = s[i];
        float* color_density = density[i];
        Diffuse(0, color_s, color_density, diff, dt);
        Advect(0, color_density, color_s, Vx, Vy, dt);
    }
}

void FluidBox::Diffuse(int b, float* x, float* x0, float diff, float dt)
{
    float a = dt * diff * (m_sizeX - 2) * (m_sizeY - 2);
    LinearSolve(b, x, x0, a, 1 + 4 * a);
}

void FluidBox::Project(float* velocX, float* velocY, float* p, float* div)
{
    // If not stable, restore h scale from original code
    for (int j = 1; j < m_sizeY - 1; j++) {
        for (int i = 1; i < m_sizeX - 1; i++) {
            div[Index(i, j)] = -0.5f * (
                velocX[Index(i + 1, j)]
                - velocX[Index(i - 1, j)]
                + velocY[Index(i, j + 1)]
                - velocY[Index(i, j - 1)]
                );
            p[Index(i, j)] = 0;
        }
    }

    SetBound(0, div);
    SetBound(0, p);
    LinearSolve(0, p, div, 1, 4);

    for (int j = 1; j < m_sizeY - 1; j++) {
        for (int i = 1; i < m_sizeX - 1; i++) {
            velocX[Index(i, j)] -= 0.5f * (p[Index(i + 1, j)] - p[Index(i - 1, j)]);
            velocY[Index(i, j)] -= 0.5f * (p[Index(i, j + 1)] - p[Index(i, j - 1)]);
        }
    }

    SetBound(1, velocX);
    SetBound(2, velocY);
}

void FluidBox::Advect(int b, float* d, float* d0, float* velocX, float* velocY, float dt)
{
    float i0, i1, j0, j1;

    float dtx = dt * (m_sizeX - 2);
    float dty = dt * (m_sizeX - 2);

    float s0, s1, t0, t1;
    float tmp1, tmp2, x, y;

    float NfloatX = float(m_sizeX);
    float NfloatY = float(m_sizeY);
    float ifloat, jfloat;
    int i, j;

    for (j = 1, jfloat = 1; j < m_sizeY - 1; j++, jfloat++) {
        for (i = 1, ifloat = 1; i < m_sizeX - 1; i++, ifloat++) {
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
    SetBound(b, d);
}



}