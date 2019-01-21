#include <config.h>
#include <ctype.h>
#include <glib.h>
#include <libintl.h>
#include <locale.h>
#include <pthread.h>
#include <pty.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <utmp.h>

#include "../../ldmutils.h"
#include "../../ldmgreetercomm.h"
#include "../../logging.h"
#include "../../plugin.h"
#include "rdesktop.h"

LdmBackend *descriptor;
RdpInfo *rdpinfo;

void __attribute__ ((constructor)) initialize()
{
    descriptor = (LdmBackend *) malloc(sizeof(LdmBackend));
    bzero(descriptor, sizeof(LdmBackend));

    descriptor->name = "rdesktop";
    descriptor->description = "rdesktop plugin";
    descriptor->init_cb = init_rdesktop;
    descriptor->auth_cb = auth_rdesktop;
    descriptor->start_cb = start_rdesktop;
    descriptor->clean_cb = close_rdesktop;
    ldm_init_plugin(descriptor);
}

/*
 * init_rdesktop
 *  Callback function for initialization
 */
void
init_rdesktop()
{
    rdpinfo = (RdpInfo *) malloc(sizeof(RdpInfo));
    bzero(rdpinfo, sizeof(RdpInfo));

    /* Get ENV value */
    rdpinfo->rdpoptions = g_strdup(getenv("RDP_OPTIONS"));
    rdpinfo->server = g_strdup(getenv("RDP_SERVER"));
}

/*
 * start_rdesktop
 *  Callback function for starting rdesktop session
 */
void
start_rdesktop()
{
    gboolean error = FALSE;

    /* Variable validation */
    if (!rdpinfo->username) {
        log_entry("rdesktop", 3, "no username");
        error = TRUE;
    }

    if (!rdpinfo->password) {
        log_entry("rdesktop", 3, "no password");
        error = TRUE;
    }

    if (!rdpinfo->server) {
        log_entry("rdesktop", 3, "no server");
        error = TRUE;
    }

    if (!rdpinfo->domain) {
        log_entry("rdesktop", 3, "no domain");
        error = TRUE;
    }

    if (error) {
        die("rdesktop", "missing mandatory information");
    }

    /* Greeter not needed anymore */
    close_greeter();

    log_entry("rdesktop", 6, "starting rdesktop session to '%s' as '%s'",
              rdpinfo->server, rdpinfo->username);
    rdesktop_session();
    log_entry("rdesktop", 6, "closing rdesktop session");
}

/*
 * _get_domain
 */
void
_get_domain()
{
    gchar *cmd = "value domain\n";

    rdpinfo->domain = ask_value_greeter(cmd);
}

/*
 * auth_rdesktop
 *  Callback function for authentication
 */
void
auth_rdesktop()
{
    gchar *cmd;

    /* Separator for domains : '|' */
    gchar *domains = getenv("RDP_DOMAIN");
    cmd =
        g_strconcat
        ("pref choice;domain;Domain;Select Domai_n ...;session;", domains,
         "\n", NULL);
    if (domains) {
        if (ask_greeter(cmd))
            die("rdesktop", "%s from greeter failed", cmd);
    } else {
        log_entry("rdesktop", 7,
                  "RDP_DOMAIN isn't defined, rdesktop will connect on default domain");
    }

    /* Ask for UserID */
    get_userid(&(rdpinfo->username));

    /* If user clicks on guest button above, this has changed  */
    get_passwd(&(rdpinfo->password));

    /* Get hostname */
    if (!rdpinfo->server)
        get_host(&(rdpinfo->server));

    /* Get Domain (rdesktop plugin specific) */
    _get_domain();

    /* Get Language */
    get_language(&(rdpinfo->lang));

    g_free(cmd);
}

/*
 * close_rdesktop
 *  Callback function for closing the plugins
 */
void
close_rdesktop()
{
    log_entry("rdesktop", 7, "closing rdesktop session");
    free(rdpinfo);
}

/*
 * rdesktop_session
 *  Start a rdesktop session to server
 */
void
rdesktop_session()
{
    gchar *cmd;

    cmd = g_strjoin(" ", "rdesktop", "-f",
                    "-u", rdpinfo->username,
                    "-p", rdpinfo->password, NULL);

    /* Only append the domain if it's set */
    if (g_strcmp0(rdpinfo->domain, "None") != 0) {
        cmd = g_strjoin(" ", cmd, "-d", rdpinfo->domain, NULL);
    }

    /* If we have custom options, append them */
    if (rdpinfo->rdpoptions) {
        cmd = g_strjoin(" ", cmd, rdpinfo->rdpoptions, NULL);
    }

    cmd = g_strjoin(" ", cmd, rdpinfo->server, NULL);

    /* Spawning rdesktop session */
    rdpinfo->rdppid = ldm_spawn(cmd, NULL, NULL, NULL);
    ldm_wait(rdpinfo->rdppid);

    g_free(cmd);
}
