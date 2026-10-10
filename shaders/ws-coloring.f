#version 330 core

in vec3 fragPos;

out vec4 FragColor;

void main() {
	FragColor = vec4(fragPos + 0., 1.0);
}