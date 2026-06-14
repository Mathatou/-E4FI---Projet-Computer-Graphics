#version 330 core
layout(location=0) in vec3 a_position;
layout(location=1) in vec3 a_normal; 
layout(location=2) in vec2 a_texCoord;

// 'varying' devient 'out'
out vec3 v_Normal;
out vec3 v_Position;
out vec2 v_texCoord;

layout(std140) uniform CameraData
{
    mat4 m_ViewMatrix; // offset 0
    mat4 m_Perspective; // offset 64
};

uniform mat4 m_WorldMatrix;

void main(void) 
{  
    vec4 positionMonde = m_WorldMatrix * vec4(a_position, 1.0);
    gl_Position = m_Perspective * m_ViewMatrix * positionMonde;
    
    v_Position = vec3(positionMonde);
    v_Normal = mat3(m_WorldMatrix) * a_normal;  
    v_texCoord = a_texCoord;
}