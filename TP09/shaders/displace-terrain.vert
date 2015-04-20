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
  v = v+1/2;
  vec2 text = vec2(mod(v.x,512) , mod(v.y,512));
  v.z += texture2D(terrain,text);

  outBuffer = vec4(v.z*2,v.z*2,v.z*2,1.0);
  eyeView     = normalize((mdvMat*vec4(v,1.0)).xyz); // position in view space, normalized (view vector)
  gl_Position = projMat*mdvMat*vec4(v,1.0);          // projected position (in screen space)

}
