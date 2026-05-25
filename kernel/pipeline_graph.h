#ifndef PIPELINE_GRAPH_H
#define PIPELINE_GRAPH_H

typedef unsigned int uint32_t;

#define PIPELINE_MAX_NODES 16
#define PIPELINE_MAX_EDGES 32

enum {
    NODE_TYPE_SENSOR = 0,
    NODE_TYPE_AUDIO_OUT = 1,
    NODE_TYPE_VISION_IN = 2,
    NODE_TYPE_SPIKE_ENGINE = 3
};

typedef struct {
    uint32_t id;
    uint32_t type;
    uint32_t active;
    char name[16];
} PipelineNode;

typedef struct {
    uint32_t id;
    uint32_t source_node_id;
    uint32_t dest_node_id;
    uint32_t active;
    void *ring_buffer; // Pointer to physical page
    uint32_t ring_size;
} PipelineEdge;

void pipeline_init(void);
int pipeline_add_node(uint32_t type, const char *name);
int pipeline_link(uint32_t src_id, uint32_t dest_id);
int pipeline_start(void);
int pipeline_stop(void);
int pipeline_get_nodes(PipelineNode *out_nodes, int max_nodes);
int pipeline_get_edges(PipelineEdge *out_edges, int max_edges);

#endif
