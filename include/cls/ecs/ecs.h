/**
 * @file cls/ecs/ecs.h
 * @brief Entity-Component-System management for the Cyclecs library.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * @copyright Copyright (C) 2026 Jagger Harris
 * @see cls/ecs/ecs.c
 */

#ifndef CLS_ECS_H
#define CLS_ECS_H

#include <cls/util/error.h>
#include <cls/util/table.h>
#include <cls/util/types.h>

/* Forward declarations. */
struct cls_array;
struct cls_app;
struct cls_mem;

/**
 * @defgroup ecs Entity-Component-System
 * @ingroup ecs
 * @brief ECS for managing entities, components, and systems. Engine foundation.
 * @{
 */

/**
 * @brief Sentinel value representing an invalid world.
 */
static const u32 CLS_WORLD_MAX = U32_MAX;

/**
 * @brief Sentinel value representing an invalid entity.
 */
static const u32 CLS_ENTITY_MAX = U32_MAX;

/**
 * @brief Sentinel value representing an invalid component.
 */
static const u32 CLS_COMPONENT_MAX = U32_MAX;

/**
 * @brief Sentinel value representing an invalid singleton.
 */
static const u32 CLS_SINGLETON_MAX = U32_MAX;

/**
 * @brief Sentinel value representing an invalid system.
 */
static const u32 CLS_SYSTEM_MAX = U32_MAX;

/**
 * @brief Entity identifier.
 *
 * A value of CLS_WORLD_MAX represents an invalid world.
 */
typedef u32 cls_world;

/**
 * @brief Entity identifier.
 *
 * A value of CLS_ENTITY_MAX represents an invalid entity.
 */
typedef u32 cls_entity;

/**
 * @brief Component identifier.
 *
 * A value of CLS_COMPONENT_MAX represents an invalid component.
 */
typedef u32 cls_component;

/**
 * @brief Singleton identifier.
 *
 * A value of CLS_SINGLETON_MAX represents an invalid singleton.
 */
typedef u32 cls_singleton;

/**
 * @brief System identifier.
 *
 * A value of CLS_SYSTEM_MAX represents an invalid system.
 */
typedef u32 cls_system;

/**
 * @struct cls_ecs
 * @brief Entity component system.
 */
struct cls_ecs;

/**
 * @struct cls_ecs_world
 * @brief ECS world.
 */
struct cls_ecs_world;

/**
 * @struct cls_ecs_world_query
 * @brief ECS query.
 */
struct cls_ecs_world_query;

/**
 * @brief Callback invoked for each world.
 *
 * @param[in] world     Current world.
 * @param[in] user_data User-defined data.
 *
 * @return CLS_SUCCESS On success.
 * @retval (error)     If the callback reports an error. Iteration continues.
 */
typedef cls_error (*cls_ecs_world_iter_fn)(struct cls_ecs_world *world,
                                           void *user_data);

/**
 * @brief Callback invoked by a system.
 *
 * If the system was registered with a query, `query` contains the matching
 * entities. Otherwise, `query` is NULL.
 *
 * @param[in] query System query, or NULL.
 * @param[in] app   Application state.
 *
 * @return CLS_SUCCESS On success.
 * @retval (error)     If the system fails.
 */
typedef cls_error (*cls_ecs_world_system_fn)(struct cls_ecs_world_query *query,
                                             struct cls_app *app);

/**
 * @brief Creates an ECS.
 *
 * Allocates an ECS and initializes its world table. Destroy the returned ECS
 * with cls_ecs_destroy().
 *
 * @param[out] ecs ECS.
 * @param[in]  mem Memory allocator.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR       If `ecs` or `mem` is NULL.
 * @retval CLS_OUT_OF_MEMORY If allocation fails.
 *
 * @code
 * struct cls_ecs *ecs;
 * cls_ecs_create(&ecs, mem);
 * // Use ecs.
 * cls_ecs_destroy(ecs);
 * @endcode
 */
cls_error cls_ecs_create(struct cls_ecs **ecs, struct cls_mem *mem);

