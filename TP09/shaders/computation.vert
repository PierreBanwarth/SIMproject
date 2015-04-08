#version 330

// Input attributes 
//layout(location = 0) in vec3 position;// position of the vertex in world space

// Uniform variables
//uniform mat4 mdvMat; // modelview matrix (constant for all the vertices)

// Output variables that will be interpolated during rasterization (equivalent to varying)
//out vec3 normalView;

void main() {
	// Final position of the current vertex
	gl_Position = vec4(1.0,1.0,1.0,1.0);          // projected position (in screen space)
}
