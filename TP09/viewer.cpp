#include "viewer.h"

#include <math.h>
#include <iostream>
#include <QTime>

using namespace std;

Viewer::Viewer(char *,const QGLFormat &format)
    : QGLWidget(format),
      _timer(new QTimer(this)),
      _currentshader(0),
      _light(glm::vec3(0,0,1)),
      _motion(glm::vec3(0,0,0)),
      _mode(false),
      _showShadowMap(false),
      //_ndResol(512) {
      _ndResol(1024) {

    setlocale(LC_ALL,"C");

    _grid = new Grid(_ndResol,-1.0f,1.0f);
    _cam  = new Camera(1.0f,glm::vec3(0.0f,0.0f,0.0f));

    _timer->setInterval(10);
    connect(_timer,SIGNAL(timeout()),this,SLOT(updateGL()));
}

Viewer::~Viewer() {
    delete _timer;
    delete _grid;
    delete _cam;

    for(unsigned int i=0;i<_shaders.size();++i) {
        delete _shaders[i];
    }

    // delete all GPU objects
    deleteVAO();
    deleteFBO();
    deleteExtraTextures();
}

void Viewer::createFBO() {
    // FBOs
    glGenFramebuffers(1,&_fboTerrain); // 512: Heighmap
    glGenFramebuffers(1,&_fboViewport); // viewport: colormap, normalmap, depthmap

    // Textures
    glGenTextures(1,&_texHeight);
    glGenTextures(1,&_texColor);
    glGenTextures(1,&_texNormal);
    glGenTextures(1,&_texDepth);
}

void Viewer::deleteFBO() {
    // FBOs
    glDeleteFramebuffers(1,&_fboTerrain);
    glDeleteFramebuffers(1,&_fboViewport);

    // Textures
    glDeleteTextures(1,&_texHeight);
    glDeleteTextures(1,&_texColor);
    glDeleteTextures(1,&_texNormal);
    glDeleteTextures(1,&_texDepth);
}

void Viewer::createVAO() {
    const GLfloat quadData[] = {-1.0f,-1.0f,0.0f, 1.0f,-1.0f,0.0f, -1.0f,1.0f,0.0f,
                                -1.0f,1.0f,0.0f, 1.0f,-1.0f,0.0f, 1.0f,1.0f,0.0f };

    // create the VAO associated with the grid (the terrain)
    glGenBuffers(2,_terrain);
    glGenVertexArrays(1,&_vaoTerrain);
    glBindVertexArray(_vaoTerrain);
    glBindBuffer(GL_ARRAY_BUFFER,_terrain[0]); // vertices
    glBufferData(GL_ARRAY_BUFFER,_grid->nbVertices()*3*sizeof(float),_grid->vertices(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,_terrain[1]); // indices
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,_grid->nbFaces()*3*sizeof(int),_grid->faces(),GL_STATIC_DRAW);

    // create the VAO associated with the quad
    glGenBuffers(1,&_quad);
    glGenVertexArrays(1,&_vaoQuad);
    glBindVertexArray(_vaoQuad);
    glBindBuffer(GL_ARRAY_BUFFER,_quad); // vertices
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadData),quadData,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void *)0);
    glEnableVertexAttribArray(0);
}

void Viewer::deleteVAO() {
    glDeleteBuffers(2,_terrain);
    glDeleteBuffers(1,&_quad);
    glDeleteVertexArrays(1,&_vaoTerrain);
    glDeleteVertexArrays(1,&_vaoQuad);
}

void Viewer::updateTex(GLuint tex,GLenum filter,GLenum wrap,unsigned int w,
                       unsigned int h,GLint iformat, GLenum format,bool isShadowmap) {

    glBindTexture(GL_TEXTURE_2D,tex);
    glTexImage2D(GL_TEXTURE_2D, 0,iformat,w,h,0,format,GL_FLOAT,NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

    if(isShadowmap) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    }
}

