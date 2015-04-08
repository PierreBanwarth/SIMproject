#ifndef VIEWER_H
#define VIEWER_H

// GLEW lib: needs to be included first!!
#include <GL/glew.h> 

// OpenGL library 
#include <GL/gl.h>

// OpenGL Utility library
#include <GL/glu.h>

// OpenGL Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QGLFormat>
#include <QGLWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTimer>
#include <stack>

#include "camera.h"
#include "shader.h"
#include "grid.h"

class Viewer : public QGLWidget {
public:
    Viewer(char *filename,const QGLFormat &format=QGLFormat::defaultFormat());
    ~Viewer();

protected :
    virtual void paintGL();
    virtual void initializeGL();
    virtual void resizeGL(int width,int height);
    virtual void keyPressEvent(QKeyEvent *ke);
    virtual void mousePressEvent(QMouseEvent *me);
    virtual void mouseMoveEvent(QMouseEvent *me);

private:
    // OpenGL objects creation
    void generateIds();
    void cleanIds();
    void initFBO();
    void initVBO();
    void createShaders();

    // init a shader (called to get all variable locations)
    void initShaders();

    QTimer        *_timer;    // timer that controls the animation
    unsigned int   _currentshader; // current shader index

    Grid   *_grid;   // the grid
    Camera *_cam;    // the camera

    glm::vec3 _light;  // light direction
    glm::vec3 _motion; // motion offset for the noise texture
    bool      _mode;   // camera motion or light motion
    bool      _showShadowMap;

    std::vector<std::string> _vertexFilenames;   // all vertex filenames
    std::vector<std::string> _fragmentFilenames; // all fragment filenames
    std::vector<Shader *>    _shaders;           // all the shaders

    // location ids for the noise texture pass
    GLint _noiseVertexLoc;
    GLint _noiseMotionLoc;

    // VAOs (wrapper for the VBO)
    GLuint _vaoQuad;
    GLuint _vaoTerrain;

    // Quad VBO
    GLuint _vboQuad;

    // Terrain VBO
    GLuint _vboTerrain[2]; // Buffers for: vertices, faces


    // FBOs


    // shadow map and noise resolution
    unsigned int _ndResol;
};

#endif // VIEWER_H
