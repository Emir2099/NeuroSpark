#include "pipeline_graph.h"

static PipelineNode nodes[PIPELINE_MAX_NODES];
static PipelineEdge edges[PIPELINE_MAX_EDGES];
static uint32_t next_node_id = 1;
static uint32_t next_edge_id = 1;
static int pipeline_running = 0;

extern void *pmm_alloc_page(void);

void pipeline_init(void) {
    int i;
    for (i = 0; i < PIPELINE_MAX_NODES; i++) {
        nodes[i].active = 0;
    }
    for (i = 0; i < PIPELINE_MAX_EDGES; i++) {
        edges[i].active = 0;
    }
    pipeline_running = 0;
    next_node_id = 1;
    next_edge_id = 1;
}

int pipeline_add_node(uint32_t type, const char *name) {
    int i;
    for (i = 0; i < PIPELINE_MAX_NODES; i++) {
        if (!nodes[i].active) {
            nodes[i].id = next_node_id++;
            nodes[i].type = type;
            nodes[i].active = 1;
            int j = 0;
            while (name[j] && j < 15) {
                nodes[i].name[j] = name[j];
                j++;
            }
            nodes[i].name[j] = '\0';
            return nodes[i].id;
        }
    }
    return -1;
}

int pipeline_link(uint32_t src_id, uint32_t dest_id) {
    int i;
    for (i = 0; i < PIPELINE_MAX_EDGES; i++) {
        if (!edges[i].active) {
            edges[i].id = next_edge_id++;
            edges[i].source_node_id = src_id;
            edges[i].dest_node_id = dest_id;
            edges[i].active = 1;
            edges[i].ring_size = 4096;
            edges[i].ring_buffer = pmm_alloc_page(); // Zero-copy handoff ring
            return edges[i].id;
        }
    }
    return -1;
}

int pipeline_start(void) {
    if (pipeline_running) return 0;
    pipeline_running = 1;
    return 1;
}

int pipeline_stop(void) {
    if (!pipeline_running) return 0;
    pipeline_running = 0;
    return 1;
}

int pipeline_get_nodes(PipelineNode *out_nodes, int max_nodes) {
    int count = 0;
    int i;
    for (i = 0; i < PIPELINE_MAX_NODES && count < max_nodes; i++) {
        if (nodes[i].active) {
            out_nodes[count++] = nodes[i];
        }
    }
    return count;
}

int pipeline_get_edges(PipelineEdge *out_edges, int max_edges) {
    int count = 0;
    int i;
    for (i = 0; i < PIPELINE_MAX_EDGES && count < max_edges; i++) {
        if (edges[i].active) {
            out_edges[count++] = edges[i];
        }
    }
    return count;
}
