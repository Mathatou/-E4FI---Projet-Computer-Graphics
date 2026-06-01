#include "common/GLShader.h"
#include <cstddef>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include "DragonData.h"
#include "matrixHelper.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb-master/stb_image.h"
#pragma region Def des variables globales
GLShader g_BasicShader; 
const int WIN_W = 960*2;
const int WIN_H = 540*2;
GLuint VBO_dragon;
GLuint VAO_dragon;
GLuint IBO_dragon;
GLuint texID_dragon;
int loc_rotationy;
int loc_translate; 
int loc_diffuse;
int loc_specular;
int loc_shininess;
int loc_view;
GLfloat angleD = 0;
float mViewMatrix[16];
float radius = 20.f;
float theta;
float phi;
bool isDragging = false;
#pragma endregion

bool Initialise() 
{ 
    g_BasicShader.LoadVertexShader("myVS.vs");  
    g_BasicShader.LoadFragmentShader("myFS.fs"); 
    g_BasicShader.Create() ;
    
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

    float cx = cos(0.7f);
    float sx = sin(0.7f);

    const float mRotateX[16] = {
        1, 0, 0, 0,  // Col 1
        0, cx, sx, 0,  // Col 2
        0, -sx, cx, 0, // Col 3
        0, 0, 0, 1   // Col 4
    };
    
    float cy = cos(angleD);
    float sy = sin(angleD);
    
    const float mRotateY[16] = {
        cy, sy, 0, 0,  // Col 1
        -sy, cy, 0, 0,  // Col 2
        0, 0, 1, 0, // Col 3
        0, 0, 0, 1   // Col 4
    };

    float cz = cos(0.7f);
    float sz = sin(0.7f);
    const float mRotateZ[16] = 
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

    float nearClip = 0.1f;
    float farClip= 1000.f;
    float f=1/tan((90.f*M_PI/180)/2.0f); // Cotan(fov/2)
    float aspect = (float)WIN_W/WIN_H;
    float mPerspective[16]=
    {
        f/aspect,0,0,0,
        0,f,0,0,
        0,0,(farClip+nearClip)/(nearClip-farClip),-1,
        0,0,(2*nearClip*farClip)/(nearClip-farClip),0
    };

    float mTranslateRotateXYZ[16];
    float mWorldMatrix[16];
    multMatrix(mTranslate,mRotateXYZ,mTranslateRotateXYZ);
    multMatrix(mTranslateRotateXYZ,mScale,mWorldMatrix);

#pragma endregion
    
    glClearColor(0.5f, 0.5f, 0.5f, 1.f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
/// Dragon 
#pragma region Def Buffers Dragon
    glGenVertexArrays(1,&VAO_dragon);
    glGenBuffers(1, &VBO_dragon);
    glGenBuffers(1,&IBO_dragon);
    glGenTextures(1,&texID_dragon);

    glBindVertexArray(VAO_dragon);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_dragon);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_dragon);
    //Texture ex 3
    {
        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &texID_dragon);
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
    }
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(DragonVertices), DragonVertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(DragonIndices),DragonIndices, GL_STATIC_DRAW);

#pragma endregion
    
