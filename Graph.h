#ifndef GRAPHS_CPP_GRAPH_H
#define GRAPHS_CPP_GRAPH_H

#include <utility>
#include <vector>
#include <sstream>
#include <algorithm>
#include <deque>
#include <queue>

namespace graph {

    // Interface of a graph
    class Graph {
    public:
        virtual void addEdge(int v, int w) = 0; // add edge v-w to graph
        [[nodiscard]] virtual const std::vector<int>& adj(int v) const = 0; // get vertices adjacent to v
        [[nodiscard]] virtual int V() const = 0; // number of vertices
        [[nodiscard]] virtual int E() const = 0; // number of edges
        [[nodiscard]] virtual std::string toString() const = 0; // create string representation
        [[nodiscard]] virtual bool vertexValid(int v) const = 0; // whether a vertex exists
        virtual ~Graph() = default; // virtual destructor
    };

    // Graph built with adjacency list
    class AdjacencyListGraph : public Graph {
    public:
        // create graph with V vertices
        explicit AdjacencyListGraph(int V) :
                numV(V),
                numE(0),
                adjacencies(V, std::vector<int>{}) {}

        // create graph with input stream
        explicit AdjacencyListGraph(std::istream &is) {
            if (!(is >> numV) || numV < 0) throw std::invalid_argument("Invalid format (V)");
            if (!(is >> numE || numE < 0)) throw std::invalid_argument("Invalid format (E)");
            adjacencies.insert(adjacencies.end(), numV, {});
            for (int i = 0; i < numE; ++i) {
                int v1, v2;
                if (!(is >> v1 >> v2)) throw std::invalid_argument("Invalid format (edges)");
                if (v1 < 0 || v1 >= numV || v2 < 0 || v2 >= numV) {
                    throw std::invalid_argument("Cannot create edge v1-v2");
                }
                adjacencies[v1].push_back(v2);
                adjacencies[v2].push_back(v1);
            }
        }

        // add edge v-w to graph
        void addEdge(int v, int w) override {
            if (!vertexValid(v) || !vertexValid(w)) {
                throw std::invalid_argument("Cannot create edge v-w");
            }
            adjacencies[v].push_back(w);
            adjacencies[w].push_back(v);
            ++numE;
        }

        // get vertices adjacent to v
        [[nodiscard]] const std::vector<int>& adj(int v) const override {
            if (!vertexValid(v)) {
                throw std::invalid_argument("Invalid vertex");
            }
            return adjacencies[v];
        }

        // number of vertices
        [[nodiscard]] int V() const override {
            return numV;
        }

        // number of edges
        [[nodiscard]] int E() const override {
            return numE;
        }

        // create string representation
        [[nodiscard]] std::string toString() const override {
            std::stringstream ss;
            ss << "[Graph with " << numV << " vertices and " << numE << " edges]\n";
            for (size_t i = 0; i < adjacencies.size(); ++i) {
                for (const auto edge : adjacencies[i]) {
                    ss << i << "-" << edge << "\n";
                }
            }
            return ss.str();
        }

        // whether a vertex exists
        [[nodiscard]] bool vertexValid(int v) const override {
            return v >= 0 && v < numV;
        }

    private:
        int numV = 0; // vertices
        int numE = 0; // edges
        std::vector<std::vector<int>> adjacencies{}; // vector with V elements each of size degree(vertex) to represent edges

    };

    // degree of a vertex
    int degree(const Graph& g, const int v) {
        if(!g.vertexValid(v)) throw std::invalid_argument("Invalid vertex");
        return g.adj(v).size();
    }

    // maximum degree of any vertex in graph
    int maxDegree(const Graph& g) {
        int deg = 0;
        for(int i=0; i<g.V(); ++i) {
            deg = std::max(deg, static_cast<int>(g.adj(i).size()));
        }

        return deg;
    }

    // average degree of vertices
    double avgDegree(const Graph& g) {
        return 2.0 * g.E() / g.V();
    }

    // count the number of self-loops
    int numSelfLoops(const Graph& g) {
        int num = 0;
        for(int i=0; i<g.V(); ++i) {
            for(const auto& e : g.adj(i)) {
                if(e == i) ++num;
            }
        }
        return num/2; // each self-loop is counted twice
    }

