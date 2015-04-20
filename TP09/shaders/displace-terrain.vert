#version 330

// input attributes 
layout(location = 0) in vec3 position;// position of the vertex in world space
uniform sampler2D terrain;
out vec4 outBuffer;
out vec3 eyeView;
uniform mat4 mdvMat; // modelview matrix (constant for all the vertices)
uniform mat4 projMat; // projection matrix (constant for all the vertices)
uniform mat3 normalMat; // normal matrix (constant for all the vertices)


void main() {
  vec3 v = position;
  v.z += texture2D(terrain,vec2(v.x,v.y));
  outBuffer = vec4(v,0.2);
  eyeView     = normalize((mdvMat*vec4(v,1.0)).xyz); // position in view space, normalized (view vector)
  gl_Position = projMat*mdvMat*vec4(v,1.0);          // projected position (in screen space)

}
