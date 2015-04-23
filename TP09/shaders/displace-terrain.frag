#version 330

// input
uniform sampler2D colormap;

in vec3 eyeView;
in float outelev;
in vec3 normal;

// output
layout(location=0) out vec4 outColor;
layout(location=1) out vec4 outNormal;

void main() {
  vec2 uvcoord = vec2(1-outelev,0);
 
  // Colorer le niveau le plus bas en bleu (eau)
  if (outelev > 0.89) { // Outelev va de ]0 à 0.9[
    outColor = vec4(0.2,0.2,1.0,0.0);
  } else {
    outColor = texture(colormap,uvcoord);
  }
  outNormal = vec4(normal,1.0);
}
