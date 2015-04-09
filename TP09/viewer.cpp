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
      _ndResol(512) {

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

    cleanIds();
}

void Viewer::generateIds() { // TODO! ==================================================================================generateIds
    // VAOs
    glGenVertexArrays(1,&_vaoQuad);
    glGenVertexArrays(1,&_vaoTerrain);
    // VBOs
    glGenBuffers(1,&_vboQuad);
    glGenBuffers(2,_vboTerrain);

    // FBOs
    //glGenFramebuffers(4, _frameBuffer); //On genere un id associé au FBO

    // Textures
    //glGenTextures(1,&_texDepth);
}

void Viewer::cleanIds() { // TODO! ======================================================================================cleanIds
    // VBOs
    glDeleteBuffers(1,&_vboQuad);
    glDeleteBuffers(2,_vboTerrain);
    // VAOs
    glDeleteVertexArrays(1,&_vaoQuad);
    glDeleteVertexArrays(1,&_vaoTerrain);

    // FBOs
    //glDeleteFramebuffers();
}

/**
 * @brief Viewer::initFBO
 * The Frame Buffer Objects are wrappers for textures, instead of drawning on the screen
 * we could use the FBOs as an output for our drawns. (=Redirect the stdout to a storable frame buffer!)
 * We will need 4 Passes to get our final mountains on the screen, so we need:
 * 1 FBO to get the results of the Computation PASS
 * 1 FBO to store the shadowmap of the Shadow PASS
 */
void Viewer::initFBO() { // TODO! ========================================================================================initFBO
    // Creates the textures
    //glBindTexture(GL_TEXTURE_2D,); // Sets texture as active
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24 ,_depthResol, _depthResol, 0, GL_DEPTH_COMPONENT, GL_FLOAT,NULL); // ???

    // Parametrize the texture
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Parameters for PCF (shadow blur with OpenGL) -> shadowmap_2D
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

    // Links the textures to the FBO
    //glBindFramebuffer(GL_FRAMEBUFFER,_fbo); // Sets FBO as active
    //glBindTexture(GL_TEXTURE_2D,_texDepth); // Sets the texture as active
    //glFramebufferTexture2D(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,_texDepth,0); // Links

    // test if everything is ok
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "Warning: FBO not complete!" << endl;

    // disable FBO
    glBindFramebuffer(GL_FRAMEBUFFER,0);
}

/**
 * @brief Viewer::initVBO
 * Allocates the space for the Vertex Buffer Objects (Quad and Terrain)
 * Quad is just the geometry maded to display polygonal images on the screen (2 triangles = rectangle)
 * Terrain is our mountain vertices... We also need to store the faces (index of vertices)
 */
void Viewer::initVBO() { // OK!
    const GLfloat quadData[] = {-1.0f,-1.0f,0.0f, 1.0f,-1.0f,0.0f, -1.0f,1.0f,0.0f, -1.0f,1.0f,0.0f, 1.0f,-1.0f,0.0f, 1.0f,1.0f,0.0f }; // Quad vertices

    // Creates VBO of Quad VAO
    glBindVertexArray(_vaoQuad); // Sets Quad VAO as active                           // Sets Quad VAO as active

    // Sends and enables vertices to the GPU
    glBindBuffer(GL_ARRAY_BUFFER, _vboQuad);                // Sets Quad VBO (vertices) as active
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW); // Links the data to the VBO
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);



    // Creates VBOs of Terrain VAO
    glBindVertexArray(_vaoTerrain); // Sets Terrain VAO as active

    // Sends and enables vertices to the GPU
    glBindBuffer(GL_ARRAY_BUFFER, _vboTerrain[0]);          // Sets Terrain VBO (vertices) as active
    glBufferData(GL_ARRAY_BUFFER, _grid->nbVertices()*3*sizeof(float), _grid->vertices(), GL_STATIC_DRAW); // Links the Grid.vertices to this VBO
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); // FIXME: is this needed?
    glEnableVertexAttribArray(0); // FIXME: is this needed?
    // Sends and enables faces to the GPU
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboTerrain[1]);          // Sets Terrain VBO (faces) as active
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _grid->nbFaces()*3*sizeof(unsigned int), _grid->faces(), GL_STATIC_DRAW); // Links the Grid.vertices to this VBO
    // Don't need to VertexAttrib and EnableVertex cause it's not a vertex!(?) (faces)

    // back to normal
    glBindVertexArray(0);
}

