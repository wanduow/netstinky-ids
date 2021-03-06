/*
 *
 * Copyright (c) 2020 The University of Waikato, Hamilton, New Zealand.
 *
 * This file is part of netstinky-ids.
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/BSD-2-Clause
 *
 *
 */
#include <stdlib.h>
#include <string.h>

#include "ids_storedvalues.h"

/* DECLARATIONS */

/**
 * Initialize a pre-allocated ioc_value.
 * @returns: 0 if successful.
 */
static int
init_ids_ioc_value(ids_ioc_value_t *value, int botnet_id);

/**
 * Finalize an ioc_value.
 */
static void
fini_ids_ioc_value(ids_ioc_value_t *value);

/* DEFINITIONS */

ids_ioc_value_t *
new_ids_ioc_value(int botnet_id)
{
    int rc;
    ids_ioc_value_t *value = NULL;
    value = malloc(sizeof(*value));
    if (!value) return NULL;

    rc = init_ids_ioc_value(value, botnet_id);
    if (rc)
    {
        free(value);
        return NULL;
    }

    return value;
}

void
free_ids_ioc_value(ids_ioc_value_t *value)
{
    fini_ids_ioc_value(value);
    if (value) free(value);
}

static int
init_ids_ioc_value(ids_ioc_value_t *value, int botnet_id)
{
    if (!value) return -1;

    value->botnet_id = botnet_id;
    return 0;
}

static void
fini_ids_ioc_value(ids_ioc_value_t *value)
{
    if (!value) return;
    memset(value, 0, sizeof(*value));
}
