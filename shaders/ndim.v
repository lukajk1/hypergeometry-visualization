#version 330 core

// Up to 8 coordinates, split across two vec4 attributes (xyzw, then the remaining axes)
layout(location = 0) in vec4 pos_group1;
layout(location = 1) in vec4 pos_group2;

const int MAX_DIMENSIONS = 8;

// Distance of the viewpoint from the origin along each axis that gets projected out
const float PROJECTION_DISTANCE = 3.0;

uniform int dimensions;
// dimensions x dimensions rotation matrix, row-major
uniform float rotationMat[MAX_DIMENSIONS * MAX_DIMENSIONS];
uniform float scale;

uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;

void main() {
    float position[MAX_DIMENSIONS] = float[](
        pos_group1.x, pos_group1.y, pos_group1.z, pos_group1.w,
        pos_group2.x, pos_group2.y, pos_group2.z, pos_group2.w);

    // 1. Rotate in N dimensions - manual matrix-vector multiplication
    float rotated[MAX_DIMENSIONS] = float[](0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < dimensions; i++) {
        for (int j = 0; j < dimensions; j++) {
            rotated[i] += rotationMat[i * dimensions + j] * position[j];
        }
    }

    // 2. Perspective-project one axis at a time until three remain (N -> N-1 -> ... -> 3).
    // Points further along the dropped axis shrink, the same way depth works in 3D.
    for (int k = dimensions - 1; k >= 3; k--) {
        float depth = PROJECTION_DISTANCE + rotated[k];

        // Clip points at or behind the viewpoint
        if (depth < 0.001) {
            gl_Position = vec4(0.0);
            fragPos = vec3(0.0);
            return;
        }

        for (int i = 0; i < k; i++) {
            rotated[i] /= depth;
        }
    }

    // 2D shapes lie in the z = 0 plane. Scale is applied in 3D world space to avoid distortion.
    vec3 worldPos = vec3(rotated[0], rotated[1], rotated[2]) * scale;
    // color output will use worldspace coords
    fragPos = worldPos;

    // 3. Apply the 3D view (camera) transformation and projection to screen space
    gl_Position = projection * view * vec4(worldPos, 1.0);
}
