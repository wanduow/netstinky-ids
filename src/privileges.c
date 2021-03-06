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
#include <config.h>

#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

#include "utils/logging.h"
#include "privileges.h"

int
drop_privileges(const uid_t newuid, const uid_t newgid)
{
    gid_t oldgid = getgid();
    uid_t olduid = getuid();
    int res;

    // Already a non-root user
    if (olduid != 0) return 1;

    if (!olduid) setgroups(1, &newgid);

    if (newgid != oldgid)
    {
#if !defined(linux)
        setegid(newgid);
        res = setgid(newgid);
        if (res != 0)
        {
            logger(L_ERROR, "Failed to setgid(): %s", strerror(errno));
            return -1;
        }
#else
        res = setregid(newgid, newgid);
        if (res != 0)
        {
            logger(L_ERROR, "Failed to setregid(): %s", strerror(errno));
            return -1;
        }
#endif
    }

    if (newuid != olduid)
    {
#if !defined(linux)
        seteuid(newuid);
        res = setuid(newuid);
        if (res != 0)
        {
            logger(L_ERROR, "Failed to setuid(): %s", strerror(errno));
            return -1;
        }
#else
        res = setreuid(newuid, newuid);
        if (res != 0)
        {
            logger(L_ERROR, "Failed to setreuid(): %s", strerror(errno));
            return -1;
        }
#endif
    }

    // Validate that the changes stuck
    if (newgid != oldgid && (setegid(oldgid) != -1 || getegid() != newgid))
        return -1;
    if (newuid != olduid && (seteuid(olduid) != -1 || geteuid() != newuid))
        return -1;

    return 0;
}

int
ch_user(const char *user, const char *group)
{
    struct passwd pwd;
    struct passwd *pwdres = NULL;
    struct group grp;
    struct group *grpres = NULL;
    char *buf;
    int s;

    ssize_t bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);

    if (bufsize == -1) bufsize = 16384;
    buf = calloc(bufsize, sizeof(char));

    if (group != NULL)
    {
        s = getgrnam_r(group, &grp, buf, bufsize, &grpres);
        if (grpres == NULL)
        {
            if (s == 0)
                logger(L_ERROR, "gid for group %s not found!", group);
            else
                logger(L_ERROR, "Failed to look up gid for group %s: %s", group,
                        strerror(errno));
            return -1;
        }
    }

    s = getpwnam_r(user, &pwd, buf, bufsize, &pwdres);
    if (pwdres == NULL)
    {
        if (s == 0)
            logger(L_ERROR, "uid for user %s not found!", user);
        else
            logger(L_ERROR, "Failed to look up uid for user %s: %s", user,
                    strerror(errno));
        return -1;
    }

    s = drop_privileges(
        pwd.pw_uid,
        (group == NULL) ? pwd.pw_gid : grp.gr_gid);

    free(buf);

    return s;
}
