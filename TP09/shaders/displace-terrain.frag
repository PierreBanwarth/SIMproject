#version 330

uniform sampler2D colormap;
in vec3 eyeView;
in vec2 uvcoord;
in float outelev;
out vec4 outBuffer2;
void main() {
 outBuffer2 = texture(colormap,vec2(outelev,outelev));
}
