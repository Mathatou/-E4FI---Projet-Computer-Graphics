#include "Model.hpp"
#include "Camera.hpp"
#include "DragonData.h"
#include "matrixHelper.hpp"
#include "common/GLShader.h"
#include "libs/imgui-master/imgui.h"
#include "libs/imgui-master/backends/imgui_impl_glfw.h"
#include "libs/imgui-master/backends/imgui_impl_opengl3.h"
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

GLFWwindow* window;

#pragma region Les modeles 3D affichés
Model* modelKirby  = nullptr;
Model* modelDragon = nullptr;
Model* modelMiles = nullptr;
Model* modelGamma = nullptr;
#pragma endregion

#pragma region Def des variables globales
const int WIN_W = 960*2;
const int WIN_H = 540*2;

GLuint uboCamera = 0;
Camera* mainCam = nullptr;
float limitTheta = ((float)M_PI / 2.0f) - 0.01f; 
GLShader g_BasicShader; 

GLuint texID_dragon;
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
bool hasTexture = true;
int currentEffect = 0;       // 0 = Normal, 1 = Inversion, 2 = Noir & Blanc, 3 = Flou
int loc_reflectivityPreset; // Pour stocker l'uniform du shader
int currentReflectivityPreset = 0; // 0 = Or, 1 = Eau, 2 = Argent
int loc_postProcessEffect;   // Pour stocker l'uniform du shader
GLuint quadVAO = 0;
GLuint quadVBO = 0;
GLShader g_PostProcessShader;
GLShader g_SkyboxShader;
GLuint skyboxVAO = 0;
GLuint skyboxVBO = 0;
GLuint skyboxTex = 0;


float mWorldMatrixDragon[16];
float mWorldMatrixKirby[16];
float mWorldMatrixMiles[16];
float mWorldMatrixGamma[16];
float mDragonPos[3] = {0.f, 0.f, 0.f};
float mKirbyPos[3] = {0.f, 0.f, 0.f};
float mMilesPos[3] = {-8.f, 0.f, -5.f};
float mGammaPos[3] = {8.f, 0.f, -5.f};
float mDragonScale = 1.0f;
float mKirbyScale  = 1.0f;
float mMilesScale  = 1.0f;
float mGammaScale  = 1.0f;

#pragma endregion

GLuint LoadSkybox(std::vector<std::string> faces) {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);
    int w, h, count;
    for(int i=0; i<faces.size(); i++) {
        stbi_set_flip_vertically_on_load(false); 
        unsigned char *data = stbi_load(faces[i].c_str(), &w, &h, &count, 0);
        GLenum format;
        if (count == 4) {
            format = GL_RGBA;
        } else {
            format = GL_RGB;
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return texID;
}

bool Initialise() 
{ 
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init("#version 330 core"); // GLSL version. If using GL ES 2.0, set to "#version 100"

    // Loading basic shaders
    g_BasicShader.LoadVertexShader("myVS.vs");  
    g_BasicShader.LoadFragmentShader("myFS.fs"); 
    g_BasicShader.Create() ;

    // Loading 3D models and the cam
    modelKirby = new Model();
    modelDragon = new Model();
    modelMiles = new Model();
    modelGamma = new Model();
    mainCam = new Camera(WIN_W, WIN_H);
    // Loading the models from OBJs and .h file
    modelKirby->Load("../2_OBJs/kirby.obj");
    modelMiles->Load("../2_OBJs/MilesMorales.obj");
    modelGamma->Load("../2_OBJs/gamma.obj");
    modelDragon->LoadFromData
    (   
        DragonVertices, 
        sizeof(DragonVertices) / sizeof(float), 
        DragonIndices, 
        sizeof(DragonIndices) / sizeof(uint16_t)
    );
    
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
            loc_reflectivityPreset = glGetUniformLocation(basicProgram,"u_reflectivityPreset");
            // Envoi des datas
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
		loc_postProcessEffect = glGetUniformLocation(g_PostProcessShader.GetProgram(), "u_effect");
        
    }
#pragma endregion

#pragma region Config Skybox
    {
        float skyboxVertices[] = {
            -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
        };

        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
        g_SkyboxShader.LoadVertexShader("skyboxVS.vs");
        g_SkyboxShader.LoadFragmentShader("skyboxFS.fs");
        g_SkyboxShader.Create();

        GLuint blockIdx = glGetUniformBlockIndex(g_SkyboxShader.GetProgram(), "CameraData");
        if(blockIdx != GL_INVALID_INDEX) {
            glUniformBlockBinding(g_SkyboxShader.GetProgram(), blockIdx, 0);
        }

        std::vector<std::string> faces = {
            "envmaps/right.png",
            "envmaps/left.png",
            "envmaps/top.png",
            "envmaps/bottom.png",
            "envmaps/front.png",
            "envmaps/back.png"
        };
        skyboxTex = LoadSkybox(faces);
    }
#pragma endregion
    
#ifdef WIN32 
    wglSwapIntervalEXT(1); 
#endif 
    return true;  
} 
 

