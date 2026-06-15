#pragma once
#include "matrixHelper.hpp" // Pour utiliser ta fonction LookAt et vec3
#include <string.h>
#include <GLFW/glfw3.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Camera {
public:
    // Le constructeur initialise la caméra et calcule la matrice de projection
    Camera(int width, int height);
    float radius;
    float theta;
    float phi;

    // Met à jour la matrice de vue (à appeler dans Render)
    void Update();

    // Gestion des événements de la souris et du clavier
    void OnScroll(double yOffset);
    void OnMouseButton(int button, int action);
    void OnMouseMove(double xpos, double ypos);
    void Reset();

    // Accesseurs pour envoyer les matrices au Shader (ou à l'UBO plus tard)
    const float* GetViewMatrix() const { return mViewMatrix; }
    const float* GetProjectionMatrix() const { return mPerspective; }

private:
    bool isDragging;
    
    int winWidth;
    int winHeight;

    float mViewMatrix[16];
    float mPerspective[16];
};