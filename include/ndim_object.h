#pragma once
#ifndef NDIM_OBJECT_H
#define NDIM_OBJECT_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <glad/glad.h>
#include "polytopes.h"
#include "rotation.h"
#include "shader_s.h"

extern float EDGE_THICKNESS;
extern float VERTEX_SIZE;

struct NDimObject {
    std::string name;             // "4D Hypercube", "5D Simplex", ...
    Polytope geometry;
    std::vector<RotationPlane> rotationPlanes;
    float scale;                  // Uniform scale applied after projecting to 3D
    bool renderEdges = true;      // If false, render only vertices

    // OpenGL resources
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    Shader* shader = nullptr;
    std::string shaderVertPath;
    std::string shaderFragPath;

    NDimObject(std::string name, Polytope geometry, std::vector<RotationPlane> rotationPlanes, float scale,
               std::string shaderVertPath, std::string shaderFragPath)
        : name(std::move(name)), geometry(std::move(geometry)), rotationPlanes(std::move(rotationPlanes)),
          scale(scale), shaderVertPath(std::move(shaderVertPath)), shaderFragPath(std::move(shaderFragPath)) {}

    int dimensions() const { return geometry.dimensions; }
    int matrixSize() const { return dimensions() * dimensions(); }
    int numVec4Groups() const { return (dimensions() + 3) / 4; }
    int stride() const { return dimensions() * sizeof(float); }

    // Initialize buffers and shader. Requires a current OpenGL context.
    void init() {
        setupBuffers();
        shader = new Shader(shaderVertPath.c_str(), shaderFragPath.c_str());
    }

    // outMatrix must be pre-allocated with at least matrixSize() floats
    void buildRotationMatrix(float* outMatrix, float time) const {
        ::buildRotationMatrix(rotationPlanes, dimensions(), time, outMatrix);
    }

    // Setup OpenGL buffers for this object
    void setupBuffers() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, geometry.lineVertices.size() * sizeof(float),
                     geometry.lineVertices.data(), GL_STATIC_DRAW);

        // Coordinates are split across vec4 attributes: xyzw in location 0, the rest in location 1
        int groups = numVec4Groups();
        for (int i = 0; i < groups; i++) {
            glEnableVertexAttribArray(i);

            // The last group can hold fewer than 4 components
            int componentsInThisGroup = (i == groups - 1 && dimensions() % 4 != 0)
                ? dimensions() % 4
                : 4;

            glVertexAttribPointer(
                i,                          // attribute location
                componentsInThisGroup,      // number of components (1-4)
                GL_FLOAT,                   // type
                GL_FALSE,                   // normalized?
                stride(),                   // stride
                reinterpret_cast<void*>(static_cast<uintptr_t>(i * 4 * sizeof(float))) // offset
            );
        }

        // unbind active vao
        glBindVertexArray(0);
    }

    // Draw the object
    void draw() const {
        glBindVertexArray(VAO);

        if (renderEdges) {
            glLineWidth(EDGE_THICKNESS);
            glDrawArrays(GL_LINES, 0, geometry.vertexCount());
        }

        glPointSize(VERTEX_SIZE);
        glDrawArrays(GL_POINTS, 0, geometry.vertexCount());
    }

    // Cleanup OpenGL resources
    void cleanup() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        delete shader;
        shader = nullptr;
    }
};

#endif