void Viewer::createShaders() { // OK!
    // *** PASS 1 *******************
    _vertexFilenames.push_back("shaders/noise.vert");
    _fragmentFilenames.push_back("shaders/noise.frag");
    // ******************************

    // *** PASS 2 *******************
    _vertexFilenames.push_back("shaders/computation.vert");
    _fragmentFilenames.push_back("shaders/computation.frag");
    // ******************************

    // *** PASS 3 *******************
    _vertexFilenames.push_back("shaders/shadow.vert");
    _fragmentFilenames.push_back("shaders/shadow.frag");
    // ******************************

    // *** PASS 4 *******************
    _vertexFilenames.push_back("shaders/render.vert");
    _fragmentFilenames.push_back("shaders/render.frag");
    // ******************************
}

/**
 * @brief Viewer::initShaders
 * Will define some names to use for our uniform variables and attributes sended to the shaders...
 */
void Viewer::initShaders() { // TODO! ==================================================================================initShaders
    // PASS 1: Noise
    glUseProgram(_shaders[0]->id());
    _noiseVertexLoc = glGetAttribLocation (_shaders[0]->id() ,"position");
    _noiseMotionLoc = glGetUniformLocation(_shaders[0]->id() ,"motion");

    // PASS 2: Computation
    glUseProgram(_shaders[1]->id());

    // PASS 3: Shadows
    glUseProgram(_shaders[2]->id());

    // PASS 4: Final Rendering VII
    glUseProgram(_shaders[3]->id());

    // Disable everything
    glUseProgram(0);
}

/**
 * @brief Viewer::drawSceneFromCamera
 * @param id
 * Draws the mountains from the pov of the camera.
 * Used on PASS 2 : Computation
 * We have to send the heighmap (noise map) and some matrices (mvp pov camera),
 * to the .vert, it must compute the good vertices position with it.
 * Then, we compute the Depthmap (pov of the camera), the Colormap,
 * the Normalmap from the .frag (, maybe tangentmap?...)
 */
