#include "Camera.hpp"

Camera::Camera(int width, int height) : winWidth(width), winHeight(height) 
{
    Reset();

    // Calcul de la matrice de projection (Déplacé depuis ton Initialise)
    float nearClip = 0.1f;
    float farClip = 1000.f;
    float f = 1.0f / tan((90.f * M_PI / 180.0f) / 2.0f);
    float aspect = (float)winWidth / (float)winHeight;

    // Initialisation du tableau à 0
    float temp[16] =
    {
        f/aspect, 0, 0,                                        0,
        0,        f, 0,                                        0,
        0,        0, (farClip+nearClip)/(nearClip-farClip),   -1,
        0,        0, (2*nearClip*farClip)/(nearClip-farClip),  0
    };
    memcpy(mPerspective, temp, sizeof(mPerspective)); // ← write into the member
}


void Camera::Update() {
    float camY = radius * sin(theta);
    float camX = radius * cos(theta) * cos(phi);
    float camZ = radius * cos(theta) * sin(phi);
    
    LookAt(vec3{camX, camY, camZ}, vec3{0, 0, 0}, vec3{0, 1, 0}, mViewMatrix);
}

void Camera::OnScroll(double yOffset) {
    radius += (float)yOffset * 2.0f;
    if (radius < 2.0f) radius = 2.0f;
}

void Camera::OnMouseButton(int button, int action) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) isDragging = true;
        else if (action == GLFW_RELEASE) isDragging = false;
    }
}

void Camera::OnMouseMove(double xpos, double ypos) {
    if (isDragging) {
        float deltaX = (float)xpos - (winWidth / 2.0f);
        float deltaY = (float)ypos - (winHeight / 2.0f);

        phi += deltaX * 0.005f;
        theta += deltaY * 0.005f;

        float limitTheta = ((float)M_PI / 2.0f) - 0.01f; 
        if (theta > limitTheta) theta = limitTheta;
        if (theta < -limitTheta) theta = -limitTheta;
    }
}

void Camera::Reset() {
    theta = 0.0f;
    phi = 0.0f;
    radius = 20.f;
    isDragging = false;
}