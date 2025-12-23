#version 300 es
precision highp float;
layout(location=0) in vec3 vPosition; 
 
uniform mat4 lightSpaceTrMatrix; 
uniform mat4 model; 
 
void main() 
{ 
    gl_Position = lightSpaceTrMatrix * model * vec4(vPosition, 1.0f); 
} 