void Render()
{ 
    // Ecrtiure dans le FBO
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glUseProgram(g_BasicShader.GetProgram());
    glUniform1i(loc_reflectivityPreset, currentReflectivityPreset);
    mainCam->Update();
    
    glBindBuffer(GL_UNIFORM_BUFFER, uboCamera);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 64, mainCam->GetViewMatrix());
    glBufferSubData(GL_UNIFORM_BUFFER, 64, 64, mainCam->GetProjectionMatrix());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /// Creating the world matrices for the 3D models
    /// Look at tooltip to see what are the parameters of the function
    MakeTRSMatrix(
        mDragonPos[0], mDragonPos[1], mDragonPos[2], 
        0.5f, 0.f, 0.5f, 
        mDragonScale, mWorldMatrixDragon);
    MakeTRSMatrix(
        mKirbyPos[0], mKirbyPos[1], mKirbyPos[2], 
        -1.5f, 0.f, 0.5f, 
        mKirbyScale, mWorldMatrixKirby);
    MakeTRSMatrix(
        mMilesPos[0], mMilesPos[1], mMilesPos[2], 
        0.f, 0.f, 0.f, 
        mMilesScale, mWorldMatrixMiles);
    MakeTRSMatrix(
        mGammaPos[0], mGammaPos[1], mGammaPos[2], 
        0.f, 0.f, 0.f, 
        mGammaScale, mWorldMatrixGamma);

    //Dessin Kirby
    if(modelKirby)
    {
        if (modelKirby->material.hasTexture) {
            glUniform1i(loc_hasTexture, 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, modelKirby->material.diffuseTexture);
        } 
        else {
            glUniform1i(loc_hasTexture, 0); 
        }
        glUniform3fv(loc_diffuse, 1, modelKirby->material.diffuse);
        glUniform3fv(loc_specular, 1, modelKirby->material.specular);
        glUniform1f(loc_shininess, modelKirby->material.shininess);
        glUniformMatrix4fv(loc_world, 1, GL_FALSE, mWorldMatrixKirby);
        modelKirby->Draw();
    }
    // Dessin Miles
    if(modelMiles)
    {
        if (modelMiles->material.hasTexture) {
            glUniform1i(loc_hasTexture, 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, modelMiles->material.diffuseTexture);
        } 
        else {
            glUniform1i(loc_hasTexture, 0); 
        }
        glUniform3fv(loc_diffuse, 1, modelMiles->material.diffuse);
        glUniform3fv(loc_specular, 1, modelMiles->material.specular);
        glUniform1f(loc_shininess, modelMiles->material.shininess);
        glUniformMatrix4fv(loc_world, 1, GL_FALSE, mWorldMatrixMiles);
        modelMiles->Draw();
    }
    // Dessin Gamma
    if(modelGamma)
    {
        if (modelGamma->material.hasTexture) {
            glUniform1i(loc_hasTexture, 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, modelGamma->material.diffuseTexture);
        } 
        else {
            glUniform1i(loc_hasTexture, 0); 
        }
        glUniform3fv(loc_diffuse, 1, modelGamma->material.diffuse);
        glUniform3fv(loc_specular, 1, modelGamma->material.specular);
        glUniform1f(loc_shininess, modelGamma->material.shininess);
        glUniformMatrix4fv(loc_world, 1, GL_FALSE, mWorldMatrixGamma);
        modelGamma->Draw();
    }
    // Dessin Dragon
    if(modelDragon)
    {
        glUniform1i(loc_hasTexture, hasTexture ? 1 : 0);     
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID_dragon);
        glUniform3fv(loc_diffuse, 1, modelDragon->material.diffuse);
        glUniform3fv(loc_specular, 1, modelDragon->material.specular);
        glUniform1f(loc_shininess, modelDragon->material.shininess);
        glUniformMatrix4fv(loc_world, 1, GL_FALSE, mWorldMatrixDragon);
        modelDragon->Draw();
    }
    // Dessin Skybox
    glDepthFunc(GL_LEQUAL); 
    glUseProgram(g_SkyboxShader.GetProgram());
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
    glUniform1i(glGetUniformLocation(g_SkyboxShader.GetProgram(), "skybox"), 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);

    // Lecture du FBO et affichage sur l'écran
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    
    glUseProgram(g_PostProcessShader.GetProgram());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fboTexture);
    glUniform1i(glGetUniformLocation(g_PostProcessShader.GetProgram(), "screenTexture"), 0);
    glUniform1i(loc_postProcessEffect, currentEffect);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
}

