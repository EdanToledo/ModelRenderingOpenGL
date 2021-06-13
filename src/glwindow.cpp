#include <iostream>
#include <stdio.h>

#include "SDL.h"
#include <GL/glew.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/vec3.hpp>                  // glm::vec3
#include <glm/vec4.hpp>                  // glm::vec4
#include <glm/mat4x4.hpp>                // glm::mat4
#include <glm/ext/matrix_transform.hpp>  // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp>  // glm::pi
#include <glm/gtx/transform.hpp>
#include "glwindow.h"
#include "geometry.h"

using namespace std;
glm::mat4 Rotation = glm::mat4(1);
glm::mat4 Translation = glm::mat4(1);
glm::mat4 Scaling = glm::mat4(1);
glm::mat4 Model;
glm::mat4 View;
glm::mat4 Projection;

glm::vec3 cameraPosition = glm::vec3(0, 0, 3);
glm::vec3 lightSource1 = glm::vec3(1, 0, 1);
glm::vec3 lightColor1 = glm::normalize(glm::vec3(64, 224, 208))+glm::vec3(0.2,0.2,0.2);
glm::vec3 lightSource2 = glm::vec3(-1, 0, 1);
glm::vec3 lightColor2 = glm::normalize(glm::vec3(255, 165, 0))+glm::vec3(0.2,0.2,0.2);
bool rotatingLights = false;
bool dragging = false;
bool rotating = false;
char axis = 'x';
bool rotate_on_world = true;

bool translating = false;
bool scaling = false;
bool duplicate = false;
bool colour = false;
const float winsizex = 1024;
const float winsizey = 1024;
float obj_x_size = 0;
float obj_vertices_count = 0;
float obj_vertices_count2 = 0;

const char *glGetErrorString(GLenum error)
{
    switch (error)
    {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return "UNRECOGNIZED";
    }
}

//Taken from learnopengl.com
GLuint loadTexture(char const *path)
{
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void glPrintError(const char *label = "Unlabelled Error Checkpoint", bool alwaysPrint = false)
{
    GLenum error = glGetError();
    if (alwaysPrint || (error != GL_NO_ERROR))
    {
        printf("%s: OpenGL error flag is %s\n", label, glGetErrorString(error));
    }
}

GLuint loadShader(const char *shaderFilename, GLenum shaderType)
{
    FILE *shaderFile = fopen(shaderFilename, "r");
    if (!shaderFile)
    {
        return 0;
    }

    fseek(shaderFile, 0, SEEK_END);
    long shaderSize = ftell(shaderFile);
    fseek(shaderFile, 0, SEEK_SET);

    char *shaderText = new char[shaderSize + 1];
    size_t readCount = fread(shaderText, 1, shaderSize, shaderFile);
    shaderText[readCount] = '\0';
    fclose(shaderFile);

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, (const char **)&shaderText, NULL);
    glCompileShader(shader);

    delete[] shaderText;

    return shader;
}

GLuint loadShaderProgram(const char *vertShaderFilename,
                         const char *fragShaderFilename)
{
    GLuint vertShader = loadShader(vertShaderFilename, GL_VERTEX_SHADER);
    GLuint fragShader = loadShader(fragShaderFilename, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE)
    {
        GLsizei logLength = 0;
        GLchar message[1024];
        glGetProgramInfoLog(program, 1024, &logLength, message);
        cout << "Shader load error: " << message << endl;
        return 0;
    }

    return program;
}

OpenGLWindow::OpenGLWindow()
{
}

GeometryData loadOBJFile(const string filename)
{
    GeometryData objData;

    objData.loadFromOBJFile(filename);

    return objData;
}