void Viewer::updateFBO() {

    // FBO Terrain
    // create the noise texture
    updateTex(_texHeight,GL_LINEAR,GL_CLAMP_TO_EDGE,_ndResol,_ndResol,GL_RGBA32F,GL_RGBA);

    // attach textures to the FBO dedicated to creating the terrain
    glBindFramebuffer(GL_FRAMEBUFFER,_fboTerrain);
    glBindTexture(GL_TEXTURE_2D,_texHeight);
    glFramebufferTexture2D(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,_texHeight,0);



    // FBO Viewport
    // create the colormap
    updateTex(_texColor,GL_LINEAR,GL_CLAMP_TO_EDGE,width(),height(),GL_RGBA32F,GL_RGBA);
    // attach textures to the FBO dedicated to the computation phase
    glBindFramebuffer(GL_FRAMEBUFFER,_fboViewport);
    glBindTexture(GL_TEXTURE_2D,_texColor);
    glFramebufferTexture2D(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,_texColor,0);

    // creates the normal texture
    updateTex(_texNormal,GL_LINEAR,GL_CLAMP_TO_EDGE,width(),height(),GL_RGBA32F,GL_RGBA);
    // attach textures to the FBO dedicated to creating the terrain
    glBindFramebuffer(GL_FRAMEBUFFER,_fboViewport);
    glBindTexture(GL_TEXTURE_2D,_texNormal);
    glFramebufferTexture2D(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D,_texNormal,0);

    // creates the depth map
    updateTex(_texDepth,GL_LINEAR,GL_CLAMP_TO_EDGE,width(),height(),GL_DEPTH_COMPONENT24,GL_DEPTH_COMPONENT);
    // attach textures to the FBO dedicated to creating the terrain
    glBindFramebuffer(GL_FRAMEBUFFER,_fboViewport);
    glBindTexture(GL_TEXTURE_2D,_texDepth);
    glFramebufferTexture2D(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,_texDepth,0);


    // test if everything is ok
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Warning: FBO[0] not complete!" << endl;


    // disable FBO
    glBindFramebuffer(GL_FRAMEBUFFER,0);
}

void Viewer::createExtraTextures() {
    // Load the color texture
    glGenTextures(1,&_colorTexId);
    QImage image0 = QGLWidget::convertToGLFormat(QImage("texture/texturemontagne.png"));
    glBindTexture(GL_TEXTURE_2D,_colorTexId);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,image0.width(),image0.height(),0,
                 GL_RGBA,GL_UNSIGNED_BYTE,(const GLvoid *)image0.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    // cloud texture
    glGenTextures(1,&_cloudTexId);
    QImage image1 = QGLWidget::convertToGLFormat(QImage("texture/cloudMap.jpg"));
    glBindTexture(GL_TEXTURE_2D,_cloudTexId);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,image1.width(),image1.height(),0,
                 GL_RGBA,GL_UNSIGNED_BYTE,(const GLvoid *)image1.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    // water specular texture
    glGenTextures(1,&_specularId);
    QImage image2 = QGLWidget::convertToGLFormat(QImage("texture/water_specular.jpg"));
    glBindTexture(GL_TEXTURE_2D,_specularId);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,image2.width(),image2.height(),0,
                 GL_RGBA,GL_UNSIGNED_BYTE,(const GLvoid *)image2.bits());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
}

void Viewer::deleteExtraTextures() {
    glDeleteTextures(1,&_colorTexId);
    glDeleteTextures(1,&_cloudTexId);
}

void Viewer::createShaders() {
    // *** height field ***
    _vertexFilenames.push_back("shaders/noise.vert");
    _fragmentFilenames.push_back("shaders/noise.frag");
    // ******************************
    _vertexFilenames.push_back("shaders/show-terrain.vert");
    _fragmentFilenames.push_back("shaders/show-terrain.frag");

    _vertexFilenames.push_back("shaders/displace-terrain.vert");
    _fragmentFilenames.push_back("shaders/displace-terrain.frag");

    _vertexFilenames.push_back("shaders/final-renderer.vert");
    _fragmentFilenames.push_back("shaders/final-renderer.frag");
}

void Viewer::createHeightMap(GLuint id) {
    // send the motion vector
    glUniform3fv(glGetUniformLocation(id,"motion"),1,&(_motion[0]));

    // draw the quad
    glBindVertexArray(_vaoQuad);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);
}

