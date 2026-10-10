#version 330 core

in vec3 fragPos;

out vec4 FragColor;

void main() {
	float height = fragPos.y + .7;
	FragColor = vec4(height, height, height, 1.0);
}