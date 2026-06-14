#include "Model.hpp"
#include "Camera.hpp"
#include "DragonData.h"
#include "matrixHelper.hpp"
#include "common/GLShader.h"
#include <cstddef>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb-master/stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "libs/stb-master/tiny_obj_loader.h"


#pragma region Les modeles 3D affichés
Model* modelKirby  = nullptr;
Model* modelDragon = nullptr;
#pragma endregion


Camera* mainCam = nullptr;
GLuint uboCamera = 0;


#pragma region Def des variables globales
GLShader g_BasicShader; 
GLuint texID_dragon;
const int WIN_W = 960*2;
const int WIN_H = 540*2;
int loc_rotationy;
int loc_translate; 
int loc_diffuse;
int loc_specular;
int loc_shininess;
int loc_view;
int loc_hasTexture;
int loc_world;

float mWorldMatrixDragon[16];
float mWorldMatrixKirby[16];

GLfloat angleD = 0;
#pragma endregion

bool Initialise() 
{ 
    g_BasicShader.LoadVertexShader("myVS.vs");  
    g_BasicShader.LoadFragmentShader("myFS.fs"); 
    g_BasicShader.Create() ;
    modelKirby = new Model();
    modelDragon = new Model();
    mainCam = new Camera(WIN_W, WIN_H);
    modelKirby->Load("../kirby.obj");
    modelDragon->LoadFromData
    (
        DragonVertices, 
        sizeof(DragonVertices) / sizeof(float), 
        DragonIndices, 
        sizeof(DragonIndices) / sizeof(uint16_t)
    );
#pragma region Config Matrices 
    float tX = 0.f;
    float tY = 0.f;
    float tZ = -0.f;
    
    float mTranslate[16] = 
    {
        1.0f, 0.0f, 0.0f, 0.0f, 
        0.0f, 1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 1.0f, 0.0f, 
        tX,   tY,   tZ,   1.0f
    };

    float cx = cos(0.5f);
    float sx = sin(0.5f);

    float mRotateX[16] = {
        1, 0, 0, 0,  // Col 1
        0, cx, sx, 0,  // Col 2
        0, -sx, cx, 0, // Col 3
        0, 0, 0, 1   // Col 4
    };
    
    float cy = cos(angleD);
    float sy = sin(angleD);
    
    float mRotateY[16] = {
        cy, sy, 0, 0,  // Col 1
        -sy, cy, 0, 0,  // Col 2
        0, 0, 1, 0, // Col 3
        0, 0, 0, 1   // Col 4
    };

    float cz = cos(0.5f);
    float sz = sin(0.5f);
    float mRotateZ[16] = 
    {
        cz,0,-sz,0,
        0,1,0,0,
        sz,0,cz,0,
        0,0,0,1
    };
    float mRotateXY[16];
    float mRotateXYZ[16];
    multMatrix(mRotateY,mRotateX,mRotateXY);
    multMatrix(mRotateXY,mRotateZ,mRotateXYZ);

    float scale = 1;
    float mScale[16]=
    {
        scale,0,0,0,
        0,scale,0,0,
        0,0,scale,0,
        0,0,0,1
    };

    float mTranslateRotateXYZ[16];
    multMatrix(mTranslate,mRotateXYZ,mTranslateRotateXYZ);
    multMatrix(mTranslateRotateXYZ,mScale,mWorldMatrixDragon);
    
    mTranslate[13] = -15.0f;
    mTranslate[14] = -10.0f;
    mRotateX[5] = cos (1.5f);
    mRotateX[6] = sin (1.5f);
    mRotateX[9] = -sin(1.5f);
    mRotateX[10] = cos(1.5f);
    multMatrix(mTranslate,mRotateXYZ,mTranslateRotateXYZ);
    multMatrix(mTranslate,mScale,mWorldMatrixKirby);

#pragma endregion
    // glUniformMatrix4fv(glGetUniformLocation(g_BasicShader.GetProgram(),"m_Perspective"),1,GL_FALSE,mainCam->GetProjectionMatrix());
    glClearColor(0.5f, 0.5f, 0.5f, 1.f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
/// Dragon 
#pragma region Def Buffers Dragon
    //Texture ex 3
    {
        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1,&texID_dragon);
        glBindTexture(GL_TEXTURE_2D, texID_dragon);
        // Filtrage trilinéaire en minification et bilineaire en magnification
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Si rien n’est specifie pour GL_TEXTURE_WRAP_* c’est GL_REPEAT par defaut
        const char* filename = "./dragon.png";
        int w, h;
        stbi_set_flip_vertically_on_load(true);
        uint8_t *data = stbi_load(filename, &w, &h, nullptr, STBI_rgb_alpha);
        if (data) 
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
            std::cerr << "Failed to load texture: " << filename << std::endl;
    }
#pragma endregion
    
#pragma region Config
    {
        // glBufferData alloue et transfert sizeof(Vertex) octets issus du tableau triangle
        auto basicProgram = g_BasicShader.GetProgram(); 
        glUseProgram(basicProgram); 
        {
            //Recuperation de la localisation des "effets" dans le shader
            int loc_persp = glGetUniformLocation(basicProgram,"m_Perspective");
            int loc_sampler = glGetUniformLocation(basicProgram,"m_sampler");
            loc_world = glGetUniformLocation(basicProgram,"m_WorldMatrix");
            loc_view = glGetUniformLocation(basicProgram,"m_ViewMatrix");            
            loc_hasTexture = glGetUniformLocation(basicProgram,"u_hasTexture");
            loc_diffuse = glGetUniformLocation(basicProgram,"u_mat.diffuse");
            loc_specular = glGetUniformLocation(basicProgram,"u_mat.specular");
            loc_shininess = glGetUniformLocation(basicProgram,"u_mat.shininess");
            
            // Envoi des datas
            // glUniformMatrix4fv(loc_persp,1,GL_FALSE,mainCam->GetProjectionMatrix());
            glUniform1i(loc_sampler,0);    
        }
    }
#pragma endregion

#pragma region Config UBO Camera
    {
        glGenBuffers(1, &uboCamera);
        glBindBuffer(GL_UNIFORM_BUFFER, uboCamera);
        glBufferData(GL_UNIFORM_BUFFER, 2 * 64, NULL, GL_STATIC_DRAW);

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboCamera);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        auto basicProgram = g_BasicShader.GetProgram();
        GLuint blockIndex = glGetUniformBlockIndex(basicProgram, "CameraData");
        
        if(blockIndex != GL_INVALID_INDEX)
            glUniformBlockBinding(basicProgram, blockIndex, 0);
    }


