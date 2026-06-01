#include "matrixHelper.hpp"
#include <iostream>
#include <cmath>
using namespace std;

void multMatrix(const float* A,const float* B, float* result)
{
    for(int i = 0; i < 16;i++)
    {
        int l = i % 4;
        int c = i / 4;
        float sum = 0;
        for(int k = 0; k < 4; k++)
        {
            sum += A[l+k*4]*B[k+c*4];
        }
        result[i] = sum;
    }
}

void normalize(vec3& v)
{
    float length = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    v.x /= length;
    v.y /= length;
    v.z /= length;
}

void cross(vec3 a, vec3 b, vec3& result)
{
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
}

float dot(vec3 a, vec3 b)
{
    float result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}

void LookAt(vec3 position, vec3 target, vec3 up, float* viewMatrix)
{
    vec3 forward = {
        -target.x + position.x,
        -target.y + position.y,
        -target.z + position.z
    };
    normalize(forward);
    
    vec3 right; 
    cross(up, forward,right);
    normalize(right);
    
    vec3 newUp;
    cross(forward, right,newUp);
    normalize(newUp);

    float dotProdPR = -dot(position, right);
    float dotProdPU = -dot(position, newUp);
    float dotProdPF = -dot(position, forward);
    // les X
    viewMatrix[0] = right.x;
    viewMatrix[1] = newUp.x;
    viewMatrix[2] = forward.x;
    viewMatrix[3] = 0.0f;

    // les Y
    viewMatrix[4] = right.y;
    viewMatrix[5] = newUp.y;
    viewMatrix[6] = forward.y;
    viewMatrix[7] = 0.0f;

    // les Z
    viewMatrix[8] = right.z;
    viewMatrix[9] = newUp.z;
    viewMatrix[10] = forward.z;
    viewMatrix[11] = 0.0f;

    // Translation
    viewMatrix[12] = dotProdPR;
    viewMatrix[13] = dotProdPU;
    viewMatrix[14] = dotProdPF;
    viewMatrix[15] = 1.0f;
    
}
