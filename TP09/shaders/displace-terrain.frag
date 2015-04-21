#version 330

uniform sampler2D colormap;

in vec3 eyeView;
in float outelev;
in vec3 normal;
out vec4 outBuffer;
out vec4 outNormal

void main() {
 vec2 uvcoord = vec2(1-outelev,0);
 //calcul
 outBuffer = texture(colormap,uvcoord);
 outNormal = vec4(normal,1.0);
}
