#pragma once
#include <vector>
struct vec3
{
    float x;
    float y;
    float z;
};
    
void multMatrix(const float* A,const float*B, float* result);
void LookAt(vec3 position, vec3 target, vec3 up, float* viewMatrix);
float dot(vec3 a, vec3 b);
void cross(vec3 a, vec3 b, vec3& result);
void normalize(vec3& v);
void MakeTRSMatrix(
    float tx, float ty, float tz,
    float rx, float ry, float rz,  // Euler angles
    float scale,
    float* out);