    namespace internal {
        // return whether element v exists in a collection col
        template<typename Col>
        [[nodiscard]] inline bool validIndex(const Col& col, const int v) {
            if(v < 0 || static_cast<size_t>(v) >= col.size()) {
                return false;
            } else {
                return true;
            }
        }

    } // namespace internal

    // Result of a search for all paths reachable from a specific vortex
    class PathsFromVertexResult {
    public:
        // Construct by copying fields
        explicit PathsFromVertexResult(int _start, const std::vector<int>& _distTo, const std::vector<int>& _edgeTo) // NOLINT
                                       :
                                       start(_start), distTo(_distTo), edgeTo(_edgeTo) {}

        // Construct by moving fields
        explicit PathsFromVertexResult(int _start, std::vector<int>&& _distTo, std::vector<int>&& _edgeTo)
                                       :
                                       start(_start), distTo(std::move(_distTo)), edgeTo(std::move(_edgeTo)) {}

        // from which vertex the search was started
        [[nodiscard]] int from() const {
            return start;
        }

        // whether a path from start to v exists
        [[nodiscard]] bool hasPathTo(const int v) const {
            if(!internal::validIndex(distTo, v)) {
                return false;
            } else {
                return distTo[v] != -1;
            }
        }

        // the saved path from start to v or an empty result
        [[nodiscard]] std::vector<int> pathTo(const int v) const {
            if(!hasPathTo(v)) {
                return {};
            }

            // build path in the reverse direction
            std::vector<int> path{v};
            int prev = edgeTo[v];
            while(prev != -1) {
                path.push_back(prev);
                prev = edgeTo[prev];
            }

            std::reverse(path.begin(), path.end());
            return path;
        }

        [[nodiscard]] int distanceTo(const int v) const {
            if(!internal::validIndex(distTo, v)) {
                return -1;
            } else {
                return distTo[v];
            }
        }
    private:
        const int start; // from which vertex the search started
        const std::vector<int> distTo; // number of steps from start to vertex
        const std::vector<int> edgeTo; // from which vertex another vertex was visited from the first time
    };

    namespace depth_first_search {
        namespace internal {
            // Recursive function for depth first search to all vertices from v
            void fromVertexToAllRec(const Graph& g, const int v, // NOLINT
                                    std::vector<int>& distTo, std::vector<int>& edgeTo) {
                for(const int other : g.adj(v)) {
                    if(distTo[other] == -1) {
                        // not yet visited
                        distTo[other] = distTo[v]+1;
                        edgeTo[other] = v; // save that we got to other from v
                        fromVertexToAllRec(g, other, distTo, edgeTo);
                    }
                }
            }
        }

        // Depth-first search from vertex start to all reachable vertices
        PathsFromVertexResult fromVertexToAll(const Graph &g, const int start) {
            if(!g.vertexValid(start)) throw std::invalid_argument("Invalid start vertex");

            std::vector<int> distTo(g.V(), -1); // number of steps from start to vertex
            std::vector<int> edgeTo(g.V(), -1); // from which vertex another vertex was visited from the first time

            distTo[start] = 0;
            // start recursive depth-first start from start
            internal::fromVertexToAllRec(g, start, distTo, edgeTo);

            // return queryable results
            return PathsFromVertexResult(start, std::move(distTo), std::move(edgeTo));
        }
    } // depth_first_search

    namespace breadth_first_search {
        // Breadth-first search from vertex start to all reachable vertices
        PathsFromVertexResult fromVertexToAll(const Graph &g, const int start) {
            if(start < 0 || start >= g.V()) throw std::invalid_argument("Invalid start vertex");

            std::vector<int> distTo(g.V(), -1); // number of steps from start to vertex
            std::vector<int> edgeTo(g.V(), -1); // from which vertex another vertex was visited from the first time

            std::queue<int> queue{};
            queue.push(start);
            distTo[start] = 0;
            while (!queue.empty()) {
                // remove vertex from queue
                const int v = queue.front();
                queue.pop();
                // add all yet unvisited adjacent vertices to queue and mark them
                for (const int other : g.adj(v)) {
                    if (distTo[other] == -1) {
                        // yet unvisited
                        queue.push(other);
                        edgeTo[other] = v;
                        distTo[other] = distTo[v] + 1;
                    }
                }
            }

            // return queryable results
            return PathsFromVertexResult(start, std::move(distTo), std::move(edgeTo));
        }

    }

} // namespace graph

#endif //GRAPHS_CPP_GRAPH_H
