#include "sulfur/shared/path.h"

const char *sf_path_name(const sf_path *path) {
    if (path->is_global || path->segment_count != 1) {
        return NULL;
    }

    return path->segments[0].name;
}
