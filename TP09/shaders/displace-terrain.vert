#version 330

// input attributes 
layout(location = 0) in vec3 position;// position of the vertex in world space

uniform sampler2D terrain; // noisemap
uniform mat4 mdvMat; // modelview matrix (constant for all the vertices)
uniform mat4 projMat; // projection matrix (constant for all the vertices)
uniform mat3 normalMat; // normal matrix (constant for all the vertices)

// output
out vec3 eyeView;
out float outelev;
out vec3 normal;

// Constants
float pas = 1./512;
float alpha = 125; // normal force

void main() {
  vec3 newPos = position;

  vec2 uvcoord = (position*0.5 + 0.5).xy;
  
  newPos.z += texture(terrain,uvcoord).z;
  vec2 haut =   vec2(uvcoord.x,uvcoord.y + pas);
  vec2 bas =    vec2(uvcoord.x,uvcoord.y - pas);
  vec2 gauche = vec2(uvcoord.x - pas,uvcoord.y);
  vec2 droite = vec2(uvcoord.x + pas,uvcoord.y);
  float gradientgy = (texture(terrain,haut).x  - texture(terrain,bas).x) * alpha;
  float gradientgx = (texture(terrain,droite).x- texture(terrain,gauche).x) * alpha;
  vec3 v1 = vec3(1,0,gradientgx);
  vec3 v2 = vec3(0,1,gradientgy);

  outelev = texture(terrain,uvcoord).z;  
  normal = normalize(cross(v1,v2));
  eyeView = normalize((mdvMat*vec4(newPos,1.0)).xyz); // position in view space, normalized (view vector)
  gl_Position = projMat*mdvMat*vec4(newPos,1.0);      // projected position (in screen space)
}

