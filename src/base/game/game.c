#include "base/game/game.h"
#include "core/app/app.h"
#include "core/util/logger.h"
#include <stdint.h>

enum materials { MATERIAL_CONTAINER };

static int counter = 0;

err game_init(app *app) {
    err err = CORE_SUCCESS;
    assets *app_assets = NULL;

    err = app_get_assets(&app_assets, app);
    if (err)
        goto err;

    assets_material_add(app_assets, MATERIAL_CONTAINER, "quad",
                        "container.jpg");

    return err;

err:
    logger_log(LOGGER_ERR, "Failed to initialize game", err);
    return err;
}

err game_run(void) {
    err err = CORE_SUCCESS;

    /* somehow draw rectangle with texture and shader */

    // renderer_draw(material, vertices, other data...);

    if (counter % 100 == 0)
        logger_log(LOGGER_DEBUG, "Running the actual game...", 0);

    counter++;
    return err;
}
