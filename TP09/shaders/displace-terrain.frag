#version 330

uniform sampler2D terrain;
in vec4 outBuffer;
in vec3 eyeview;
out vec4 outBuffer2;
void main() {
 outBuffer2 = outBuffer;
}
