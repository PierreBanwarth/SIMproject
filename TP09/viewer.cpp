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

void Viewer::generateIds() {
  // VBOs
  glGenBuffers(1,&_quad);

  // generate your ids here 
  
}

void Viewer::cleanIds() {
  // VBOs
  glDeleteBuffers(1,&_quad);
  
  // delete your ids here 
}

void Viewer::initFBO() {
  
  // init your FBOs here 

  // disable FBO
  glBindFramebuffer(GL_FRAMEBUFFER,0);
}


void Viewer::initVBO() {
  // create your VBOs here 
  
  // create the VBO associated with the screen quad 
  const GLfloat quadData[] = {
    -1.0f,-1.0f,0.0f, 1.0f,-1.0f,0.0f, -1.0f,1.0f,0.0f, -1.0f,1.0f,0.0f, 1.0f,-1.0f,0.0f, 1.0f,1.0f,0.0f }; 
  glBindBuffer(GL_ARRAY_BUFFER,_quad); // vertices 
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadData),quadData,GL_STATIC_DRAW);
}

void Viewer::createShaders() {
  // *** height field *** 
  _vertexFilenames.push_back("shaders/noise.vert");
  _fragmentFilenames.push_back("shaders/noise.frag");
  // ******************************

  // add your shaders here 

}

void Viewer::initShaders() {
  // height field 
  glUseProgram(_shaders[0]->id());
  _noiseVertexLoc = glGetAttribLocation (_shaders[0]->id() ,"position");
  _noiseMotionLoc = glGetUniformLocation(_shaders[0]->id() ,"motion");

  // init your locations here 


  // disable everything
  glUseProgram(0);
}

void Viewer::paintGL() {

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
  glBindBuffer(GL_ARRAY_BUFFER,_quad);
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

