//
// Marosi Attila Csaba <atisu@sztaki.hu>
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation;
// either version 2.1 of the License, or (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// To view the GNU Lesser General Public License visit
// http://www.gnu.org/copyleft/lesser.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include <errno.h>
#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#endif // _WIN32
#include <fstream>
#include "boinc_api.h"
#include "util.h"
#include "gw_common.h"
#include "common.h"


static char *levels[8];
static int cpu_time_warning = 0;

void gw_init() {
    // stupid windows...
    levels[LOG_DEBUG]   = (char*) "Debug";
    levels[LOG_INFO]    = (char*) "Info";
    levels[LOG_NOTICE]  = (char*) "Notice";
    levels[LOG_WARNING] = (char*) "Warning";
    levels[LOG_ERR]     = (char*) "Error";
    levels[LOG_CRIT]    = (char*) "Critical";
}


void gw_do_log(int level, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    gw_do_vlog(level, fmt, ap);
    va_end(ap);
}


void gw_do_vlog(int level, const char *fmt, va_list ap) {
    const char *levstr;
    char timebuf[32];
    struct tm *tm;
    time_t now;

    if (level >= 0 && level < (int)(sizeof(levels) / sizeof(levels[0])) && levels[level])
        levstr = levels[level];
    else
        levstr = "Unknown";
    now = time(NULL);
    tm = localtime(&now);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm);

    fprintf(stderr, "%s [%s] ", timebuf, levstr);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
}


std::string gw_resolve_filename(const char *filename) {
    std::string filename_resolved;

    boinc_resolve_filename_s(filename, filename_resolved);
    return filename_resolved;
}


bool gw_copy_file(const char* src, const char* dst) {
    std::ifstream input_file;
    std::ofstream output_file;

    input_file.open(src, std::ios::in | std::ios::binary);
    if (!input_file) {
        gw_do_log(LOG_ERR, "Failed to open file '%s' for copiing (src)", src);
        return false;
    }
    output_file.open(dst, std::ios::out | std::ios::binary);
    if (!output_file) {
        input_file.close();
        gw_do_log(LOG_ERR, "Failed to open file '%s' for copiing (dst)", dst);
        return false;
    }
    output_file << input_file.rdbuf();
    input_file.close();
    if (input_file.fail()) {
        output_file.close();
        gw_do_log(LOG_ERR, "Failed to close  file '%s' ", src);
        return false;
    }  
    output_file.close();
    if (output_file.fail()) {
        gw_do_log(LOG_ERR, "Failed to close  file '%s' ", dst);
        return false;
    }  
    return true;
}


void gw_finish(int status, double total_cpu_time) {
    gw_report_status(total_cpu_time, 100.0, true);
#ifdef WANT_DCAPI
    // copy stderr/ stdout to DC-API equvialents
    std::string resolved_filename;
    resolved_filename = gw_resolve_filename(DC_LABEL_STDOUT);
    gw_copy_file(STDOUT_FILE, resolved_filename.c_str());
    resolved_filename = gw_resolve_filename(DC_LABEL_STDERR);
    gw_copy_file(STDERR_FILE, resolved_filename.c_str());
#endif // WANT_DCAPI
	gw_do_log(LOG_INFO, "Exiting...");
    boinc_finish(status);
}


void gw_report_fraction_done(double fraction_done) {
    char msg_buf[MSG_CHANNEL_SIZE];
    
    if (app_client_shm == NULL) {
        gw_do_log(LOG_ERR, "Cannot report fraction done, shared memory is not available");
        return;
    }
    sprintf(msg_buf,
	        "<fraction_done>%2.8f</fraction_done>\n",
	        fraction_done);
    app_client_shm->shm->app_status.send_msg(msg_buf);
}


void gw_report_status(double cpu_time, double fraction_done, bool final) {
    char msg_buf[MSG_CHANNEL_SIZE];

    // do not try to report time when running standalone
    if (app_client_shm == NULL) {
        if (cpu_time_warning == 0) {
            gw_do_log(LOG_WARNING, 
	                  "Cannot report cpu time (%10.4f sec), shared memory is not available "\
                      "(not repeating this message again)",
	                  cpu_time);
            cpu_time_warning = 1;
        }
        return;
    }
    // hack to report cpu time -->
    // if result is killed and restarted, only the cpu time
    // of the last run will be reported
    sprintf(msg_buf,
	        "<current_cpu_time>%10.4f</current_cpu_time>\n"
	        "<checkpoint_cpu_time>%.15e</checkpoint_cpu_time>\n"
	        "<fraction_done>%2.8f</fraction_done>\n",
	        cpu_time,
	        0.0,
	        fraction_done);
    app_client_shm->shm->app_status.send_msg(msg_buf);
    if (final) {
        gw_do_log(LOG_INFO, "Reporting final cpu time: %10.4f seconds", cpu_time);
        for (int i=0; i<5; i++) {
            app_client_shm->shm->app_status.send_msg(msg_buf);
            boinc_sleep(POLL_PERIOD);
        }
    }
    // <--
}


double gw_read_fraction_done(void) {
    double fraction = 0.0;
    int result;
    FILE *f = fopen(FILE_FRACTION_DONE, "r");
    if (!f)
        return 0.0;
    result = fscanf(f, "%lf", &fraction);
    fclose(f);
    return fraction;
}

