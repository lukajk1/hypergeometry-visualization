#pragma once
#ifndef SHAPE_CATALOG_H
#define SHAPE_CATALOG_H

#include "ndim_object.h"

constexpr int MIN_DIMENSION = 2;
constexpr int MAX_DIMENSION = 8;

// Order matches the "Family" dropdown in the UI
enum ShapeFamily { HYPERCUBE = 0, SIMPLEX = 1, CROSS_POLYTOPE = 2, SHAPE_FAMILY_COUNT };

extern const char* SHAPE_FAMILY_NAMES[SHAPE_FAMILY_COUNT];

// Builds every family in dimensions 2-8. Requires a current OpenGL context.
void initializeObjects();
void cleanUpObjects();

// Returns nullptr when the family or dimension is out of range
NDimObject* getObject(int family, int dimension);

#endif
