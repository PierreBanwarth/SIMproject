#version 330

// input
uniform sampler2D colormap;

in vec3 eyeView;
in float outelev;
in vec3 normal;

// output
layout(location=0) out vec4 outBuffer;
layout(location=1) out vec4 outNormal;

void main() {
 vec2 uvcoord = vec2(1-outelev,0);
 //calcul
 outBuffer = texture(colormap,uvcoord);
 outNormal = vec4(normal,1.0);
}
