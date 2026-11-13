#include "sulfur/pipeline/frontend/semantic/namespace.h"

#include <string.h>

#include "sulfur/pipeline/frontend/semantic/semantic_internal.h"
#include "sulfur/pipeline/frontend/semantic/symbol.h"
#include "sulfur/utils/string.h"

static bool append_segment(
    char *buf, size_t cap, size_t *len, bool *first, const char *name
) {
    const char *sep = *first ? "" : "::";
    size_t sep_len = strlen(sep);
    size_t name_len = strlen(name);

    if (sep_len + name_len >= cap - *len) {
        return false;
    }

    memcpy(buf + *len, sep, sep_len);
    *len += sep_len;
    memcpy(buf + *len, name, name_len);
    *len += name_len;
    buf[*len] = '\0';

    *first = false;
    return true;
}

bool sf_namespace_push_path(sf_namespace_stack *ns, const sf_path *path) {
    if (ns->depth + path->segment_count > SF_NS_MAX_DEPTH) {
        return false;
    }

    for (size_t i = 0; i < path->segment_count; i++) {
        ns->segments[ns->depth++] = path->segments[i].name;
    }

    return true;
}

bool sf_namespace_format(
    char *buf, size_t cap, const sf_namespace_stack *ns, size_t prefix_depth,
    const sf_path_segment *segs, size_t seg_count
) {
    if (cap == 0) {
        return false;
    }

    size_t len = 0;
    bool first = true;
    buf[0] = '\0';

    for (size_t i = 0; i < prefix_depth; i++) {
        if (!append_segment(buf, cap, &len, &first, ns->segments[i])) {
            return false;
        }
    }

    for (size_t i = 0; i < seg_count; i++) {
        if (!append_segment(buf, cap, &len, &first, segs[i].name)) {
            return false;
        }
    }

    return true;
}

const sf_func_symbol *sf_semantic_resolve_func(
    sf_semantic *s, const sf_path *path
) {
    char buf[SF_QNAME_MAX];
    size_t k = path->is_global ? 0 : s->ns.depth;

    for (;;) {
        if (sf_namespace_format(
                buf, sizeof buf, &s->ns, k, path->segments, path->segment_count
            )) {
            const sf_func_symbol *sym =
                sf_func_symbol_table_lookup(s->funcs, buf);

            if (sym != NULL) {
                return sym;
            }
        }

        if (k == 0) {
            return NULL;
        }

        k--;
    }
}

const char *sf_semantic_suggest_func(sf_semantic *s, const sf_path *path) {
    char typed[SF_QNAME_MAX];

    if (!sf_namespace_format(
            typed, sizeof typed, &s->ns, 0, path->segments, path->segment_count
        )) {
        return NULL;
    }

    char prefix[SF_QNAME_MAX];
    size_t prefix_len = 0;

    if (s->ns.depth > 0 && !path->is_global) {
        if (!sf_namespace_format(
                prefix, sizeof prefix, &s->ns, s->ns.depth, NULL, 0
            )) {
            return NULL;
        }

        prefix_len = strlen(prefix);
    }

    const char *best = NULL;
    size_t best_distance = SIZE_MAX;

    for (size_t i = 0; i < s->funcs->count; i++) {
        const char *full = s->funcs->functions[i].name;
        const char *candidates[2] = {full, NULL};

        if (prefix_len > 0 && strncmp(full, prefix, prefix_len) == 0 &&
            full[prefix_len] == ':' && full[prefix_len + 1] == ':') {
            candidates[1] = full + prefix_len + 2;
        }

        for (size_t c = 0; c < 2; c++) {
            if (candidates[c] == NULL) {
                continue;
            }

            size_t d = sf_string_distance(candidates[c], typed);

            if (d < best_distance) {
                best_distance = d;
                best = candidates[c];
            }
        }
    }

    if (best == NULL || best_distance > strlen(typed) / 2) {
        return NULL;
    }

    return best;
}
