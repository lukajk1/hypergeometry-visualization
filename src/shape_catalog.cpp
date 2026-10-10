#include "shape_catalog.h"

#include <memory>
#include <string>

const char* SHAPE_FAMILY_NAMES[SHAPE_FAMILY_COUNT] = { "Hypercube", "Simplex", "Cross-Polytope" };

namespace {

constexpr int DIMENSION_COUNT = MAX_DIMENSION - MIN_DIMENSION + 1;

// Rotation planes per dimension, indexed by dimension - MIN_DIMENSION
const std::vector<RotationPlane> ROTATIONS[DIMENSION_COUNT] = {
    { {0, 1, 0.5f} },                                                 // 2D: XY
    { {0, 1, 0.4f} },                                                 // 3D: XY
    { {1, 3, 0.7f} },                                                 // 4D: YW
    { {1, 3, 0.5f}, {0, 4, 0.3f} },                                   // 5D: YW, XV
    { {1, 3, 0.45f}, {0, 4, 0.35f}, {2, 5, 0.28f} },                  // 6D: YW, XV, ZU
    { {1, 3, 0.4f}, {0, 4, 0.3f}, {2, 5, 0.25f} },                    // 7D: YW, XV, ZU
    { {1, 3, 0.38f}, {0, 4, 0.28f}, {2, 5, 0.23f}, {6, 7, 0.18f} },   // 8D: YW, XV, ZU, 7-8
};

// Each projection step shrinks the shape, so higher dimensions need a larger scale to fill the view
const float SCALES[SHAPE_FAMILY_COUNT][DIMENSION_COUNT] = {
    { 1.0f, 0.65f, 1.5f, 4.0f, 10.0f, 25.0f, 55.0f },     // Hypercube
    { 1.0f, 1.2f, 3.35f, 7.9f, 28.5f, 128.0f, 285.0f },   // Simplex
    { 1.0f, 1.2f, 3.5f, 8.0f, 30.0f, 120.0f, 300.0f },    // Cross-Polytope
};

Polytope makePolytope(int family, int dimension) {
    switch (family) {
    case HYPERCUBE: return makeHypercube(dimension);
    case SIMPLEX: return makeSimplex(dimension);
    default: return makeCrossPolytope(dimension);
    }
}

std::unique_ptr<NDimObject> objects[SHAPE_FAMILY_COUNT][DIMENSION_COUNT];

} // namespace

void initializeObjects() {
    for (int family = 0; family < SHAPE_FAMILY_COUNT; family++) {
        for (int dimension = MIN_DIMENSION; dimension <= MAX_DIMENSION; dimension++) {
            int d = dimension - MIN_DIMENSION;
            auto object = std::make_unique<NDimObject>(
                std::to_string(dimension) + "D " + SHAPE_FAMILY_NAMES[family],
                makePolytope(family, dimension),
                ROTATIONS[d],
                SCALES[family][d]);
            object->init();
            objects[family][d] = std::move(object);
        }
    }
}

void cleanUpObjects() {
    for (auto& family : objects) {
        for (auto& object : family) {
            if (object) {
                object->cleanup();
                object.reset();
            }
        }
    }
}

NDimObject* getObject(int family, int dimension) {
    if (family < 0 || family >= SHAPE_FAMILY_COUNT || dimension < MIN_DIMENSION || dimension > MAX_DIMENSION) {
        return nullptr;
    }
    return objects[family][dimension - MIN_DIMENSION].get();
}
