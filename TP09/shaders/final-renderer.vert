#version 330

// input attributes 
layout(location = 0) in vec3 position;// position of the vertex in world space
float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}
void main() {
  gl_Position = vec4(position,1.0);
}
