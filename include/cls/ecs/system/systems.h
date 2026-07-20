/**
 * @file cls/ecs/system/systems.h
 * @brief Default systems for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/ecs/system/systems.c
 */

#ifndef CLS_SYSTEMS_H
#define CLS_SYSTEMS_H

#include <cls/util/error.h>

/* Forward declarations. */
struct cls_app;
struct cls_ecs_world_query;

/**
 * @defgroup systems Systems
 * @ingroup ecs
 * @brief Default system helpers.
 * @{
 */

cls_error cls_button_system(struct cls_ecs_world_query *query,
                            struct cls_app *app);
cls_error cls_camera_system(struct cls_ecs_world_query *query,
                            struct cls_app *app);
cls_error cls_label_render_system(struct cls_ecs_world_query *query,
                                  struct cls_app *app);
cls_error cls_render_system(struct cls_ecs_world_query *query,
                            struct cls_app *app);

/** @} */

#endif // CLS_SYSTEMS_H
