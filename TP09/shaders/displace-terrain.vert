#version 330

// input attributes 
layout(location = 0) in vec3 position;// position of the vertex in world space
uniform sampler2D terrain;

out vec3 eyeView;
out vec2 uvcoord;
out float outelev;
uniform mat4 mdvMat; // modelview matrix (constant for all the vertices)
uniform mat4 projMat; // projection matrix (constant for all the vertices)
uniform mat3 normalMat; // normal matrix (constant for all the vertices)


void main() {
  vec3 v = position;

  vec3 text = position*0.5 + 0.5;

  vec4 elevation = texture2D(terrain,text.xy);
  v.z += texture2D(terrain,text.xy);
  uvcoord = vec2(v.z,v.z);
  outelev = v.z ;
  eyeView = normalize((mdvMat*vec4(v,1.0)).xyz); // position in view space, normalized (view vector)

  gl_Position = projMat*mdvMat*vec4(v,1.0);          // projected position (in screen space)

}

out vec4 outBuffer2;
