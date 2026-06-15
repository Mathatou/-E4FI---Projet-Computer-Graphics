#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 TexCoords;

layout(std140) uniform CameraData {
    mat4 m_ViewMatrix;
    mat4 m_Perspective;
};

void main() {
    TexCoords = aPos;

    mat4 viewNoTranslation = mat4(mat3(m_ViewMatrix));
    vec4 pos = m_Perspective * viewNoTranslation * vec4(aPos, 1.0);

    gl_Position = pos.xyww; 
}