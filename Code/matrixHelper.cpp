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
/// @brief Creates a transformation matrix from translation, rotation, and scale components
/// @param tx translation on X 
/// @param ty translation on Y
/// @param tz translation on Z
/// @param rx rotation on X
/// @param ry rotation on Y
/// @param rz rotation on Z
/// @param scale scale
/// @param out the new matrix
void MakeTRSMatrix(
    float tx, float ty, float tz,
    float rx, float ry, float rz,  // Euler angles
    float scale,
    float* out)
{
    float cx = cos(rx), sx = sin(rx);
    float cy = cos(ry), sy = sin(ry);
    float cz = cos(rz), sz = sin(rz);

    float mT[16]={
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        tx,   ty,   tz,   1.0f
    };

    float mRX[16]={
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, cx,   sx,   0.0f,
        0.0f, -sx,  cx,   0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    float mRY[16] = { 
        cy,sy,0,0, 
        -sy,cy,0,0, 
        0,0,1,0,    
        0,0,0,1 
    };
    float mRZ[16] = { 
        cz,0,-sz,0, 
        0,1,0,0,  
        sz,0,cz,0,   
        0,0,0,1 
    };
    float mS[16] = {
        scale,0,0,0,
        0,scale,0,0,
        0,0,scale,0,
        0,0,0,1
    };
    float tmp1[16],tmp2[16],tmp3[16];
    multMatrix(mRY,mRX,tmp1);
    multMatrix(tmp1,mRZ,tmp2); // RY * RX * RZ
    multMatrix(tmp2,mS,tmp3); // T * R
    multMatrix(mT,tmp3,out); // T * R * S 
}