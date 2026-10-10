// Checks the polytope generators and rotation matrices without needing an OpenGL context.
#include <cmath>
#include <cstdio>
#include <vector>

#include "polytopes.h"
#include "rotation.h"

namespace {

int failures = 0;

void check(bool condition, const char* what, int n) {
    if (!condition) {
        std::printf("FAIL [%dD] %s\n", n, what);
        failures++;
    }
}

bool near(double a, double b, double tolerance = 1e-4) {
    return std::fabs(a - b) < tolerance;
}

double edgeLength(const Polytope& p, int edge) {
    const float* a = &p.lineVertices[(2 * edge) * p.dimensions];
    const float* b = &p.lineVertices[(2 * edge + 1) * p.dimensions];
    double sum = 0.0;
    for (int i = 0; i < p.dimensions; i++) {
        sum += (a[i] - b[i]) * (a[i] - b[i]);
    }
    return std::sqrt(sum);
}

bool allEdgesHaveLength(const Polytope& p, double length) {
    for (int e = 0; e < p.edgeCount(); e++) {
        if (!near(edgeLength(p, e), length)) {
            return false;
        }
    }
    return true;
}

bool centredOnOrigin(const Polytope& p) {
    // Every vertex appears in the same number of edges for these regular polytopes,
    // so the mean of the line endpoints is the mean of the vertices
    for (int axis = 0; axis < p.dimensions; axis++) {
        double sum = 0.0;
        for (int v = 0; v < p.vertexCount(); v++) {
            sum += p.lineVertices[v * p.dimensions + axis];
        }
        if (!near(sum / p.vertexCount(), 0.0)) {
            return false;
        }
    }
    return true;
}

void testHypercube(int n) {
    Polytope p = makeHypercube(n);
    check(p.dimensions == n, "hypercube dimensions", n);
    check(p.edgeCount() == n * (1 << (n - 1)), "hypercube has n * 2^(n-1) edges", n);
    check(allEdgesHaveLength(p, 2.0), "hypercube edges have length 2", n);
    check(centredOnOrigin(p), "hypercube centred on origin", n);
}

void testSimplex(int n) {
    Polytope p = makeSimplex(n);
    check(p.edgeCount() == n * (n + 1) / 2, "simplex joins every pair of its n+1 vertices", n);
    check(allEdgesHaveLength(p, 2.0), "simplex is regular with edge length 2", n);
    check(centredOnOrigin(p), "simplex centred on origin", n);

    // Circumradius of a regular n-simplex with edge length 2 is sqrt(2n / (n+1))
    double expectedRadius = std::sqrt(2.0 * n / (n + 1.0));
    bool onSphere = true;
    for (int v = 0; v < p.vertexCount(); v++) {
        double sum = 0.0;
        for (int axis = 0; axis < n; axis++) {
            double x = p.lineVertices[v * n + axis];
            sum += x * x;
        }
        onSphere = onSphere && near(std::sqrt(sum), expectedRadius);
    }
    check(onSphere, "simplex vertices lie on the circumsphere", n);
}

void testCrossPolytope(int n) {
    Polytope p = makeCrossPolytope(n);
    check(p.edgeCount() == 2 * n * (n - 1), "cross-polytope has 2n(n-1) edges", n);
    check(allEdgesHaveLength(p, std::sqrt(2.0)), "cross-polytope edges have length sqrt(2)", n);
    check(centredOnOrigin(p), "cross-polytope centred on origin", n);
}

bool isOrthogonal(const std::vector<float>& m, int n) {
    for (int a = 0; a < n; a++) {
        for (int b = 0; b < n; b++) {
            double dot = 0.0;
            for (int k = 0; k < n; k++) {
                dot += m[a * n + k] * m[b * n + k];
            }
            if (!near(dot, a == b ? 1.0 : 0.0)) {
                return false;
            }
        }
    }
    return true;
}

void testRotation(int n) {
    std::vector<float> m(n * n);

    buildRotationMatrix({}, n, 1.0f, m.data());
    check(isOrthogonal(m, n) && near(m[0], 1.0), "no planes gives the identity", n);

    // A single XY rotation by 90 degrees sends X to Y
    buildRotationMatrix({ {0, 1, 1.0f} }, n, 3.14159265f / 2.0f, m.data());
    check(near(m[0 * n + 0], 0.0) && near(m[1 * n + 0], 1.0) && near(m[0 * n + 1], -1.0),
          "XY rotation by 90 degrees sends X to Y", n);

    // Every pair of axes, so planes overlap heavily. Writing the plane entries
    // directly into the matrix breaks orthogonality here; composing does not.
    std::vector<RotationPlane> planes;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            planes.push_back({ i, j, 0.3f + 0.1f * (i + j) });
        }
    }
    for (float t : { 0.4f, 1.7f, 12.3f }) {
        buildRotationMatrix(planes, n, t, m.data());
        check(isOrthogonal(m, n), "planes sharing axes still give an orthogonal matrix", n);
    }
}

} // namespace

int main() {
    for (int n = 2; n <= 8; n++) {
        testHypercube(n);
        testSimplex(n);
        testCrossPolytope(n);
        testRotation(n);
    }

    if (failures == 0) {
        std::printf("All geometry tests passed\n");
        return 0;
    }
    std::printf("%d check(s) failed\n", failures);
    return 1;
}
