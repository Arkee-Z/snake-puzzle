#include "snake_puzzle/gen/dependency.h"

#include <string.h>

void dependency_init(DependencyGraph *graph, int node_count)
{
    int i;

    memset(graph, 0, sizeof(*graph));
    graph->node_count = sp_clamp_int(node_count, 0, SP_DEP_MAX_NODES);
    for (i = 0; i < graph->node_count; ++i) {
        graph->topo[i] = i;
    }
    graph->topo_count = graph->node_count;
}

void dependency_add_edge(DependencyGraph *graph, int from, int to)
{
    if (from < 0 || to < 0 || from >= graph->node_count || to >= graph->node_count ||
        from == to) {
        return;
    }
    graph->edge[from][to] = true;
}

void dependency_build_linear(DependencyGraph *graph, int node_count)
{
    int i;

    dependency_init(graph, node_count);
    for (i = 0; i + 1 < graph->node_count; ++i) {
        dependency_add_edge(graph, i, i + 1);
    }
}

bool dependency_topological_sort(DependencyGraph *graph)
{
    int indegree[SP_DEP_MAX_NODES];
    int queue[SP_DEP_MAX_NODES];
    int head = 0;
    int tail = 0;
    int i;
    int j;

    memset(indegree, 0, sizeof(indegree));
    graph->topo_count = 0;
    for (i = 0; i < graph->node_count; ++i) {
        for (j = 0; j < graph->node_count; ++j) {
            if (graph->edge[i][j]) {
                indegree[j] += 1;
            }
        }
    }
    for (i = 0; i < graph->node_count; ++i) {
        if (indegree[i] == 0) {
            queue[tail++] = i;
        }
    }
    while (head < tail) {
        int node = queue[head++];
        graph->topo[graph->topo_count++] = node;
        for (j = 0; j < graph->node_count; ++j) {
            if (graph->edge[node][j]) {
                indegree[j] -= 1;
                if (indegree[j] == 0) {
                    queue[tail++] = j;
                }
            }
        }
    }
    return graph->topo_count == graph->node_count;
}

bool dependency_is_acyclic(const DependencyGraph *graph)
{
    DependencyGraph copy = *graph;

    return dependency_topological_sort(&copy);
}

int dependency_chain_length(const DependencyGraph *graph)
{
    return graph->topo_count;
}
