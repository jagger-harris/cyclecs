#ifndef ECS_COMPONENT_NODE_H
#define ECS_COMPONENT_NODE_H

#include "core/util/array.h"
#include "core/util/types.h"
#include <cglm/cglm.h>

struct node {
    struct array children;
    u64 parent;
};

#endif // ECS_COMPONENT_NODE_H
