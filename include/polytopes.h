#pragma once
#ifndef POLYTOPES_H
#define POLYTOPES_H

#include <vector>

// Wireframe of a convex polytope, laid out for GL_LINES: each edge contributes
// both of its endpoints, and each endpoint is `dimensions` floats.
struct Polytope {
    int dimensions = 0;
    std::vector<float> lineVertices;

    int vertexCount() const { return static_cast<int>(lineVertices.size()) / dimensions; }
    int edgeCount() const { return vertexCount() / 2; }
};

// n-cube with vertices at every combination of +-1: 2^n vertices, n * 2^(n-1) edges
Polytope makeHypercube(int n);

// Regular n-simplex centred on the origin with edge length 2: n+1 vertices, every pair joined
Polytope makeSimplex(int n);

// n-orthoplex with vertices at +-1 on each axis: 2n vertices, every pair joined except opposites
Polytope makeCrossPolytope(int n);

#endif