#pragma region Config
    {
        // glBufferData alloue et transfert sizeof(Vertex) octets issus du tableau triangle
        auto basicProgram = g_BasicShader.GetProgram(); 
        glUseProgram(basicProgram); 
        int stride = sizeof(float)*8;
        //Position : 
        {
            int loc_position = glGetAttribLocation(basicProgram, "a_position");
            glEnableVertexAttribArray(loc_position);
            glVertexAttribPointer(loc_position, 3, GL_FLOAT, false, stride, (void*)0);
        }
        //Normale
        {
            int loc_normal = glGetAttribLocation(basicProgram,"a_normal");
            glEnableVertexAttribArray(loc_normal);
            glVertexAttribPointer(loc_normal, 3, GL_FLOAT, false, stride,(void*)(sizeof(float)*3) );
        }
        //Texture
        {
            int loc_texCoor = glGetAttribLocation(basicProgram,"a_texCoord");
            glEnableVertexAttribArray(loc_texCoor);
            glVertexAttribPointer(loc_texCoor,2,GL_FLOAT,false,stride,(void*)(sizeof(float)*6));
        
        }
        //Effet sur les mats;
        {
            //Recuperation de la localisation des "effets" dans le shader
            // loc_translate = glGetUniformLocation(basicProgram,"m_Translate");
            // int loc_rotationx = glGetUniformLocation(basicProgram,"m_RotateX");
            // int loc_rotationz = glGetUniformLocation(basicProgram,"m_RotateZ");
            // loc_rotationy = glGetUniformLocation(basicProgram,"m_RotateY");
            // int loc_scale = glGetUniformLocation(basicProgram,"m_Scale");
            int loc_persp = glGetUniformLocation(basicProgram,"m_Perspective");
            int loc_sampler = glGetUniformLocation(basicProgram,"m_sampler");
            int loc_world = glGetUniformLocation(basicProgram,"m_WorldMatrix");
            loc_view = glGetUniformLocation(basicProgram,"m_ViewMatrix");            

            // Envoi des datas
            // glUniformMatrix4fv(loc_rotationx,1,GL_FALSE,mRotateX);
            // glUniformMatrix4fv(loc_rotationz,1,GL_FALSE,mRotateZ);
            // glUniformMatrix4fv(loc_scale,1,GL_FALSE,mScale);
            glUniformMatrix4fv(loc_persp,1,GL_FALSE,mPerspective);
            glUniformMatrix4fv(loc_world,1,GL_FALSE,mWorldMatrix);
            glUniform1i(loc_sampler,0);
        }

        //Couleurs diffuse et specular
        {
            loc_diffuse = glGetUniformLocation(basicProgram,"u_mat.diffuse");
            loc_specular = glGetUniformLocation(basicProgram,"u_mat.specular");
            loc_shininess = glGetUniformLocation(basicProgram,"u_mat.shininess");
            float shine = 1.0f;    
        }
    }

    glBindVertexArray(0);
    // je recommande de reinitialiser les etats a la fin pour eviter les effets de bord
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#pragma endregion

#ifdef WIN32 
    wglSwapIntervalEXT(1); 
#endif 
    return true;  
} 
 
void Terminate() { 
    g_BasicShader.Destroy(); 

    glDeleteBuffers(1, &IBO_dragon);
    glDeleteBuffers(1, &VBO_dragon);
    glDeleteVertexArrays(1,&VAO_dragon);
    glDeleteTextures(1,&texID_dragon);
}

void Render()
{ 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Dessin dragon
    glBindVertexArray(VAO_dragon);
    // mTranslate[14] = -20.f;
    
    //angle+=.01;
    // cy = cos(angle);
    // sy = sin(angle);
    
    // mRotateY[0] = cy;
    // mRotateY[1] = sy;
    // mRotateY[4] = -sy;
    // mRotateY[5] = cy;
    
    // Camera Orbitale
    {
        float camY = radius * sin(theta);
        float camX = radius * cos(theta) * cos(phi);
        float camZ = radius * cos(theta) * sin(phi);

        LookAt(vec3{camX, camY, camZ}, vec3{0, 0, 0}, vec3{0, 1, 0}, mViewMatrix);

        glUniformMatrix4fv(loc_view,1,GL_FALSE,mViewMatrix);    
        // Position / Target / Up

    }
    glUniform3f(loc_diffuse,   .1f, .8f, .4f);
    glUniform3f(loc_specular,  .3f, .3f, .3f);  
    glUniform1f(loc_shininess, 12.0f);


    glDrawElements(GL_TRIANGLES,
        45000,
        GL_UNSIGNED_SHORT,
        (void*)0);
}

void Display(GLFWwindow* window)
{
    Render();
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void scroll_callback(GLFWwindow* window, double xpos, double yOffset)
{
    radius += (float) yOffset * 2.0f;
    if(radius < 2.0f)
        radius = 2.0f;
}
void mouse_button_callback( GLFWwindow* window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        isDragging = true;
    }
    else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        isDragging = false;
    }
}
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(isDragging)
    {
        float deltaX = (float)xpos - WIN_W / 2.0f;
        float deltaY = (float)ypos - WIN_H / 2.0f;

        phi += deltaX * 0.005f; // Sensibilité de la rotation horizontale
        theta += deltaY * 0.005f; // Sensibilité de la rotation verticale
        // Limite pour éviter le retournement de la caméra 
        float limitTheta = ((float)M_PI / 2.0f) - 0.01f; 
        if(theta > limitTheta) theta = limitTheta;
        if(theta < -limitTheta) theta =-limitTheta;

        // Recentrer la souris
        glfwSetCursorPos(window, WIN_W / 2.0, WIN_H / 2.0);
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
        theta = 0.0f;
        phi = 0.0f;
        radius = 20.f;
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