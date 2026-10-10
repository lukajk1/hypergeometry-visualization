#pragma once
#ifndef NDIM_OBJECT_H
#define NDIM_OBJECT_H

#include <cstddef>
#include <cstring>
#include <cmath>
#include <glad/glad.h>
#include "shader_s.h"

extern float EDGE_THICKNESS;
extern float VERTEX_SIZE;

struct RotationPlane {
    int axis1;        // First axis (0=X, 1=Y, 2=Z, 3=W, 4=V, etc.)
    int axis2;        // Second axis
    float speed;      // Rotation speed (radians per second)
};

// Helper function to generate NxN identity matrix
// Note: Caller must ensure buffer has enough space (N*N floats)
inline void generateIdentityMatrix(float* matrix, int N) {
    for (int i = 0; i < N * N; i++) {
        matrix[i] = 0.0f;
    }
    for (int i = 0; i < N; i++) {
        matrix[i * N + i] = 1.0f;  // Set diagonal to 1
    }
}

struct NDimObjectData {
    // Vertex data
    const float* vertices;
    size_t vertexDataSize;        // sizeof(vertices) in bytes
    int vertexCount;              // Total number of vertices
    int dimensions;                // N (2, 3, 4, 5, 6, 7, etc.)

    // Rotation configuration
    const RotationPlane* defaultRotationPlanes;
    int numRotationPlanes;
    float* identityMatrix;         // NxN identity matrix for rotation base (generated automatically)

    // Transform
    float scale;                   // Uniform scale factor applied to all dimensions

    // Rendering mode
    bool renderEdges;              // If true, render edges (GL_LINES); if false, render only vertices

    // OpenGL resources
    unsigned int VAO;
    unsigned int VBO;
    Shader* shader;               // Shader program for this object

    // Metadata
    const char* name;             // "Hypercube", "Simplex", "Cross-Polytope"
    const char* shaderVertPath;
    const char* shaderFragPath;

    // Helper functions
    int numVec4Groups() const { return (dimensions + 3) / 4; }
    int stride() const { return dimensions * sizeof(float); }
    int attributeOffset(int groupIndex) const {
        return groupIndex * 4 * sizeof(float);
    }
    int matrixSize() const { return dimensions * dimensions; }

    // Initialize identity matrix based on dimensions
    void initIdentityMatrix() {
        if (identityMatrix != nullptr) {
            generateIdentityMatrix(identityMatrix, dimensions);
        }
    }

    // Initialize everything (identity matrix, buffers, and shader)
    void init() {
        initIdentityMatrix();
        setupBuffers();
        initShader();
    }

    // Build rotation matrix by applying all default rotation planes
    // outMatrix must be pre-allocated with at least matrixSize() floats
    void buildRotationMatrix(float* outMatrix, float time) const {
        // Copy identity matrix as base
        memcpy(outMatrix, identityMatrix, matrixSize() * sizeof(float));

        // Apply all rotation planes
        for (int p = 0; p < numRotationPlanes; p++) {
            const RotationPlane& plane = defaultRotationPlanes[p];
            float planeAngle = time * plane.speed;

            int i = plane.axis1;
            int j = plane.axis2;

            // A rotation in plane ij is the identity except for
            //   [i][i] = cos, [i][j] = -sin, [j][i] = sin, [j][j] = cos
            // Left-multiplying by it only mixes rows i and j, so compose in place.
            // Writing those four entries directly would only be correct when no
            // two planes share an axis.
            float c = cos(planeAngle);
            float s = sin(planeAngle);
            for (int col = 0; col < dimensions; col++) {
                float a = outMatrix[i * dimensions + col];
                float b = outMatrix[j * dimensions + col];
                outMatrix[i * dimensions + col] = c * a - s * b;
                outMatrix[j * dimensions + col] = s * a + c * b;
            }
        }
    }

    // Setup OpenGL buffers for this object
    void setupBuffers() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexDataSize, vertices, GL_STATIC_DRAW);

        // Set up vertex attributes based on dimension
        int numVec4Groups_local = numVec4Groups();

        for (int i = 0; i < numVec4Groups_local; i++) {
            glEnableVertexAttribArray(i);

            // Calculate how many components in this vec4 (could be less than 4 for the last group)
            int componentsInThisGroup = (i == numVec4Groups_local - 1 && dimensions % 4 != 0)
                ? dimensions % 4
                : 4;

            glVertexAttribPointer(
                i,                          // attribute location
                componentsInThisGroup,      // number of components (1-4)
                GL_FLOAT,                   // type
                GL_FALSE,                   // normalized?
                stride(),                   // stride
                (void*)attributeOffset(i)   // offset
            );
        }

        // unbind active vao
        glBindVertexArray(0);
    }

    // Initialize shader from paths
    void initShader() {
        shader = new Shader(shaderVertPath, shaderFragPath);
    }

    // Draw the object
    void draw() const {
        glBindVertexArray(VAO);

        if (renderEdges) {
            glLineWidth(EDGE_THICKNESS);
            glDrawArrays(GL_LINES, 0, vertexCount);
        }

        glPointSize(VERTEX_SIZE);
        glDrawArrays(GL_POINTS, 0, vertexCount);
    }

    // Cleanup OpenGL resources
    void cleanup() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        if (shader != nullptr) {
            delete shader;
            shader = nullptr;
        }
    }
};

#endif