void OpenGLWindow::initGL()
{
    // We need to first specify what type of OpenGL context we need before we can create the window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    sdlWin = SDL_CreateWindow("OpenGL Prac 1",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              winsizex, winsizey, SDL_WINDOW_OPENGL);
    if (!sdlWin)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Error", "Unable to create window", 0);
    }
    SDL_GLContext glc = SDL_GL_CreateContext(sdlWin);
    SDL_GL_MakeCurrent(sdlWin, glc);
    SDL_GL_SetSwapInterval(1);

    glewExperimental = true;
    GLenum glewInitResult = glewInit();
    glGetError(); // Consume the error erroneously set by glewInit()
    if (glewInitResult != GLEW_OK)
    {
        const GLubyte *errorString = glewGetErrorString(glewInitResult);
        cout << "Unable to initialize glew: " << errorString;
    }

    int glMajorVersion;
    int glMinorVersion;
    glGetIntegerv(GL_MAJOR_VERSION, &glMajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &glMinorVersion);
    cout << "Loaded OpenGL " << glMajorVersion << "." << glMinorVersion << " with:" << endl;
    cout << "\tVendor: " << glGetString(GL_VENDOR) << endl;
    cout << "\tRenderer: " << glGetString(GL_RENDERER) << endl;
    cout << "\tVersion: " << glGetString(GL_VERSION) << endl;
    cout << "\tGLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0, 0, 0, 1);

    // Note that this path is relative to your working directory
    // when running the program (IE if you run from within build
    // then you need to place these files in build as well)
    shader = loadShaderProgram("simple.vert", "simple.frag");
    glUseProgram(shader);

    int colorLoc = glGetUniformLocation(shader, "objectColor");
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);

    // Load the model that we want to use and buffer the vertex attributes
    GeometryData geo = loadOBJFile("objects/suzanne.obj");
    obj_vertices_count = geo.vertexCount();
    obj_x_size = abs(geo.minx) + abs(geo.maxx);


    GeometryData geo2 = loadOBJFile("objects/suzanne.obj");
    obj_vertices_count2 = geo2.vertexCount();


    texture = loadTexture("marble.png");
    texture2 = loadTexture("bricks.jpg");
    // normalMap = loadTexture("NormalMap.png");
    // The projection matrix
    Projection = glm::perspective(glm::radians(45.0f), winsizex / winsizey, 0.1f, 100.0f);

    View = glm::lookAt(
        glm::vec3(0, 0, 3), // the camera's position
        glm::vec3(0, 0, 0), // the camera's target
        glm::vec3(0, 1, 0)  // the camera's upwards direction
    );
    //The model matrix
    Model = glm::mat4(1.0f);

    int vertexLoc = glGetAttribLocation(shader, "position");


    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, geo.vertexCount() * 3 * sizeof(float), geo.vertexData(), GL_STATIC_DRAW);

    glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(vertexLoc);

    glGenBuffers(1, &normalbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
    glBufferData(GL_ARRAY_BUFFER, geo.vertexCount() * 3 * sizeof(glm::vec3), geo.normalData(), GL_STATIC_DRAW);

    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,0,nullptr);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &texturebuffer);
    glBindBuffer(GL_ARRAY_BUFFER, texturebuffer);
    glBufferData(GL_ARRAY_BUFFER, geo.vertexCount() * 2 * sizeof(glm::vec2), geo.textureCoordData(), GL_STATIC_DRAW);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,0,nullptr);

    glEnableVertexAttribArray(2);

    glGenBuffers(1, &tangentbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, tangentbuffer);
    glBufferData(GL_ARRAY_BUFFER, geo.vertexCount() * 3 * sizeof(glm::vec3), geo.tangentData(), GL_STATIC_DRAW);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr );
    glEnableVertexAttribArray(3);

    glGenBuffers(1, &bitangentbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, bitangentbuffer);
    glBufferData(GL_ARRAY_BUFFER, geo.vertexCount() * 3 * sizeof(glm::vec3), geo.bitangentData(), GL_STATIC_DRAW);

    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    
    glEnableVertexAttribArray(4);

    //Second object

    glGenVertexArrays(1, &vao2);
    glBindVertexArray(vao2);
    glGenBuffers(1, &vertexBuffer2);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer2);
    glBufferData(GL_ARRAY_BUFFER, geo2.vertexCount() * 3 * sizeof(float), geo2.vertexData(), GL_STATIC_DRAW);

    glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(vertexLoc);


    glGenBuffers(1, &normalbuffer2);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer2);
    glBufferData(GL_ARRAY_BUFFER, geo2.vertexCount() * 3 * sizeof(glm::vec3), geo2.normalData(), GL_STATIC_DRAW);

    glVertexAttribPointer(
        1,        
        3,       
        GL_FLOAT, 
        GL_FALSE, 
        0,        
        nullptr
    );
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &texturebuffer2);
    glBindBuffer(GL_ARRAY_BUFFER, texturebuffer2);
    glBufferData(GL_ARRAY_BUFFER, geo2.vertexCount() * 2 * sizeof(glm::vec2), geo2.textureCoordData(), GL_STATIC_DRAW);

    glVertexAttribPointer(
        2,        
        2,       
        GL_FLOAT, 
        GL_FALSE, 
        0,       
        nullptr
    );
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &tangentbuffer2);
    glBindBuffer(GL_ARRAY_BUFFER, tangentbuffer);
    glBufferData(GL_ARRAY_BUFFER, geo.vertexCount() * 3 * sizeof(glm::vec3), geo.tangentData(), GL_STATIC_DRAW);

    glVertexAttribPointer(
        3,        
        3,       
        GL_FLOAT, 
        GL_FALSE,
        0,       
        nullptr
    );
    glEnableVertexAttribArray(3);

    glGenBuffers(1, &bitangentbuffer2);
    glBindBuffer(GL_ARRAY_BUFFER, bitangentbuffer);
    glBufferData(GL_ARRAY_BUFFER, geo.vertexCount() * 3 * sizeof(glm::vec3), geo.bitangentData(), GL_STATIC_DRAW);

    glVertexAttribPointer(
        4,       
        3,       
        GL_FLOAT, 
        GL_FALSE, 
        0,       
       nullptr
    );
    glEnableVertexAttribArray(4);

  

    GLuint lightSourceVector1 = glGetUniformLocation(shader, "lightSource1");
    glUniform3fv(lightSourceVector1, 1, &lightSource1[0]);
    GLuint lightSourceColor1 = glGetUniformLocation(shader, "lightColor1");
    glUniform3fv(lightSourceColor1, 1, &lightColor1[0]);

    GLuint lightSourceVector2 = glGetUniformLocation(shader, "lightSource2");
    glUniform3fv(lightSourceVector2, 1, &lightSource2[0]);
    GLuint lightSourceColor2 = glGetUniformLocation(shader, "lightColor2");
    glUniform3fv(lightSourceColor2, 1, &lightColor2[0]);

    GLuint viewPosID = glGetUniformLocation(shader, "viewPos");
    glUniform3fv(viewPosID, 1, &cameraPosition[0]);
    //********************
    //Get ModelViewProjection matrix uniform variable
    GLuint Model_Matrix = glGetUniformLocation(shader, "Model");
    glUniformMatrix4fv(Model_Matrix, 1, GL_FALSE, &Model[0][0]);
    GLuint View_Matrix = glGetUniformLocation(shader, "View");

    glUniformMatrix4fv(View_Matrix, 1, GL_FALSE, &View[0][0]);
    GLuint Projection_Matrix = glGetUniformLocation(shader, "Projection");

    glUniformMatrix4fv(Projection_Matrix, 1, GL_FALSE, &Projection[0][0]);

    glPrintError("Setup complete", true);
}
//Following methods are essentially just wrappers so i dont have to type glm every time
glm::mat4 translate(float x, float y, float z)
{
    return glm::translate(glm::vec3(x, y, z));
}
glm::mat4 rotate(const float radians_to_rotate, float xaxis, float yaxis, float zaxis)
{
    return glm::rotate(glm::radians(radians_to_rotate), glm::vec3(xaxis, yaxis, zaxis));
}
glm::mat4 scale(float size)
{
    return glm::scale(glm::vec3(size));
}