void Viewer::drawSceneFromCamera(GLuint id) {
    // create gbuffers (deferred shading)

    // Send uniform variables (view matrix)*
    // send the light
    glUniform3fv(glGetUniformLocation(id,"light"),1,&(_light[0]));
    glUniformMatrix4fv(glGetUniformLocation(id,"mdvMat"),1,GL_FALSE,&(_cam->mdvMatrix()[0][0]));
    glUniformMatrix4fv(glGetUniformLocation(id,"projMat"),1,GL_FALSE,&(_cam->projMatrix()[0][0]));
    glUniformMatrix3fv(glGetUniformLocation(id,"normalMat"),1,GL_FALSE,&(_cam->normalMatrix()[0][0]));

    // send the animation clock
    glUniform1f(glGetUniformLocation(id,"animTimer"),(_animTimer));

    // Send color texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,_colorTexId);
    glUniform1i(glGetUniformLocation(id,"colormap"),0);
    // Send cloud texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,_cloudTexId);
    glUniform1i(glGetUniformLocation(id,"cloud"),1);
    // Send noisemap
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,_texHeight);
    glUniform1i(glGetUniformLocation(id,"noisemap"),2);
    // Send specular
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D,_specularId);
    glUniform1i(glGetUniformLocation(id,"specmap"),3);


    glBindVertexArray(_vaoTerrain);
    glDrawElements(GL_TRIANGLES,3*_grid->nbFaces(),GL_UNSIGNED_INT,(void *)0);
    glBindVertexArray(0);
}

void Viewer::drawSceneFromLight(GLuint id) {
    // create shadowmap
}

void Viewer::renderFinalImage(GLuint id) {

    // send the light
    glUniform3fv(glGetUniformLocation(id,"light"),1,&(_light[0]));

    // Send vawe texture
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D,_texWave);
    glUniform1i(glGetUniformLocation(id,"vaweTexture"),3);

    // send the textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,_texColor);
    glUniform1i(glGetUniformLocation(id,"colormap"),0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,_texNormal);
    glUniform1i(glGetUniformLocation(id,"normalmap"),1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,_texDepth);
    glUniform1i(glGetUniformLocation(id,"depthmap"),2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D,_specularId);
    glUniform1i(glGetUniformLocation(id,"specmap"),3);

    // Geometry: rectangle
    glBindVertexArray(_vaoQuad);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0);
}

