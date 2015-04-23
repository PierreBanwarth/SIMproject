#version 330

// input
uniform sampler2D colormap;
uniform float animTimer;

in vec3 eyeView;
in float outelev;
in vec3 normal;
in vec3 newPos;

// output
layout(location=0) out vec4 outColor;
layout(location=1) out vec4 outNormal;

// Parameters
float colorTextureStretching = 0.4; // vers 0: diminue étirement texture
									// vers +inf: augmente étirement texture
float colorTextureHeight = -0.15;    // texture hight offset 
vec4 waterColor = vec4(0.2,0.2,1.0,0.0); // Color of the water
float waterThreshold = 0.11; // Must be > waterLevel in noise.frag! 

void main() {
  vec2 uvcoord = vec2((1-outelev)/colorTextureStretching+colorTextureHeight,0);
 
  // Colorer le niveau le plus bas en bleu (eau)
  if (outelev > (1-waterThreshold)) { // Outelev va de ]0 à 0.9[
	  
	  // TODO creer une texture de reflets (noise.frag) pour l'eau? plutôt que calculer ça avec des mod chelous!!!
    if (mod(newPos.y,0.02) < 0.001 && mod(newPos.x,0.02) < 0.001) {
		waterColor += animTimer/60;
	}
	if (mod(newPos.y,0.03) < 0.001 && mod(newPos.x,0.03) < 0.001) {
		waterColor += 1-(animTimer/60);
	}
	
    outColor = waterColor;
  } else {
    outColor = texture(colormap,uvcoord);
  }
  outNormal = vec4(normal,1.0);
}
