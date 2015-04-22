#version 330

// input attributes 
layout(location = 0) in vec3 position;// position of the vertex in world space

void main() {
  gl_Position = vec4(position,1.0);
}
