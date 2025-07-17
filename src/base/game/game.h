#ifndef GAME_H
#define GAME_H

#include "core/app/assets.h"
#include "core/ecs/ecs.h"
#include "core/gfx/renderer.h"
#include "core/util/err.h"

err game_init(struct assets *assets, struct ecs *ecs,
              struct renderer *renderer);
err game_run(void);

#endif // GAME_H
