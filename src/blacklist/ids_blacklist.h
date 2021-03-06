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
/** @file
 *
 */
#ifndef SRC_BLACKLIST_IDS_BLACKLIST_H_
#define SRC_BLACKLIST_IDS_BLACKLIST_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../utils/common.h"
#include "domain_blacklist.h"
/* #include "firehol_ip_blacklist.h" */
#include "ip_blacklist.h"
#include "urlhaus_domain_blacklist.h"

/**
 * Initialize the domain blacklist.
 */
int setup_domain_blacklist(domain_blacklist **bl);

/**
 * Initialize the IP blacklist.
 */
int setup_ip_blacklist(ip_blacklist **bl);

#endif /* SRC_BLACKLIST_IDS_BLACKLIST_H_ */
