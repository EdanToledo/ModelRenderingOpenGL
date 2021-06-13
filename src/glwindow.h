#ifndef GL_WINDOW_H
#define GL_WINDOW_H

#include <GL/glew.h>

#include "geometry.h"

class OpenGLWindow
{
public:
    OpenGLWindow();

    void initGL();
    void render();
    bool handleEvent(SDL_Event e);
    void cleanup();

private:
    SDL_Window* sdlWin;

    GLuint shader;

    GLuint texture;
    GLuint texturebuffer;
    GLuint normalMap;
    GLuint vao;
    GLuint vertexBuffer;

    GLuint normalbuffer;
    GLuint tangentbuffer;
    GLuint bitangentbuffer;

    GLuint texture2;
    GLuint texturebuffer2;
    GLuint normalbuffer2;
    GLuint normalMap2;
    GLuint vao2;
    GLuint vertexBuffer2;
    GLuint tangentbuffer2;
    GLuint bitangentbuffer2;

};

#endif