void Display(GLFWwindow* window)
{
    Render();
#pragma region ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Spacing();
    if(ImGui::CollapsingHeader("Kirby Material Settings"))
    {
        ImGui::Spacing();
        ImGui::Text("Ambient : ");
        ImGui::SliderFloat("kirby_ambient0", &modelKirby->material.ambient[0], 0.0f, 1.0f);
        ImGui::SliderFloat("kirby_ambient1", &modelKirby->material.ambient[1], 0.0f, 1.0f);
        ImGui::SliderFloat("kirby_ambient2", &modelKirby->material.ambient[2], 0.0f, 1.0f);
        ImGui::Text("Change Kirby Color : ");
        if(ImGui::BeginCombo("Kirby Color", "Select Color"))
        {
            if (ImGui::Selectable("Pink(ish)")) {
                modelKirby->material.diffuse[0] = 1.0f;
                modelKirby->material.diffuse[1] = 0.254f;
                modelKirby->material.diffuse[2] = 0.738f;
            }
            if(ImGui::Selectable("Red")) {
                modelKirby->material.diffuse[0] = 1.0f;
                modelKirby->material.diffuse[1] = 0.0f;
                modelKirby->material.diffuse[2] = 0.0f;
            }
            if(ImGui::Selectable("Green")) {
                modelKirby->material.diffuse[0] = 0.0f;
                modelKirby->material.diffuse[1] = 1.0f;
                modelKirby->material.diffuse[2] = 0.0f;
            }
            if(ImGui::Selectable("Blue")) {
                modelKirby->material.diffuse[0] = 0.0f;
                modelKirby->material.diffuse[1] = 0.0f;
                modelKirby->material.diffuse[2] = 1.0f;
            }
            ImGui::EndCombo();
        }
        ImGui::Spacing();
        ImGui::Text("Adjust Kirby Color : ");
        ImGui::SliderFloat("kirby_diffuse0", &modelKirby->material.diffuse[0], 0.0f, 1.0f);
        ImGui::SliderFloat("kirby_diffuse1", &modelKirby->material.diffuse[1], 0.0f, 1.0f);
        ImGui::SliderFloat("kirby_diffuse2", &modelKirby->material.diffuse[2], 0.0f, 1.0f);
        ImGui::Spacing();
        ImGui::Text("Adjust Kirby Shininess : ");
        ImGui::SliderFloat("kirby_shininess", &modelKirby->material.shininess, 1.0f, 128.0f);
        ImGui::Spacing();
        ImGui::Text("Adjust Kirby Specular : ");
        ImGui::SliderFloat("Kirby_specular0", &modelKirby->material.specular[0], 0.0f, 1.0f);
        ImGui::SliderFloat("Kirby_specular1", &modelKirby->material.specular[1], 0.0f, 1.0f);
        ImGui::SliderFloat("Kirby_specular2", &modelKirby->material.specular[2], 0.0f, 1.0f);
        ImGui::Spacing();
        
    }
    if(ImGui::CollapsingHeader("Kirby Position and Scale"))
    {
        ImGui::Spacing();
        ImGui::Text("Adjust Kirby Position : ");
        ImGui::SliderFloat("kirby_pos_x", &mKirbyPos[0], -10.0f, 10.0f);
        ImGui::SliderFloat("kirby_pos_y", &mKirbyPos[1], -10.0f, 10.0f);
        ImGui::SliderFloat("kirby_pos_z", &mKirbyPos[2], -10.0f, 10.0f);
        ImGui::Spacing();
        ImGui::Text("Adjust Kirby Scale : ");
        ImGui::SliderFloat("kirby_scale", &mKirbyScale, 0.1f, 5.0f);
    }
    if(ImGui::CollapsingHeader("Dragon Material Settings"))
    {
        ImGui::Spacing();
        ImGui::Text("Ambient : ");
        ImGui::SliderFloat("dragon_ambient0", &modelDragon->material.ambient[0], 0.0f, 1.0f);
        ImGui::SliderFloat("dragon_ambient1", &modelDragon->material.ambient[1], 0.0f, 1.0f);
        ImGui::SliderFloat("dragon_ambient2", &modelDragon->material.ambient[2], 0.0f, 1.0f);
        ImGui::Checkbox("Toggle Dragon Texture", &hasTexture);
        if(!hasTexture) 
        {
            ImGui::Text("Adjust Dragon Color : ");
            ImGui::SliderFloat("dragon_diffuse0", &modelDragon->material.diffuse[0], 0.0f, 1.0f);
            ImGui::SliderFloat("dragon_diffuse1", &modelDragon->material.diffuse[1], 0.0f, 1.0f);
            ImGui::SliderFloat("dragon_diffuse2", &modelDragon->material.diffuse[2], 0.0f, 1.0f);
        }
        ImGui::Spacing();
        ImGui::Text("Adjust Dragon Shininess : ");
        ImGui::SliderFloat("dragon_shininess", &modelDragon->material.shininess, 1.0f, 128.0f);
        ImGui::Spacing();
        ImGui::Text("Adjust Dragon Specular : ");
        ImGui::SliderFloat("dragon_specular0", &modelDragon->material.specular[0], 0.0f, 1.0f);
        ImGui::SliderFloat("dragon_specular1", &modelDragon->material.specular[1], 0.0f, 1.0f);
        ImGui::SliderFloat("dragon_specular2", &modelDragon->material.specular[2], 0.0f, 1.0f);
        ImGui::Spacing();
    }
    if(ImGui::CollapsingHeader("Dragon Position and Scale"))
    {
        ImGui::Spacing();
        ImGui::Text("Adjust Dragon Position : ");
        ImGui::SliderFloat("dragon_pos_x", &mDragonPos[0], -10.0f, 10.0f);
        ImGui::SliderFloat("dragon_pos_y", &mDragonPos[1], -10.0f, 10.0f);
        ImGui::SliderFloat("dragon_pos_z", &mDragonPos[2], -10.0f, 10.0f);
        ImGui::Spacing();
        ImGui::Text("Adjust Dragon Scale : ");
        ImGui::SliderFloat("dragon_scale", &mDragonScale, 0.1f, 5.0f);
    }
    if(ImGui::CollapsingHeader("Miles Material Settings"))
    {
        ImGui::Spacing();
        ImGui::Text("Ambient : ");
        ImGui::SliderFloat("Miles_ambient0", &modelMiles->material.ambient[0], 0.0f, 1.0f);
        ImGui::SliderFloat("Miles_ambient1", &modelMiles->material.ambient[1], 0.0f, 1.0f);
        ImGui::SliderFloat("Miles_ambient2", &modelMiles->material.ambient[2], 0.0f, 1.0f);
        ImGui::Spacing();
        ImGui::Text("Adjust Miles Color : ");
        ImGui::SliderFloat("Miles_diffuse0", &modelMiles->material.diffuse[0], 0.0f, 1.0f);
        ImGui::SliderFloat("Miles_diffuse1", &modelMiles->material.diffuse[1], 0.0f, 1.0f);
        ImGui::SliderFloat("Miles_diffuse2", &modelMiles->material.diffuse[2], 0.0f, 1.0f);
        ImGui::Spacing();
        ImGui::Text("Adjust Miles Shininess : ");
        ImGui::SliderFloat("Miles_shininess", &modelMiles->material.shininess, 1.0f, 128.0f);
        ImGui::Spacing();
        ImGui::Text("Adjust Miles Specular : ");
        ImGui::SliderFloat("Miles_specular0", &modelMiles->material.specular[0], 0.0f, 1.0f);
        ImGui::SliderFloat("Miles_specular1", &modelMiles->material.specular[1], 0.0f, 1.0f);
        ImGui::SliderFloat("Miles_specular2", &modelMiles->material.specular[2], 0.0f, 1.0f);
        ImGui::Spacing();
        
    }
    if(ImGui::CollapsingHeader("Miles Position and Scale"))
    {
        ImGui::Spacing();
        ImGui::Text("Adjust Miles Position : ");
        ImGui::SliderFloat("Miles_pos_x", &mMilesPos[0], -10.0f, 10.0f);
        ImGui::SliderFloat("Miles_pos_y", &mMilesPos[1], -10.0f, 10.0f);
        ImGui::SliderFloat("Miles_pos_z", &mMilesPos[2], -10.0f, 10.0f);
        ImGui::Spacing();
        ImGui::Text("Adjust Miles Scale : ");
        ImGui::SliderFloat("Miles_scale", &mMilesScale, 0.1f, 5.0f);
    }
    if(ImGui::CollapsingHeader("Gamma Material Settings"))
    {
        ImGui::Spacing();
        ImGui::Text("Ambient : ");
        ImGui::SliderFloat("Gamma_ambient0", &modelGamma->material.ambient[0], 0.0f, 1.0f);
        ImGui::SliderFloat("Gamma_ambient1", &modelGamma->material.ambient[1], 0.0f, 1.0f);
        ImGui::SliderFloat("Gamma_ambient2", &modelGamma->material.ambient[2], 0.0f, 1.0f);
        ImGui::Spacing();
        ImGui::Text("Adjust Gamma Color : ");
        ImGui::SliderFloat("Gamma_diffuse0", &modelGamma->material.diffuse[0], 0.0f, 1.0f);
        ImGui::SliderFloat("Gamma_diffuse1", &modelGamma->material.diffuse[1], 0.0f, 1.0f);
        ImGui::SliderFloat("Gamma_diffuse2", &modelGamma->material.diffuse[2], 0.0f, 1.0f);
        ImGui::Spacing();
        ImGui::Text("Adjust Gamma Shininess : ");
        ImGui::SliderFloat("Gamma_shininess", &modelGamma->material.shininess, 1.0f, 128.0f);
        ImGui::Spacing();
        ImGui::Text("Adjust Gamma Specular : ");
        ImGui::SliderFloat("Gamma_specular0", &modelGamma->material.specular[0], 0.0f, 1.0f);
        ImGui::SliderFloat("Gamma_specular1", &modelGamma->material.specular[1], 0.0f, 1.0f);
        ImGui::SliderFloat("Gamma_specular2", &modelGamma->material.specular[2], 0.0f, 1.0f);
        ImGui::Spacing();
        
    }
    if(ImGui::CollapsingHeader("Gamma Position and Scale"))
    {
        ImGui::Spacing();
        ImGui::Text("Adjust Gamma Position : ");
        ImGui::SliderFloat("Gamma_pos_x", &mGammaPos[0], -10.0f, 10.0f);
        ImGui::SliderFloat("Gamma_pos_y", &mGammaPos[1], -10.0f, 10.0f);
        ImGui::SliderFloat("Gamma_pos_z", &mGammaPos[2], -10.0f, 10.0f);
        ImGui::Spacing();
        ImGui::Text("Adjust Gamma Scale : ");
        ImGui::SliderFloat("Gamma_scale", &mGammaScale, 0.1f, 5.0f);
    }
    
    if(ImGui::CollapsingHeader("Camera Settings"))
    {
        ImGui::Spacing();
        ImGui::Text("Adjust Camera Radius : ");
        ImGui::SliderFloat("camera_radius", &mainCam->radius, 1.0f, 100.0f);
        ImGui::Text("Adjust Camera Theta : ");
        ImGui::SliderFloat("camera_theta", &mainCam->theta, -3.14f, 3.14f);
        ImGui::Spacing();
        if (mainCam->theta > limitTheta) mainCam->theta = limitTheta;
        if (mainCam->theta < -limitTheta) mainCam->theta = -limitTheta;
        ImGui::Spacing();
        ImGui::Text("Adjust Camera Phi : ");
        ImGui::SliderFloat("camera_phi", &mainCam->phi, -1.57f, 1.57f);
        ImGui::Spacing();
        ImGui::Button("Reset Camera");
        if(ImGui::IsItemClicked())
            mainCam->Reset();
    }
    
	/// Post-Process Effects
	{
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Text("Post-Process Effects");
	
		// Tableau contenant le nom de nos effets
		const char* effects[] = 
		{ 
			"Normal (Aucun)", 
			"Inversion des couleurs", 
			"Noir et Blanc", 
			"Flou (Box Blur)",
			"Sepia",
			"Noyau de Convolution",
            "Aberration Chromatique",
            "Pixellisation"
		};
	
		// Ce combo va modifier directement 'currentEffect' avec l'index de l'élément choisi (0, 1, 2 ou 3)
		if (ImGui::Combo("Choix de l'effet", &currentEffect, effects, IM_ARRAYSIZE(effects)))
		{
		}
	}
    // Reflectivity
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Valeurs de reflectivité");
        const char* materials[] = { "Par Défaut","Or", "Eau", "Argent", "Bronze" };
        
        // ImGui::Combo modifie currentReflectivityPreset de 0 à 4
        if (ImGui::Combo("Preset Matériau", &currentReflectivityPreset, materials, IM_ARRAYSIZE(materials)))
        {
        }        
    }
    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#pragma endregion
    glfwSwapBuffers(window);
    glfwPollEvents();
}
void Terminate() { 
    delete modelKirby;
    delete modelDragon;
    delete modelMiles;
    delete mainCam;
    glDeleteBuffers(1, &uboCamera);
    glDeleteTextures(1,&texID_dragon);
    g_BasicShader.Destroy();
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    g_PostProcessShader.Destroy();
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteTextures(1, &skyboxTex);
    g_SkyboxShader.Destroy();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void scroll_callback(GLFWwindow* window, double xpos, double yOffset)
{
    if(ImGui::GetIO().WantCaptureMouse) return; // Ignore mouse events if ImGui wants to capture them
    if(mainCam) mainCam->OnScroll(yOffset);
}

void mouse_button_callback( GLFWwindow* window, int button, int action, int mods)
{
    if(ImGui::GetIO().WantCaptureMouse) return; // Ignore mouse events if ImGui wants to capture them
    if(mainCam) mainCam->OnMouseButton(button, action);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(ImGui::GetIO().WantCaptureMouse) return; // Ignore mouse events if ImGui wants to capture them
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