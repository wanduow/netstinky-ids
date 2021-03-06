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
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "utils/logging.h"
#include "domain_blacklist.h"
#include "ids_storedvalues.h"

/**
 * At least for the HAT-trie, extra compression can be attained by reversing the labels so that
 * TLDs come first.
 *
 * When checking domain names, they will also need to be reversed before lookup.
 *
 * Does not modify original domain string. Returns a dynamically allocated string.
 *
 * Will return NULL if memory allocation failed (there are a few extra strings required for this
 * operation).
 */
char *
_domain_blacklist_reverse_labels(const char * domain)
{
    assert(domain);
    int reversed_idx = -1;
    char *delim = ".";

    // token_cpy is the copy which will be destroyed by strtok
    char *token_cpy = strdup(domain), *reversed = NULL;
    if (!token_cpy) goto error;

    char *tok = NULL;
    size_t tok_len;

    size_t reversed_sz = strlen(domain) + 1;
    reversed = malloc(reversed_sz);
    if (!reversed) return NULL;	// Memory allocation error

    // Counts back from end as domain labels are prepended
    reversed_idx = (int) reversed_sz - 1;
    if (reversed_idx < 0) goto error;
    reversed[reversed_idx] = '\0';

    if (NULL != (tok = strtok(token_cpy, delim)))
    {
        do {
            tok_len = strlen(tok);
            reversed_idx -= tok_len;
            assert(reversed_idx >= 0);
            strncpy(reversed + reversed_idx, tok, tok_len);

            // prepend '.'
            if (--reversed_idx >= 0) reversed[reversed_idx] = '.';
        } while (NULL != (tok = strtok(NULL, delim)));
    }

    free(token_cpy);
    return reversed;

error:
    if (token_cpy) free(token_cpy);
    if (reversed) free(reversed);
    return NULL;
}

int
domain_blacklist_add(domain_blacklist *b, const char *domain, ids_ioc_value_t *value)
{
    assert(b);
    assert(domain);

    // Reverse domain label order before inserting.
    char *reversed = _domain_blacklist_reverse_labels(domain);
    if (!reversed) return 0;

    hattrie_t *h = (hattrie_t *)b;
    value_t *result = NULL;
    size_t len;

    len = strlen(reversed);
    result = hattrie_get(h, reversed, len);
    if (result && *result)
    {
        // If the trie already has a value for this domain, free the old one
        free_ids_ioc_value((ids_ioc_value_t *)*result);
    }
    *result = (uintptr_t)value;
    free(reversed);

    return (result ? 1 : 0);
}

ids_ioc_value_t *
domain_blacklist_is_blacklisted(domain_blacklist *b, const char *domain)
{
    assert(b);
    assert(domain);

    hattrie_t *h = (hattrie_t *)b;
    size_t len;
    value_t *result = NULL;

    char *reversed = _domain_blacklist_reverse_labels(domain);
    if (!reversed)
    {
        // This is a serious error since we can't check the blacklist. Definitely log it.
        logger(L_WARN, "Could not reverse domain name when checking blacklist: %s\n", domain);
        return 0;
    }

    if (b && domain)
    {
        len = strlen(domain);
        result = hattrie_tryget(h, reversed, len);
    }

    free(reversed);

    if (result)
        return (ids_ioc_value_t *) *result;
    else
        return NULL;
}

void
domain_blacklist_clear(domain_blacklist *b)
{
    assert(b);
    bool sorted = false;
    value_t *stored = NULL;
    hattrie_t *h = (hattrie_t *)b;
    hattrie_iter_t *iter = hattrie_iter_begin(h, sorted);

    // Iterate through trie and free all stored values
    while (!hattrie_iter_finished(iter))
    {
        stored = hattrie_iter_val(iter);
        free_ids_ioc_value((ids_ioc_value_t *) *stored);
        hattrie_iter_next(iter);
    }
    hattrie_iter_free(iter);

    hattrie_free(h);
}

void
free_domain_blacklist(domain_blacklist **b)
{
    assert(b);
    hattrie_t **h = (hattrie_t **)b;

    if (h && *h)
    {
        hattrie_free(*h);
        *h = NULL;
    }
}

domain_blacklist *new_domain_blacklist()
{
    hattrie_t *h = hattrie_create();
    return (h);
}
