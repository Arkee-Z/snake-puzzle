#ifndef SNAKE_PUZZLE_GEN_DEPENDENCY_H
#define SNAKE_PUZZLE_GEN_DEPENDENCY_H

#include "snake_puzzle/common.h"

#define SP_DEP_MAX_NODES 16

typedef struct DependencyGraph {
    int node_count;
    bool edge[SP_DEP_MAX_NODES][SP_DEP_MAX_NODES];
    int topo[SP_DEP_MAX_NODES];
    int topo_count;
} DependencyGraph;

void dependency_init(DependencyGraph *graph, int node_count);
void dependency_add_edge(DependencyGraph *graph, int from, int to);
void dependency_build_linear(DependencyGraph *graph, int node_count);
bool dependency_topological_sort(DependencyGraph *graph);
bool dependency_is_acyclic(const DependencyGraph *graph);
int dependency_chain_length(const DependencyGraph *graph);

#endif