void Viewer::drawSceneFromCamera(GLuint id) { // TODO (I just paste some code from ShadowTP ...) ======================drawSceneFromCamera
    // mdv matrix from the light point of view
    const float size = _mesh->radius*10;
    glm::vec3 l   = glm::transpose(_cam->normalMatrix())*_light;
    glm::mat4 p   = glm::ortho<float>(-size,size,-size,size,-size,2*size);
    glm::mat4 v   = glm::lookAt(l, glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 m   = glm::mat4(1.0);
    glm::mat4 mv  = v*m;

    // send uniform variables
    glUniformMatrix4fv(glGetUniformLocation(id,"projMat"),1,GL_FALSE,&(_cam->projMatrix()[0][0]));
    glUniformMatrix3fv(glGetUniformLocation(id,"normalMat"),1,GL_FALSE,&(_cam->normalMatrix()[0][0]));
    glUniform3fv(glGetUniformLocation(id,"light"),1,&(_light[0]));

    // send textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,_texColor[0]);
    glUniform1i(glGetUniformLocation(id,"colormap"),0);

    glActiveTexture(GL_TEXTURE0+1);
    glBindTexture(GL_TEXTURE_2D,_texNormal[0]);
    glUniform1i(glGetUniformLocation(id,"normalmap"),1);

    // *** TODO: send the shadow map here ***
    glActiveTexture(GL_TEXTURE0+2);
    glBindTexture(GL_TEXTURE_2D,_texDepth);
    glUniform1i(glGetUniformLocation(id,"shadowmap"),2);

    glBindVertexArray(_vaoObject);

    // draw several objects
    const float r = _mesh->radius*2;
    const int   n = 2;
    for(int i=-n;i<=n;++i) {

        // send the modelview matrix (changes for each object)
        const glm::vec3 pos = glm::vec3(i*r,0,i*r);
        const glm::mat4 mdv = glm::translate(_cam->mdvMatrix(),pos);
        glUniformMatrix4fv(glGetUniformLocation(id,"mdvMat"),1,GL_FALSE,&(mdv[0][0]));

        // send the modelview projection depth matrix
        const glm::mat4 mvpDepth = p*glm::translate(mv,pos);
        glUniformMatrix4fv(glGetUniformLocation(id,"mvpDepthMat"),1,GL_FALSE,&mvpDepth[0][0]);

        // draw faces
        glDrawElements(GL_TRIANGLES,3*_mesh->nb_faces,GL_UNSIGNED_INT,(void *)0);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,_texColor[1]);
    glUniform1i(glGetUniformLocation(id,"colormap"),0);

    glActiveTexture(GL_TEXTURE0+1);
    glBindTexture(GL_TEXTURE_2D,_texNormal[1]);
    glUniform1i(glGetUniformLocation(id,"normalmap"),1);

    // send initial mdv matrix
    glUniformMatrix4fv(glGetUniformLocation(id,"mdvMat"),1,GL_FALSE,&(_cam->mdvMatrix()[0][0]));

    // send initial mvp depth matrix
    const glm::mat4 mvpDepth = p*mv;
    glUniformMatrix4fv(glGetUniformLocation(id,"mvpDepthMat"),1,GL_FALSE,&mvpDepth[0][0]);

    // draw the floor
    glBindVertexArray(_vaoFloor);
    glDrawArrays(GL_TRIANGLES,0,6);

    // disable VAO
    glBindVertexArray(0);
}
/**
 * @brief Viewer::drawSceneFromLight
 * @param id
 * Draws the mountains from the pov of the light.
 * Used on PASS 3 : Shadows
 * We have to send the mountains and some matrices (mvp pov light),
 * to the .vert.
 * Then, we get the Shadowmap (=Depthmap pov light), there's almost
 * nothing to do in the shaders... (I guess)
 */
void Viewer::drawSceneFromLight(GLuint id) { // TODO (I just paste some code from ShadowTP ...) =======================drawSceneFromLight
    // mdv matrix from the light point of view
    const float size = _mesh->radius*10;
    glm::vec3 l   = glm::transpose(_cam->normalMatrix())*_light;
    glm::mat4 p   = glm::ortho<float>(-size,size,-size,size,-size,2*size);
    glm::mat4 v   = glm::lookAt(l, glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 m   = glm::mat4(1.0);
    glm::mat4 mv  = v*m;

    // *** TODO: draw the scene from the light point of view here ***

    glBindVertexArray(_vaoObject);

    // draw several objects
    const float r = _mesh->radius*2;
    const int   n = 2;
    for(int i=-n;i<=n;++i) {

        // send the modelview matrix (changes for each object)
        const glm::vec3 pos = glm::vec3(i*r,0,i*r);

        // send the modelview projection depth matrix
        const glm::mat4 mvpDepth = p*glm::translate(mv,pos);
        glUniformMatrix4fv(glGetUniformLocation(id,"mvpMat"),1,GL_FALSE,&mvpDepth[0][0]);

        // draw faces
        glDrawElements(GL_TRIANGLES,3*_mesh->nb_faces,GL_UNSIGNED_INT,(void *)0);
    }

    // send initial mvp depth matrix
    const glm::mat4 mvpDepth = p*mv;
    glUniformMatrix4fv(glGetUniformLocation(id,"mvpMat"),1,GL_FALSE,&mvpDepth[0][0]);

    // draw the floor
    glBindVertexArray(_vaoFloor);
    glDrawArrays(GL_TRIANGLES,0,6);

    // disable VAO
    glBindVertexArray(0);
}

void Viewer::paintGL() { // TODO P1, P2, P3, P4! ========================================================================= PAINT GL !

    // default : compute a 512*512 noise texture

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
    glUniform3fv(_noiseMotionLoc,1,&(_motion[0]));

    // activate quad vertices
    glEnableVertexAttribArray(_noiseVertexLoc);
    glBindBuffer(GL_ARRAY_BUFFER,_vboQuad);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void *)0);

    // draw the quad
    glDrawArrays(GL_TRIANGLES,0,6);

    // disable array
    glDisableVertexAttribArray(_noiseVertexLoc);

    // restore previous state
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    // disable shader
    glUseProgram(0);
}

void Viewer::resizeGL(int width,int height) {
    _cam->initialize(width,height,false);
    glViewport(0,0,width,height);
    initFBO();
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
        glm::vec2 v = glm::normalize(glm::vec2(glm::transpose(_cam->normalMatrix())*glm::vec3(0,0,-1)))*step;
        _motion[0] += v[0];
        _motion[1] += v[1];
    }

    if(ke->key()==Qt::Key_S) {
        glm::vec2 v = glm::normalize(glm::vec2(glm::transpose(_cam->normalMatrix())*glm::vec3(0,0,-1)))*step;
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
        initShaders();
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
    initShaders();

    // Ids for vbos, fbos and textures
    generateIds();

    // init VBO
    initVBO();
    
    // create/init FBO
    initFBO();

    // starts the timer
    _timer->start();
}

