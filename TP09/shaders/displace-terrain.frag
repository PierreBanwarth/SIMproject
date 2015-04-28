#version 330

// input
uniform sampler2D colormap;
uniform sampler2D cloud;
uniform sampler2D specmap;

uniform vec3 light;

in vec3 eyeView;
in float outelev;
in vec3 normal;
in vec3 newPos;
vec4 specColor;
float spec;

// output
layout(location=0) out vec4 outColor;
layout(location=1) out vec4 outNormal;

// Parameters
float colorTextureStretching = 0.4; // vers 0: diminue étirement texture
									// vers +inf: augmente étirement texture
float colorTextureHeight = -0.15;    // texture hight offset 
vec4 waterColor = vec4(0.2,0.2,1.0,0.0); // Color of the water
float waterThreshold = 0.11; // Must be > waterLevel in noise.frag! 
float et = 75.0;
void main() {

  vec2 uvcoord = vec2((1-outelev)/colorTextureStretching+colorTextureHeight,0);
  vec3 l = normalize(light);
  vec3 e = normalize(eyeView);

  outNormal = vec4(normal,1.0);

  // diffus and specular terms
  float spec = pow(max(dot(reflect(l,outNormal.xyz),e),0.0),et);
  float diff = max(dot(l,outNormal.xyz),0.0);
  // part of toon shading
  // Colorer le niveau le plus bas en bleu (eau)
  if(outelev > (1-waterThreshold)){// Outelev va de ]0 à 0.1
    outColor = waterColor*diff +spec*texture(specmap,vec2(newPos.x,newPos.y))*vec4(0.7,0.7,0.1,1.0);
    //outColor = waterColor + specColor;//texture(cloud,vec2(newPos.x,newPos.y));
  }else {
    outColor = texture(colormap,uvcoord);
  }
  if(smoothstep(0.0,0.8,dot(eyeView,outNormal.xyz))==1){
      //outColor = vec4(0.0 , 0.0 , 0.0 , 1.0);
  }
}

