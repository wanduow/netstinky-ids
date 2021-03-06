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
/*
 * Set up MDNS using the Avahi library
 *
 * Publishes the connection details of the IDS server which transmits IDS
 * events.
 *
 */
#include "ids_mdns_avahi.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avahi-common/alternative.h>
#include <avahi-common/domain.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>

#include "utils/logging.h"
#include "error/ids_error.h"

void create_services(AvahiMdnsContext *mdns);

/**
 * Run on an AvahiMdnsContext when the service name clashes with another
 * service advertising on the network.
 *
 * It will replace the name field in the AvahiMdnsContext.
 */
static bool AvahiMdnsContext_use_alternative_service_name(AvahiMdnsContext *mdns)
{
    char *n;

    assert(mdns);
    assert(mdns->name);
    assert(avahi_is_valid_service_name(mdns->name));

    // This can fail if c->name is not a valid service name or memory cannot be
    // allocated
    n = avahi_alternative_service_name(mdns->name);
    if (!n) return false;

    avahi_free(mdns->name);
    mdns->name = n;

    return true;
}

static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, void *userdata)
{
    AvahiMdnsContext *mdns = (AvahiMdnsContext *)userdata;
    assert(mdns);
    assert(g == mdns->group || mdns->group == NULL);
    mdns->group = g;

    switch(state)
    {
    case AVAHI_ENTRY_GROUP_ESTABLISHED:
        break;
    case AVAHI_ENTRY_GROUP_COLLISION:
        if (!AvahiMdnsContext_use_alternative_service_name(mdns))
        {
            logger(L_ERROR,
                   "Could not use alternative service name for service '%s'",
                   mdns->name);
        }
        else
        {
            logger(L_WARN, "Using alternative service name: '%s'", mdns->name);
            create_services(mdns);
        }
        break;
    case AVAHI_ENTRY_GROUP_FAILURE:
        logger(L_ERROR, "entry_group_callback: %s",
                avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
        break;
    case AVAHI_ENTRY_GROUP_UNCOMMITED:
        break;
    case AVAHI_ENTRY_GROUP_REGISTERING:
        break;
    }
}

/**
 * Sets up all services when the client state changes to running.
 */
void create_services(AvahiMdnsContext *mdns)
{
    int ret;

    // Check if first time running
    if (!mdns->group)
        if (!(mdns->group = avahi_entry_group_new(mdns->client, entry_group_callback, mdns)))
        {
            logger(L_ERROR, "create_services: %s",
                    avahi_strerror(avahi_client_errno(mdns->client)));
            goto error;
        }

    if (avahi_entry_group_is_empty(mdns->group))
    {
        if (0 > (ret = avahi_entry_group_add_service(mdns->group, AVAHI_IF_UNSPEC,
                AVAHI_PROTO_UNSPEC, 0, mdns->name, MDNS_SVC_NAME, NULL, NULL, mdns->port, NULL)))
        {
            if (AVAHI_ERR_COLLISION == ret)
                goto collision;
            logger(L_ERROR, "create_services: %s", avahi_strerror(ret));
            goto error;
        }
    }

    if (0 > (ret = avahi_entry_group_commit(mdns->group)))
    {
        logger(L_ERROR, "create_services: %s", avahi_strerror(ret));
        goto error;
    }

    return;
collision:
    AvahiMdnsContext_use_alternative_service_name(mdns);

    logger(L_WARN, "Renaming service to %s", mdns->name);
    avahi_entry_group_reset(mdns->group);
    create_services(mdns);
    return;

error:
    return;
}

/**
 * Callback for when AvahiClient changes state.
 */
static void
client_callback(AvahiClient *c, AvahiClientState state, void *userdata)
{
    AvahiMdnsContext *mdns = (AvahiMdnsContext *)userdata;

    // The first time this runs, client may not have been assigned yet
    if (!mdns->client) mdns->client = c;

    switch(state)
    {
    case AVAHI_CLIENT_S_RUNNING:
        logger(L_DEBUG, "AVAHI_CLIENT_S_RUNNING");
        create_services(mdns);
        break;
    case AVAHI_CLIENT_FAILURE:
        logger(L_ERROR, "client_callback: %s", avahi_strerror(avahi_client_errno(c)));
        break;
    case AVAHI_CLIENT_S_COLLISION:
        logger(L_WARN, "AVAHI_CLIENT_S_COLLISION");
        /* fallthrough */
    case AVAHI_CLIENT_S_REGISTERING:
        logger(L_INFO, "AVAHI_CLIENT_S_REGISTERING");
        if (mdns->group) avahi_entry_group_reset(mdns->group);
        break;
    case AVAHI_CLIENT_CONNECTING:
        logger(L_INFO, "AVAHI_CLIENT_CONNECTING");
        break;
    }
}

int ids_mdns_setup_mdns(AvahiMdnsContext *mdns, uv_loop_t *loop, int port)
{
    assert(mdns);
    assert(port > 0);

    int error;

    mdns->name = avahi_strdup("NetStinky");
    mdns->port = port;
    mdns->uv_poll = avahi_uv_poll_new(loop);
    if (!mdns->uv_poll) return NSIDS_MEM;

    mdns->client = avahi_client_new(avahi_uv_poll_get(mdns->uv_poll),
            0, client_callback, mdns, &error);
    if (!mdns->client)
    {
        logger(L_ERROR, "Failed to setup MDNS: %s", avahi_strerror(error));
        goto error;
    }

    return NSIDS_OK;

error:
    if (mdns->client)
    {
        avahi_client_free(mdns->client);
        mdns->client = NULL;
    }
    if (mdns->uv_poll)
    {
        avahi_uv_poll_free(mdns->uv_poll);
        mdns->uv_poll = NULL;
    }
    if (mdns->name)
    {
        avahi_free(mdns->name);
        mdns->name = NULL;
    }

    return false;
}

void ids_mdns_free_mdns(AvahiMdnsContext *mdns)
{
    assert(mdns);

    // freeing avahi_client will free group
    if (mdns->client) avahi_client_free(mdns->client);
    if (mdns->uv_poll) avahi_uv_poll_free(mdns->uv_poll);
    if (mdns->name) avahi_free(mdns->name);

    memset(mdns, 0, sizeof(*mdns));
}