/**
 * @brief Destroys an ECS.
 *
 * Destroys all worlds and releases the world table.
 *
 * @param[in] ecs ECS to destroy.
 */
void cls_ecs_destroy(struct cls_ecs *ecs);

/**
 * @brief Adds a world.
 *
 * Creates a world and adds it to the ECS.
 *
 * @param[out] world         World. May be NULL.
 * @param[in]  ecs           ECS.
 * @param[in]  id            World identifier.
 * @param[in]  should_update Whether the world is updated by
 *                           cls_ecs_world_update_all().
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `ecs` is NULL.
 *
 * @code
 * cls_world id = 0;
 * struct cls_ecs_world *world;
 * cls_ecs_world_add(&world, ecs, id, true);
 * @endcode
 */
cls_error cls_ecs_world_add(struct cls_ecs_world **world, struct cls_ecs *ecs,
                            cls_world id, bool should_update);

/**
 * @brief Removes a world.
 *
 * Removes the world identified by `id`.
 *
 * @param[in] ecs ECS.
 * @param[in] id  World identifier.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `ecs` is NULL.
 * @retval (error)     If the world is not found.
 */
cls_error cls_ecs_world_remove(struct cls_ecs *ecs, cls_world id);

/**
 * @brief Retrieves a world.
 *
 * @param[out] world World.
 * @param[in]  ecs   ECS.
 * @param[in]  id    World identifier.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `world` or `ecs` is NULL.
 * @retval (error)     If the world is not found.
 */
cls_error cls_ecs_world_get(struct cls_ecs_world **world,
                            const struct cls_ecs *ecs, cls_world id);

/**
 * @brief Updates all worlds.
 *
 * Updates each world registered with the ECS.
 *
 * @param[in] ecs ECS.
 * @param[in] app Application state.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `ecs` is NULL.
 */
cls_error cls_ecs_world_update_all(struct cls_ecs *ecs, struct cls_app *app);

/**
 * @brief Iterates over all worlds.
 *
 * Calls `fn` for each world.
 *
 * @param[in] ecs       ECS.
 * @param[in] fn        Callback.
 * @param[in] user_data User-defined data.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `ecs` or `fn` is NULL.
 */
cls_error cls_ecs_world_iter_all(struct cls_ecs *ecs, cls_ecs_world_iter_fn fn,
                                 void *user_data);

/**
 * @brief Flushes pending entity removals.
 *
 * Removes all entities queued for deletion.
 *
 * @param[in] world World.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `world` is NULL.
 */
cls_error cls_ecs_world_flush(struct cls_ecs_world *world);

/**
 * @brief Creates an entity.
 *
 * @param[out] e     Entity.
 * @param[in]  world World.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `e` or `world` is NULL.
 */
cls_error cls_ecs_world_entity_add(cls_entity *e, struct cls_ecs_world *world);

/**
 * @brief Queues an entity for removal.
 *
 * The entity is removed during the next flush.
 *
 * @param[in] world World.
 * @param[in] e     Entity.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `world` is NULL.
 * @retval CLS_INVALID_ARG If `e` is CLS_ENTITY_MAX.
 */
cls_error cls_ecs_world_entity_remove(struct cls_ecs_world *world,
                                      cls_entity e);

/**
 * @brief Registers a component type.
 *
 * Registers a component type with the specified size.
 *
 * @param[in] world     World.
 * @param[in] id        Component type identifier.
 * @param[in] comp_size Size of a component, in bytes.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `world` is NULL.
 * @retval CLS_OUT_OF_MEMORY If allocation fails.
 */
cls_error cls_ecs_world_component_type_add(struct cls_ecs_world *world,
                                           cls_component id, size_t comp_size);

/**
 * @brief Removes a component type.
 *
 * @param[in] world World.
 * @param[in] id    Component type identifier.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `world` is NULL.
 * @retval (error)     If the component type is not found.
 */
cls_error cls_ecs_world_component_type_remove(struct cls_ecs_world *world,
                                              cls_component id);

