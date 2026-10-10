#pragma once
#ifndef ROTATION_H
#define ROTATION_H

#include <cmath>
#include <vector>

struct RotationPlane {
    int axis1;        // First axis (0=X, 1=Y, 2=Z, 3=W, 4=V, etc.)
    int axis2;        // Second axis
    float speed;      // Rotation speed (radians per second)
};

// Builds the row-major NxN rotation that applies every plane in order at the given time.
// outMatrix must hold at least dimensions * dimensions floats.
inline void buildRotationMatrix(const std::vector<RotationPlane>& planes, int dimensions, float time, float* outMatrix) {
    for (int i = 0; i < dimensions * dimensions; i++) {
        outMatrix[i] = 0.0f;
    }
    for (int i = 0; i < dimensions; i++) {
        outMatrix[i * dimensions + i] = 1.0f;
    }

    for (const RotationPlane& plane : planes) {
        float planeAngle = time * plane.speed;

        int i = plane.axis1;
        int j = plane.axis2;

        // A rotation in plane ij is the identity except for
        //   [i][i] = cos, [i][j] = -sin, [j][i] = sin, [j][j] = cos
        // Left-multiplying by it only mixes rows i and j, so compose in place.
        // Writing those four entries directly would only be correct when no
        // two planes share an axis.
        float c = std::cos(planeAngle);
        float s = std::sin(planeAngle);
        for (int col = 0; col < dimensions; col++) {
            float a = outMatrix[i * dimensions + col];
            float b = outMatrix[j * dimensions + col];
            outMatrix[i * dimensions + col] = c * a - s * b;
            outMatrix[j * dimensions + col] = s * a + c * b;
        }
    }
}

#endif
