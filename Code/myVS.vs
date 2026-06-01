#version 120
attribute vec3 a_position;
attribute vec3 a_normal; 
attribute vec2 a_texCoord;

varying vec3 v_Normal;
varying vec3 v_Position;
varying vec2 v_texCoord;

uniform mat4 m_Perspective;

uniform mat4 m_WorldMatrix;
uniform mat4 m_ViewMatrix;


void main(void) 
{  
    vec4 positionMonde = m_WorldMatrix*vec4(a_position,1.0);
    gl_Position = m_Perspective*m_ViewMatrix*positionMonde;
    v_Position = vec3(positionMonde);
    v_Normal = mat3(m_WorldMatrix)*a_normal;  
    v_texCoord = a_texCoord;
}