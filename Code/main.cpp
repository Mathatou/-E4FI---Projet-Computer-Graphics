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
GLuint FBO;
GLuint fboTexture;
GLuint RBO;

GLuint quadVAO = 0;
GLuint quadVBO = 0;
GLShader g_PostProcessShader;


float mWorldMatrixDragon[16];
float mWorldMatrixKirby[16];

GLfloat angleD = 0;
#pragma endregion

bool Initialise() 
{ 
    // Loading basic shaders
    g_BasicShader.LoadVertexShader("myVS.vs");  
    g_BasicShader.LoadFragmentShader("myFS.fs"); 
    g_BasicShader.Create() ;

    // Loading 3D models and the cam
    modelKirby = new Model();
    modelDragon = new Model();
    mainCam = new Camera(WIN_W, WIN_H);
    // Loading the models from OBJs and .h file
    modelKirby->Load("../2_OBJs/kirby.obj");
    modelDragon->LoadFromData
    (
        DragonVertices, 
        sizeof(DragonVertices) / sizeof(float), 
        DragonIndices, 
        sizeof(DragonIndices) / sizeof(uint16_t)
    );
    /// Creating the world matrices for the 3D models
    /// Look at tooltip to see what are the parameters of the function
    MakeTRSMatrix(
        0.f, 0.f, 0.f, 
        0.5f, 0.f, 0.5f, 
        1.0f, mWorldMatrixDragon);
    MakeTRSMatrix(
        0.f, -15.f, 0.f, 
        -1.5f, 0.f, 0.5f, 
        1.0f, mWorldMatrixKirby);

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
#pragma endregion

#pragma region Config FBO
    {
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        glGenTextures(1, &fboTexture);
        glBindTexture(GL_TEXTURE_2D, fboTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIN_W, WIN_H, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);

        glGenRenderbuffers(1, &RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WIN_W, WIN_H);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "Error: Framebuffer is not complete!" << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
#pragma endregion

#pragma region Config Quad pour PostProcess
    {
        float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);

        g_PostProcessShader.LoadVertexShader("postProcessVS.vs");
        g_PostProcessShader.LoadFragmentShader("postProcessFS.fs");
        g_PostProcessShader.Create();
        
    }
#pragma endregion
    
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
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    g_PostProcessShader.Destroy();
}

void Render()
{ 
    // Ecrtiure dans le FBO
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

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
    // Lecture du FBO et affichage sur l'écran
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    
    glUseProgram(g_PostProcessShader.GetProgram());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fboTexture);
    glUniform1i(glGetUniformLocation(g_PostProcessShader.GetProgram(), "screenTexture"), 0);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
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
    glEnable(GL_CULL_FACE);
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