void Viewer::pass1() {
    glBindFramebuffer(GL_FRAMEBUFFER,_fboTerrain);

    // draw into the first buffer
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    // viewport at the size of the heightmap
    glViewport(0,0,_ndResol,_ndResol);

    // disable depth test
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // clear color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    // activate the noise shader
    glUseProgram(_shaders[0]->id());

    // generate the noise texture
    createHeightMap(_shaders[0]->id());

    // restore previous state
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void Viewer::pass2() {
    glBindFramebuffer(GL_FRAMEBUFFER,_fboViewport);

    // We want to draw into FBO!!!
    GLenum toPrint[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_DEPTH_ATTACHMENT};
    glDrawBuffers(2, toPrint);

    // restore viewport sizes
    glViewport(0,0,width(),height());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(_shaders[2]->id());

    drawSceneFromCamera(_shaders[2]->id());
}

void Viewer::pass3() {
    // TODO
}

void Viewer::pass4() {
    // disable shader
    glBindFramebuffer(GL_FRAMEBUFFER,0);

    // restore viewport sizes
    glViewport(0,0,width(),height());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // activate the buffer shader
    glUseProgram(_shaders[3]->id());

    //testShowDisp(_shaders[2]->id());
    renderFinalImage(_shaders[3]->id());
}

void Viewer::paintGL() {
    int freq = 1;
    //  PASS 1 : Generate noise
    pass1();
    //  PASS 2 : Compute the texture buffers (normals, colors, depth)
    pass2();

    //  PASS 3 : Create the shadowmap (depth from the point of view of light)
    pass3();

    //  PASS 4 : Final render VII
    pass4();

    // Timer for animations!

    _globaltimer++;
    if( _globaltimer % freq == 0){
        _animTimer++;

    }
    glUseProgram(0);
}

void Viewer::resizeGL(int width,int height) {
    _cam->initialize(width,height,false);
    glViewport(0,0,width,height);
    updateFBO();
    updateGL();
}

void Viewer::mousePressEvent(QMouseEvent *me) {
    const glm::vec2 p((float)me->x(),(float)(height()-me->y()));

    if(me->button()==Qt::LeftButton) {
        _cam->initRotation(p);
        _mode = false;
    } else if(me->button()==Qt::MidButton) {
        _cam->initMoveZ(p);
        _mode = false;
    } else if(me->button()==Qt::RightButton) {
        _light[0] = (p[0]-(float)(width()/2))/((float)(width()/2));
        _light[1] = (p[1]-(float)(height()/2))/((float)(height()/2));
        _light[2] = 1.0f-std::max(fabs(_light[0]),fabs(_light[1]));
        _light = glm::normalize(_light);
        _mode = true;
    }

    updateGL();
}

void Viewer::mouseMoveEvent(QMouseEvent *me) {
    const glm::vec2 p((float)me->x(),(float)(height()-me->y()));

    if(_mode) {
        // light mode
        _light[0] = (p[0]-(float)(width()/2))/((float)(width()/2));
        _light[1] = (p[1]-(float)(height()/2))/((float)(height()/2));
        _light[2] = 1.0f-std::max(fabs(_light[0]),fabs(_light[1]));
        _light = glm::normalize(_light);
    } else {
        // camera mode
        _cam->move(p);
    }

    updateGL();
}

void Viewer::keyPressEvent(QKeyEvent *ke) {
    const float step = 0.05;
    if(ke->key()==Qt::Key_Z) {
        glm::vec2 v = glm::vec2(glm::transpose(_cam->normalMatrix())*glm::vec3(0,0,-1))*step;
        if(v[0]!=0.0 && v[1]!=0.0) v = glm::normalize(v)*step;
        else v = glm::vec2(0,1)*step;
        _motion[0] += v[0];
        _motion[1] += v[1];
    }

    if(ke->key()==Qt::Key_S) {
        glm::vec2 v = glm::vec2(glm::transpose(_cam->normalMatrix())*glm::vec3(0,0,-1))*step;
        if(v[0]!=0.0 && v[1]!=0.0) v = glm::normalize(v)*step;
        else v = glm::vec2(0,1)*step;
        _motion[0] -= v[0];
        _motion[1] -= v[1];
    }

    if(ke->key()==Qt::Key_Q) {
        _motion[2] += step;
    }

    if(ke->key()==Qt::Key_D) {
        _motion[2] -= step;
    }





    // key a: play/stop animation
    if(ke->key()==Qt::Key_A) {
        if(_timer->isActive())
            _timer->stop();
        else
            _timer->start();
    }

    // key i: init camera
    if(ke->key()==Qt::Key_I) {
        _cam->initialize(width(),height(),true);
    }

    // key f: compute FPS
    if(ke->key()==Qt::Key_F) {
        int elapsed;
        QTime timer;
        timer.start();
        unsigned int nb = 500;
        for(unsigned int i=0;i<nb;++i) {
            paintGL();
        }
        elapsed = timer.elapsed();
        double t = (double)nb/((double)elapsed);
        cout << "FPS : " << t*1000.0 << endl;
    }

    // key r: reload shaders
    if(ke->key()==Qt::Key_R) {
        for(unsigned int i=0;i<_vertexFilenames.size();++i) {
            _shaders[i]->reload(_vertexFilenames[i].c_str(),_fragmentFilenames[i].c_str());
        }
    }

    // key S: show the shadow map
    if(ke->key()==Qt::Key_S) {
        _showShadowMap = !_showShadowMap;
    }

    updateGL();
}

void Viewer::initializeGL() {
    // make this window the current one
    makeCurrent();

    // init and chack glew
    if(glewInit()!=GLEW_OK) {
        cerr << "Warning: glewInit failed!" << endl;
    }

    if(!GLEW_ARB_vertex_program   ||
            !GLEW_ARB_fragment_program ||
            !GLEW_ARB_texture_float    ||
            !GLEW_ARB_draw_buffers     ||
            !GLEW_ARB_framebuffer_object) {
        cerr << "Warning: Shaders not supported!" << endl;
    }

    // init OpenGL settings
    glClearColor(0.0,0.0,0.0,1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glViewport(0,0,width(),height());

    // initialize camera
    _cam->initialize(width(),height(),true);

    // load shader files
    createShaders();

    // init and load all shader files
    for(unsigned int i=0;i<_vertexFilenames.size();++i) {
        _shaders.push_back(new Shader());
        _shaders[i]->load(_vertexFilenames[i].c_str(),_fragmentFilenames[i].c_str());
    }

    // init shaders
    createShaders();

    // init VAO/VBO
    createVAO();
    
    // create/init FBO
    createFBO();
    updateFBO();

    createExtraTextures();

    // starts the timer
    _timer->start();
    _animTimer = 0;
    _globaltimer = 0;
    _isAnimForward = 1;
}

