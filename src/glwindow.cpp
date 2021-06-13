#include <iostream>
#include <stdio.h>

#include "SDL.h"
#include <GL/glew.h>

#include <glm/vec3.hpp>                  // glm::vec3
#include <glm/vec4.hpp>                  // glm::vec4
#include <glm/mat4x4.hpp>                // glm::mat4
#include <glm/ext/matrix_transform.hpp>  // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp>  // glm::pi

#include "glwindow.h"
#include "geometry.h"

using namespace std;
glm::mat4 Rotation = glm::mat4(1);
glm::mat4 Translation = glm::mat4(1);
glm::mat4 Scaling = glm::mat4(1);
glm::mat4 Model;
glm::mat4 View;
glm::mat4 Projection;
glm::mat4 MVP;

bool dragging = false;
bool rotating = false;
char axis = 'x';

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


    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Note that this path is relative to your working directory
    // when running the program (IE if you run from within build
    // then you need to place these files in build as well)
    shader = loadShaderProgram("simple.vert", "simple.frag");
    glUseProgram(shader);

    int colorLoc = glGetUniformLocation(shader, "objectColor");
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);

    // Load the model that we want to use and buffer the vertex attributes
    GeometryData geo = loadOBJFile("objects/teapot.obj");
    obj_vertices_count = geo.vertexCount();
    obj_x_size = abs(geo.minx) + abs(geo.maxx);

    GeometryData geo2 = loadOBJFile("objects/doggo.obj");
    obj_vertices_count2 = geo2.vertexCount();


    int vertexLoc = glGetAttribLocation(shader, "position");

    // The projection matrix
    Projection = glm::perspective(glm::radians(45.0f), winsizex / winsizey, 0.1f, 100.0f);

    View = glm::lookAt(
        glm::vec3(0, 0, 3), // the camera's position
        glm::vec3(0, 0, 0), // the camera's target
        glm::vec3(0, 1, 0)  // the camera's upwards direction
    );
    //The model matrix
    Model = glm::mat4(1.0f);

    //Model_View_Projection matrix for transformation
    MVP = Projection * View * Model;

    
    // Put obj data in vertex buffer
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, geo.vertexCount() * 3 * sizeof(float), geo.vertexData(), GL_STATIC_DRAW);
    glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(vertexLoc);

    // Get the MVP matrix location
    GLuint MVP_MATRIX = glGetUniformLocation(shader, "MVP");
    // Put the initial MVP matrix into shader
    glUniformMatrix4fv(MVP_MATRIX, 1, GL_FALSE, &MVP[0][0]);

    // Load and bind vertex array object 2
    glGenVertexArrays(1, &vao2);
    glBindVertexArray(vao2);
    // Load the vertex buffer 2 for second obj
    glGenBuffers(1, &vertexBuffer2);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer2);
    glBufferData(GL_ARRAY_BUFFER, geo2.vertexCount() * 3 * sizeof(float), geo2.vertexData(), GL_STATIC_DRAW);
    glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(vertexLoc);

   
    glPrintError("Setup complete", true);
}
//Following methods are essentially just wrappers so i dont have to type glm every time
glm::mat4 translate(const glm::mat4 &model, float x, float y, float z)
{
    return glm::translate(model, glm::vec3(x, y, z));
}
glm::mat4 rotate(const glm::mat4 &model, const float radians_to_rotate, float xaxis, float yaxis, float zaxis)
{
    return glm::rotate(model, glm::radians(radians_to_rotate), glm::vec3(xaxis, yaxis, zaxis));
}
glm::mat4 scale(const glm::mat4 &model, float size)
{
    return glm::scale(model, glm::vec3(size));
}

void OpenGLWindow::render()
{   
    // Bind vertex array object 1 to render first obj
    glBindVertexArray(vao);

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

    glUniformMatrix4fv(glGetUniformLocation(shader, "MVP"), 1, GL_FALSE, &MVP[0][0]);
    // Draw obj
    glDrawArrays(GL_TRIANGLES, 0, obj_vertices_count);

    //draws duplicate if it should be on screen
    if (duplicate)
    {   
        // Bind vertex array 2 for second object to be drawn
        glBindVertexArray(vao2);
        // Give new MVP matrix to object 2
        glUniformMatrix4fv(glGetUniformLocation(shader, "MVP"), 1, GL_FALSE, &(Projection * View *Translation*glm::inverse(Rotation)*Scaling* translate(Model, obj_x_size, 0, 0))[0][0]);
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
    {   // Rotattion axis switch
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
        // Reset and draw second object Switch
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
            Rotation = glm::mat4(1);
            Translation = glm::mat4(1);
            Scaling = glm::mat4(1);
            MVP = Projection * View * Model;

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
            Translation = translate(Translation, xdiff / 1000, ydiff / 1000, 0.0f);
        }
        // Rotate using mouse in certain directions based on axis
        if (rotating)
        {

            Rotation = rotate(Rotation, axis == 'y' ? xdiff : axis == 'x' ? ydiff
                                                                          : xdiff + ydiff,
                              axis == 'x' ? 1 : 0, axis == 'y' ? 1 : 0, axis == 'z' ? 1 : 0);
            
        }
        //Scale the model
        if (scaling)
        {

            Scaling = scale(Scaling, 1 + ((xdiff + ydiff) / 100));
        }
        //Inverse rotation to put rotation in world axis since model matrix is identity and never changed
        MVP = Projection * View *Translation* glm::inverse(Rotation)*Scaling* Model ;
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