void OpenGLWindow::render()
{

    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, texture);

    GLuint lightSourceVector1 = glGetUniformLocation(shader, "lightSource1");
    glUniform3fv(lightSourceVector1, 1, &lightSource1[0]);
    GLuint lightSourceColor1 = glGetUniformLocation(shader, "lightColor1");
    glUniform3fv(lightSourceColor1, 1, &lightColor1[0]);

    GLuint lightSourceVector2 = glGetUniformLocation(shader, "lightSource2");
    glUniform3fv(lightSourceVector2, 1, &lightSource2[0]);
    GLuint lightSourceColor2 = glGetUniformLocation(shader, "lightColor2");
    glUniform3fv(lightSourceColor2, 1, &lightColor2[0]);

    GLuint cameraPosIndex = glGetUniformLocation(shader, "viewPos");
    glUniform3fv(cameraPosIndex, 1, &cameraPosition[0]);


    // Check if colour mode is on to change colours

    if (colour)
    {
        float red = sin(SDL_GetTicks());
        float green = cos(SDL_GetTicks());
        float blue = tan(SDL_GetTicks());

        int colorLoc = glGetUniformLocation(shader, "objectColor");
        glUniform3f(colorLoc, red, green, blue);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  GLuint Model_Matrix = glGetUniformLocation(shader, "Model");
  GLuint View_Matrix = glGetUniformLocation(shader, "View");
    glUniformMatrix4fv(View_Matrix, 1, GL_FALSE, &View[0][0]);
    GLuint Projection_Matrix = glGetUniformLocation(shader, "Projection");
    glUniformMatrix4fv(Projection_Matrix, 1, GL_FALSE, &Projection[0][0]);
  
    if (rotate_on_world)
    {
      
    glUniformMatrix4fv(Model_Matrix, 1, GL_FALSE, &(Model)[0][0]);
    
 
    }
    else
    {
      glUniformMatrix4fv(Model_Matrix, 1, GL_FALSE, &(Translation * glm::inverse(Rotation) * Scaling * Model)[0][0]);
      }

    glDrawArrays(GL_TRIANGLES, 0, obj_vertices_count);

    //draws duplicate if it should be on screen
    if (duplicate)
    {

        glBindVertexArray(vao2);
        glBindTexture(GL_TEXTURE_2D, texture2);
        
        
        if (rotate_on_world)
        {
            glUniformMatrix4fv(Model_Matrix, 1, GL_FALSE, &(glm::translate(Model, glm::vec3(obj_x_size, 0, 0)))[0][0]);
        }
        else
        {
         glUniformMatrix4fv(Model_Matrix, 1, GL_FALSE, &(Translation * glm::inverse(Rotation) * Scaling * glm::translate(Model, glm::vec3(obj_x_size, 0, 0)))[0][0]);

        }
        // draw second object

        glDrawArrays(GL_TRIANGLES, 0, obj_vertices_count2);
    }

    // Swap the front and back buffers on the window, effectively putting what we just "drew"
    // onto the screen (whereas previously it only existed in memory)
    SDL_GL_SwapWindow(sdlWin);
}

