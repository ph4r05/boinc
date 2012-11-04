/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) 1999-2004 by Erik Andersen <andersen@codepoet.org>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include "libbb.h"
#if ENABLE_FEATURE_SYSLOG
#include <syslog.h>
#endif

void bb_info_msg(const char *s, ...)
{
	va_list p;
	/* va_copy is used because it is not portable
	 * to use va_list p twice */
#if ENABLE_FEATURE_SYSLOG
	va_list p2;
#endif

	va_start(p, s);
#if ENABLE_FEATURE_SYSLOG
	va_copy(p2, p);
#endif
	if (logmode & LOGMODE_STDIO) {
		vprintf(s, p);
		fputs(msg_eol, stdout);
	}
#if ENABLE_FEATURE_SYSLOG
	if (logmode & LOGMODE_SYSLOG)
		vsyslog(LOG_INFO, s, p2);
	va_end(p2);
#endif
	va_end(p);
}
