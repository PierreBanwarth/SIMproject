#version 330

out vec4 bufferColor;

// input textures and light direction as uniform variables 
uniform vec3      light;

uniform sampler2D colormap;
uniform sampler2D normalmap;
uniform sampler2D depthmap;


void main() {
  vec3 l = normalize(light);
 
  vec4 n = texelFetch(normalmap, ivec2(gl_FragCoord.xy),0);
  vec4 color = texelFetch(colormap, ivec2(gl_FragCoord.xy),0);
  float diff = max(dot(l,n.xyz),0.0);
  bufferColor = diff*color;
}