/**
 * @brief Adds a component to an entity.
 *
 * Copies the component into the world.
 *
 * @param[in] world World.
 * @param[in] e     Entity.
 * @param[in] id    Component type identifier.
 * @param[in] comp  Component data.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `world` or `comp` is NULL.
 * @retval CLS_INVALID_ARG If `e` or 'id' is CLS_ENTITY_MAX or CLS_COMPONENT_MAX
 *                         respectively, or is out of range.
 * @retval (error)         If the component type does not exist.
 */
cls_error cls_ecs_world_component_add(struct cls_ecs_world *world, cls_entity e,
                                      cls_component id, const void *comp);

/**
 * @brief Removes a component from an entity.
 *
 * @param[in] world World.
 * @param[in] e     Entity.
 * @param[in] id    Component type identifier.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `world` is NULL.
 * @retval CLS_INVALID_ARG If `e` or 'id' is CLS_ENTITY_MAX or CLS_COMPONENT_MAX
 *                         respectively, or is out of range.
 */
cls_error cls_ecs_world_component_remove(struct cls_ecs_world *world,
                                         cls_entity e, cls_component id);

/**
 * @brief Retrieves a component from an entity.
 *
 * @param[out] comp  Component.
 * @param[in]  world World.
 * @param[in]  e     Entity.
 * @param[in]  id    Component type identifier.
 *
 * @return CLS_SUCCESS     On success.
 * @retval CLS_NULLPTR     If `comp` or `world` is NULL.
 * @retval CLS_INVALID_ARG If `e` or 'id' is CLS_ENTITY_MAX or CLS_COMPONENT_MAX
 *                         respectively, or is out of range.
 */
cls_error cls_ecs_world_component_get(void **comp,
                                      const struct cls_ecs_world *world,
                                      cls_entity e, cls_component id);

/**
 * @brief Creates a query.
 *
 * Creates a query that matches entities with all specified component types.
 * Destroy the returned query with cls_ecs_world_query_destroy().
 *
 * @param[out] query      Query.
 * @param[in]  world      World.
 * @param[in]  comp_count Number of component type identifiers.
 * @param[in]  ids        Component type identifiers.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `query`, `world`, or `ids` is NULL.
 * @retval CLS_INVALID_ARG   If `comp_count` is 0 or a component type does
 * not exist.
 * @retval CLS_OUT_OF_MEMORY If allocation fails.
 *
 * @code
 * const cls_component ids[] = {CLS_COMP_TRANSFORM, CLS_COMP_CAMERA};
 * struct cls_ecs_world_query *query;
 * cls_ecs_world_query_create(&query, world, 2, ids);
 * // Use query.
 * cls_ecs_world_query_destroy(query);
 * @endcode
 */
cls_error cls_ecs_world_query_create(struct cls_ecs_world_query **query,
                                     struct cls_ecs_world *world,
                                     size_t comp_count,
                                     const cls_component ids[]);

/**
 * @brief Destroys a query.
 *
 * Releases the query.
 *
 * @param[in] query Query.
 */
void cls_ecs_world_query_destroy(struct cls_ecs_world_query *query);

/**
 * @brief Retrieves a query's world.
 *
 * @param[out] world World.
 * @param[in]  query Query.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `world` or `query` is NULL.
 */
cls_error cls_ecs_world_query_world_get(struct cls_ecs_world **world,
                                        struct cls_ecs_world_query *query);

/**
 * @brief Resets a query.
 *
 * Resets the query to the beginning.
 *
 * @param[in] query Query.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `query` is NULL.
 */
cls_error cls_ecs_world_query_clear(struct cls_ecs_world_query *query);

/**
 * @brief Retrieves the next matching entity.
 *
 * Advances the query and returns the next matching entity. If no entities
 * remain, `*e` is set to CLS_ENTITY_MAX.
 *
 * @param[out] e     Matching entity, or CLS_ENTITY_MAX.
 * @param[out] comps Optional array of component pointers.
 * @param[in]  query Query.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_INVALID_ARG If `e`, `query`, or the query is invalid.
 *
 * @code
 * cls_entity e;
 * void *comps[2];
 *
 * while (cls_ecs_world_query_next(&e, comps, query) == CLS_SUCCESS &&
 *        e != CLS_ENTITY_MAX) {
 *     // Use comps[0] and comps[1].
 * }
 * @endcode
 */