// The program will exit if this function returns false
bool OpenGLWindow::handleEvent(SDL_Event e)
{

    // A list of keycode constants is available here: https://wiki.libsdl.org/SDL_Keycode
    // Note that SDL provides both Scancodes (which correspond to physical positions on the keyboard)
    // and Keycodes (which correspond to symbols on the keyboard, and might differ across layouts)
    if (e.type == SDL_KEYDOWN)
    {
        if (e.key.keysym.sym == SDLK_a)
        {
            if (rotatingLights)
            {
                rotatingLights = false;
            }
            else
            {
                rotatingLights = true;
            }
        }

        if (e.key.keysym.sym == SDLK_r)
        {
            rotating = true;
            if (axis == 'x')
            {
                axis = 'y';
            }
            else if (axis == 'y')
            {
                axis = 'z';
            }
            else
            {
                axis = 'x';
            }

            translating = false;
            scaling = false;
        }
        if (e.key.keysym.sym == SDLK_q)
        {
            if (rotate_on_world)
            {
                Rotation = glm::mat4(1);
                Translation = glm::mat4(1);
                Scaling = glm::mat4(1);
                Model = glm::mat4(1);
                rotate_on_world = false;
            }
            else
            {
                rotate_on_world = true;
            }
        }
        // Translation switch
        if (e.key.keysym.sym == SDLK_t)
        {
            translating = true;
            rotating = false;
            scaling = false;
        }
        // Scaling Switch
        if (e.key.keysym.sym == SDLK_s)
        {
            scaling = true;
            rotating = false;
            translating = false;
        }

        if (e.key.keysym.sym == SDLK_ESCAPE)
        {
            return false;
        }


        if (e.key.keysym.sym == SDLK_g)
        {
            lightColor1 += glm::vec3(0.5, 0.5, 0.5);
        }

        // Delete duplicate switch

        if (e.key.keysym.sym == SDLK_k)
        {
            duplicate = false;
        }
        // Colour Switch
        if (e.key.keysym.sym == SDLK_c)
        {
            if (colour)
            {
                colour = false;
            }
            else
            {
                colour = true;
            }
        }

        if (e.key.keysym.sym == SDLK_UP)
        {
            glm::vec3 cameraDirection = glm::normalize(cameraPosition - glm::vec3(0, 0, 0));
            glm::vec3 cameraRight = glm::normalize(glm::cross(glm::vec3(0, 1, 0), cameraDirection));
            glm::vec4 newpos = glm::rotate(glm::mat4(1), glm::radians(-10.0f), cameraRight) * glm::vec4((cameraPosition - glm::vec3(0, 0, 0)) + glm::vec3(0, 0, 0), 1);
            cameraPosition.x = newpos.x;
            cameraPosition.y = newpos.y;
            cameraPosition.z = newpos.z;
            View = glm::lookAt(cameraPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        }
        if (e.key.keysym.sym == SDLK_DOWN)
        {
            glm::vec3 cameraDirection = glm::normalize(cameraPosition - glm::vec3(0, 0, 0));
            glm::vec3 cameraRight = glm::normalize(glm::cross(glm::vec3(0, 1, 0), cameraDirection));
            glm::vec4 newpos = glm::rotate(glm::mat4(1), glm::radians(10.0f), cameraRight) * glm::vec4((cameraPosition - glm::vec3(0, 0, 0)) + glm::vec3(0, 0, 0), 1);
            cameraPosition.x = newpos.x;
            cameraPosition.y = newpos.y;
            cameraPosition.z = newpos.z;
            View = glm::lookAt(cameraPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        }
        if (e.key.keysym.sym == SDLK_LEFT)
        {
            glm::vec4 newpos = glm::rotate(glm::mat4(1),glm::radians(-10.0f), glm::vec3(0, 1, 0)) * glm::vec4((cameraPosition - glm::vec3(0, 0, 0)) + glm::vec3(0, 0, 0), 1);
            cameraPosition.x = newpos.x;
            cameraPosition.y = newpos.y;
            cameraPosition.z = newpos.z;
            View = glm::lookAt(cameraPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        }
        if (e.key.keysym.sym == SDLK_RIGHT)
        {
            glm::vec4 newpos = glm::rotate(glm::mat4(1), glm::radians(10.0f),glm::vec3( 0, 1, 0)) * glm::vec4((cameraPosition - glm::vec3(0, 0, 0)) + glm::vec3(0, 0, 0), 1);
            cameraPosition.x = newpos.x;
            cameraPosition.y = newpos.y;
            cameraPosition.z = newpos.z;
            View = glm::lookAt(cameraPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        }
        if (e.key.keysym.sym == SDLK_z)
        {
            lightSource2 = lightSource2 + glm::vec3(0, 0.5, 0);
        }
        if (e.key.keysym.sym == SDLK_x)
        {
            lightSource2 = lightSource2 + glm::vec3(0, -0.5, 0);
        }
        if (e.key.keysym.sym == SDLK_v)
        {
            lightSource2 = lightSource2 + glm::vec3(-1, 0, 0);
        }
        if (e.key.keysym.sym == SDLK_b)
        {
            lightSource2 = lightSource2 + glm::vec3(1, 0, 0);
        }
        if (e.key.keysym.sym == SDLK_u)
        {
            lightSource1 = lightSource1 + glm::vec3(0, 0.5, 0);
        }
        if (e.key.keysym.sym == SDLK_i)
        {
            lightSource1 = lightSource1 + glm::vec3(0, -0.5, 0);
        }
        if (e.key.keysym.sym == SDLK_o)
        {
            lightSource1 = lightSource1 + glm::vec3(-1, 0, 0);
        }
        if (e.key.keysym.sym == SDLK_p)
        {
            lightSource1 = lightSource1 + glm::vec3(1, 0, 0);
        }


        if (e.key.keysym.sym == SDLK_l)
        {

            int vertexLoc = glGetAttribLocation(shader, "position");

            //Starting positions
            //******************************************
            Projection = glm::perspective(glm::radians(45.0f), winsizex / winsizey, 0.1f, 100.0f);

            View = glm::lookAt(
                glm::vec3(0, 0, 3), // The camera position
                glm::vec3(0, 0, 0), // The camera target
                glm::vec3(0, 1, 0)  // The camera upwards direction
            );

            Model = glm::mat4(1.0f);

            int colorLoc = glGetUniformLocation(shader, "objectColor");
            glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
            lightSource1 = glm::vec3(1, 0, 1);
            lightSource2 = glm::vec3(-1, 0, 1);

            cameraPosition = glm::vec3(0, 0, 3);
            Rotation = glm::mat4(1);
            Translation = glm::mat4(1);
            Scaling = glm::mat4(1);

            duplicate = true;
        }
    }

    if (e.type == SDL_MOUSEBUTTONDOWN)
    {
        dragging = true;
    }
    if (e.type == SDL_MOUSEBUTTONUP)
    {
        dragging = false;
    }
    if (e.type == SDL_MOUSEMOTION && dragging == true)
    {
        int x, y;
        SDL_GetMouseState(&x, &y);

        float xdiff = x - e.motion.x;
        float ydiff = e.motion.y - y;

        // Translate in the x and y direction
        if (translating)
        {

            if (rotate_on_world)
            {
                Translation = translate(xdiff / 1000, ydiff / 1000, 0.0f);
                Model = Translation * Model;
            }
            else
            {
                Translation = glm::translate(Translation, glm::vec3(xdiff / 1000, ydiff / 1000, 0.0f));
            }

        }
        // Rotate using mouse in certain directions based on axis
        if (rotating)
        {

            if (rotate_on_world)
            {
                Rotation = rotate(axis == 'y' ? xdiff : axis == 'x' ? ydiff
                                                                    : xdiff + ydiff,
                                  axis == 'x' ? 1 : 0, axis == 'y' ? 1 : 0, axis == 'z' ? 1 : 0);
                Model = Rotation * Model;
            }
            else
            {
                Rotation = glm::rotate(Rotation, glm::radians(axis == 'y' ? xdiff : axis == 'x' ? ydiff
                                                                                                : xdiff + ydiff),
                                       glm::vec3(axis == 'x' ? 1 : 0, axis == 'y' ? 1 : 0, axis == 'z' ? 1 : 0));
            }

        }
        //Scale the model
        if (scaling)
        {

            if (rotate_on_world)
            {
                Scaling = scale(1 + ((xdiff + ydiff) / 100));
                Model = Model * Scaling;
            }
            else
            {
                Scaling = glm::scale(Scaling, glm::vec3(1 + ((xdiff + ydiff) / 100)));
            }
        }
        
    }

    return true;
}

void OpenGLWindow::cleanup()
{
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vertexBuffer2);
    glDeleteVertexArrays(1, &vao2);
    SDL_DestroyWindow(sdlWin);
}
