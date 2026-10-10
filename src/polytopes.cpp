#include "polytopes.h"

#include <cmath>
#include <utility>

namespace {

using Point = std::vector<float>;

Polytope buildWireframe(int n, const std::vector<Point>& points, const std::vector<std::pair<int, int>>& edges) {
    Polytope polytope;
    polytope.dimensions = n;
    polytope.lineVertices.reserve(edges.size() * 2 * n);
    for (const auto& [a, b] : edges) {
        polytope.lineVertices.insert(polytope.lineVertices.end(), points[a].begin(), points[a].end());
        polytope.lineVertices.insert(polytope.lineVertices.end(), points[b].begin(), points[b].end());
    }
    return polytope;
}

} // namespace

Polytope makeHypercube(int n) {
    // Vertex index bits pick the sign of each coordinate, so neighbours differ by one bit
    const int count = 1 << n;
    std::vector<Point> points(count, Point(n));
    for (int v = 0; v < count; v++) {
        for (int axis = 0; axis < n; axis++) {
            points[v][axis] = ((v >> axis) & 1) ? 1.0f : -1.0f;
        }
    }

    std::vector<std::pair<int, int>> edges;
    for (int v = 0; v < count; v++) {
        for (int axis = 0; axis < n; axis++) {
            int neighbour = v ^ (1 << axis);
            if (v < neighbour) {
                edges.emplace_back(v, neighbour);
            }
        }
    }
    return buildWireframe(n, points, edges);
}

Polytope makeSimplex(int n) {
    // The n+1 standard basis vectors of R^(n+1) form a regular simplex with edge length sqrt(2).
    // Expressing them in the orthonormal Helmert basis of the hyperplane they span,
    //   u_k = (1, ..., 1, -k, 0, ..., 0) / sqrt(k(k+1))   (k ones, k = 1..n),
    // drops the redundant dimension and centres the simplex on the origin.
    // Scaling by sqrt(2) gives edge length 2, matching the other families.
    std::vector<Point> points(n + 1, Point(n, 0.0f));
    for (int k = 1; k <= n; k++) {
        const float norm = std::sqrt(2.0f / (k * (k + 1.0f)));
        for (int v = 0; v < k; v++) {
            points[v][k - 1] = norm;
        }
        points[k][k - 1] = -k * norm;
    }

    std::vector<std::pair<int, int>> edges;
    for (int a = 0; a <= n; a++) {
        for (int b = a + 1; b <= n; b++) {
            edges.emplace_back(a, b);
        }
    }
    return buildWireframe(n, points, edges);
}

Polytope makeCrossPolytope(int n) {
    // Vertex 2*axis sits at +1 on that axis and vertex 2*axis+1 at -1
    std::vector<Point> points(2 * n, Point(n, 0.0f));
    for (int axis = 0; axis < n; axis++) {
        points[2 * axis][axis] = 1.0f;
        points[2 * axis + 1][axis] = -1.0f;
    }

    std::vector<std::pair<int, int>> edges;
    for (int a = 0; a < 2 * n; a++) {
        for (int b = a + 1; b < 2 * n; b++) {
            bool opposite = (a / 2 == b / 2);
            if (!opposite) {
                edges.emplace_back(a, b);
            }
        }
    }
    return buildWireframe(n, points, edges);
}