cls_error cls_ecs_world_query_next(cls_entity *e, void **comps,
                                   struct cls_ecs_world_query *query);

/**
 * @brief Registers a system.
 *
 * Registers a system to run at the specified tick rate. If component types
 * are provided, a query is created and passed to the system when it runs.
 *
 * @param[in] world      World.
 * @param[in] id         System identifier.
 * @param[in] system     System callback.
 * @param[in] tick_rate  Tick rate.
 * @param[in] comp_count Number of component type identifiers. May be 0.
 * @param[in] ids        Component type identifiers. May be NULL if
 *                       `comp_count` is 0.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `world` is NULL.
 * @retval (error)     If query creation or system registration fails.
 */
cls_error cls_ecs_world_system_add(struct cls_ecs_world *world, cls_system id,
                                   cls_ecs_world_system_fn system,
                                   float tick_rate, size_t comp_count,
                                   const cls_component ids[]);

/**
 * @brief Removes a system.
 *
 * @param[in] world World.
 * @param[in] id    System identifier.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `world` is NULL.
 * @retval (error)     If the system is not found.
 */
cls_error cls_ecs_world_system_remove(struct cls_ecs_world *world,
                                      cls_system id);

/**
 * @brief Adds or updates a singleton.
 *
 * Copies the singleton into the world. If a singleton with the same identifier
 * already exists, it is replaced.
 *
 * @param[in] world World.
 * @param[in] id    Singleton identifier.
 * @param[in] data  Singleton data.
 * @param[in] size  Size of the data, in bytes.
 *
 * @return CLS_SUCCESS       On success.
 * @retval CLS_NULLPTR       If `world` or `data` is NULL.
 * @retval CLS_OUT_OF_MEMORY If allocation fails.
 */
cls_error cls_ecs_world_singleton_add(struct cls_ecs_world *world,
                                      cls_singleton id, const void *data,
                                      size_t size);

/**
 * @brief Removes a singleton.
 *
 * @param[in] world World.
 * @param[in] id    Singleton identifier.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `world` is NULL.
 * @retval (error)     If the singleton is not found.
 */
cls_error cls_ecs_world_singleton_remove(struct cls_ecs_world *world,
                                         cls_singleton id);

/**
 * @brief Retrieves a singleton.
 *
 * @param[out] out   Singleton data.
 * @param[in]  world World.
 * @param[in]  id    Singleton identifier.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR     If `out` or `world` is NULL.
 * @retval CLS_INVALID_ARG If the singleton is not found.
 */
cls_error cls_ecs_world_singleton_get(void **out,
                                      const struct cls_ecs_world *world,
                                      cls_singleton id);

/**
 * @brief Updates a world.
 *
 * Runs all registered systems and flushes pending entity removals. Does
 * nothing if updates are disabled for the world.
 *
 * @param[in] world World.
 * @param[in] app   Application state.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `world` or `app` is NULL.
 * @retval (error)     If running systems or flushing entities fails.
 */
cls_error cls_ecs_world_update(struct cls_ecs_world *world,
                               struct cls_app *app);

/**
 * @brief Retrieves the number of allocated entities.
 *
 * Includes entities queued for removal and entity identifiers available for
 * reuse.
 *
 * @param[out] len   Number of allocated entities.
 * @param[in]  world World.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `len` or `world` is NULL.
 */
cls_error cls_ecs_world_entities_length_get(size_t *len,
                                            const struct cls_ecs_world *world);

/**
 * @brief Retrieves the number of reusable entity identifiers.
 *
 * @param[out] len   Number of reusable entity identifiers.
 * @param[in]  world World.
 *
 * @return CLS_SUCCESS On success.
 * @retval CLS_NULLPTR If `len` or `world` is NULL.
 */
cls_error
cls_ecs_world_free_entities_length_get(size_t *len,
                                       const struct cls_ecs_world *world);

/** @} */

#endif // CLS_ECS_H
