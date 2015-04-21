#version 330

// input attributes 
layout(location = 0) in vec3 position;// position of the vertex in world space
uniform sampler2D terrain;

out vec3 eyeView;
out float outelev;
out vec3 normal;
uniform mat4 mdvMat; // modelview matrix (constant for all the vertices)
uniform mat4 projMat; // projection matrix (constant for all the vertices)
uniform mat3 normalMat; // normal matrix (constant for all the vertices)
float gradientgy, gradientgx;
float pas = 1/512;
float alpha = 1000;
void main() {
 vec3 v = position;

 vec3 text = position*0.5 + 0.5;

  float elevation = texture(terrain,text.xy).x;

  v.z += texture(terrain,text.xy);
  vec2 haut =   vec2(text.x,text.y + pas);
  vec2 bas =    vec2(text.x,text.y - pas);
  vec2 gauche = vec2(text.x - pas,text.y);
  vec2 droite = vec2(text.x + pas ,text.y);
  gradientgy = (texture(terrain,haut)  - texture(terrain,bas)) * alpha;
  gradientgx = (texture(terrain,droite)- texture(terrain,gauche)) * alpha;
  vec3 v1 = vec3(1,0,gradientgx);
  vec3 v2 = vec3(0,1,gradientgy);
  normal = normalize(cross(v1,v2));
  v = v + normal;
  eyeView = normalize((mdvMat*vec4(v,1.0)).xyz); // position in view space, normalized (view vector)
  gl_Position = projMat*mdvMat*vec4(v,1.0);          // projected position (in screen space)
  outelev = elevation;
}