#ifdef WIN32 
    wglSwapIntervalEXT(1); 
#endif 
    return true;  
} 
 
void Terminate() { 
    delete modelKirby;
    delete modelDragon;
    delete mainCam;
    glDeleteBuffers(1, &uboCamera);
    glDeleteTextures(1,&texID_dragon);
    g_BasicShader.Destroy();
}

void Render()
{ 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(g_BasicShader.GetProgram());
    mainCam->Update();
    glBindBuffer(GL_UNIFORM_BUFFER, uboCamera);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 64, mainCam->GetViewMatrix());
    glBufferSubData(GL_UNIFORM_BUFFER, 64, 64, mainCam->GetProjectionMatrix());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    //Dessin Kirby
    if(modelKirby)
    {
        glUniform1i(loc_hasTexture, 0);
        glUniform3fv(loc_diffuse, 1, modelKirby->material.diffuse);
        glUniform3fv(loc_specular, 1, modelKirby->material.specular);
        glUniform1f(loc_shininess, modelKirby->material.shininess);
        glUniform1i(loc_hasTexture, 0);
        glUniformMatrix4fv(loc_world, 1, GL_FALSE, mWorldMatrixKirby);
        modelKirby->Draw();
    }
    // Dessin Dragon
    if(modelDragon)
    {
        glUniform1i(loc_hasTexture, 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID_dragon);
        glUniform3fv(loc_specular, 1, modelDragon->material.specular);
        glUniform1f(loc_shininess, modelDragon->material.shininess);
        glUniform1i(loc_hasTexture, 1);
        glUniformMatrix4fv(loc_world, 1, GL_FALSE, mWorldMatrixDragon);
        modelDragon->Draw();
    }
}

void Display(GLFWwindow* window)
{
    Render();
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void scroll_callback(GLFWwindow* window, double xpos, double yOffset)
{
    if(mainCam) mainCam->OnScroll(yOffset);
}

void mouse_button_callback( GLFWwindow* window, int button, int action, int mods)
{
    if(mainCam) mainCam->OnMouseButton(button, action);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(mainCam)
    {
        mainCam->OnMouseMove(xpos, ypos);
        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            glfwSetCursorPos(window, WIN_W / 2.0, WIN_H / 2.0);
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // quitter avec echap
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // reset de la camera
    if(key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        if(mainCam)mainCam->Reset();
        
    }
}

int main(int argc, char **argv)
{
    GLFWwindow* window;

    if(!glfwInit())
        return -1;

    window = glfwCreateWindow(WIN_W, WIN_H, "Camera Orbitale", NULL, NULL);
    if(!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetScrollCallback(window,scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);
    GLenum error = glewInit();
    if(error != GLEW_OK)
    {
        std::cerr << "Error initializing GLEW: " << glewGetErrorString(error) << std::endl;
        return -1;
    }
    // glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    Initialise();
    
    while(!glfwWindowShouldClose(window))
    {
        Display(window);
    }
    Terminate();
    glfwTerminate();
    return 0;

}