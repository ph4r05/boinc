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
#include "usage.h"
#include "libbb.h"
#include "boinc_api.h"
#include "common.h"


static int resolve_filename(const char* filename) {
  char buf[PATH_MAX];
  int result;
  result = boinc_resolve_filename(filename, buf, PATH_MAX-1);
  fprintf(stdout, "%s", buf);
  return result;
}


static int fraction_done_percent(int fraction) {
  double d_fraction = fraction / 100.00;
  if (d_fraction < 0.0)
    d_fraction = 0.0;
  if (d_fraction > 1.0)
    d_fraction = 1.0;
  FILE *f = fopen(FILE_FRACTION_DONE, "w+");
  if (!f)
    return false;
  fprintf(f, "%lf", d_fraction);
  fclose(f);
  return true;
}


static int fraction_done(double fraction) {
  return fraction_done_percent(fraction * 100.00);
}


int boinc_main(int argc, char **argv);
int boinc_main(int argc, char **argv)
{
  int retval, status;
  int fraction;
  double d_fraction;
  char *endptr;

  retval = EXIT_SUCCESS;
  if (argc<=1) {
    fprintf(stdout, "%s\n", boinc_trivial_usage);
    return retval;
  }

  if (strcmp(argv[1], "resolve_filename")==0) {
    if (argc<=2) {
      fprintf(stderr,"ERROR: missing parameter for resolve_filename\n");
      return retval;
    }
    resolve_filename(argv[2]);
  } else if (strcmp(argv[1], "fraction_done_percent")==0) {
    if (argc<=2) {
      fprintf(stderr, "ERROR: missing parameter for fraction_done\n");
      return retval;
    }
    errno=0;
    fraction = strtol(argv[2], &endptr, 10);
    if (errno!=0 || argv[2]==endptr) {
      fprintf(stderr,"ERROR: invalid parameter for fraction_done ('%s'->%d)\n", argv[2], fraction);
      return retval;            
    }
    fraction_done_percent(fraction);
  } else if (strcmp(argv[1], "fraction_done")==0) {
    if (argc<=2) {
      fprintf(stderr,"ERROR: missing parameter for fraction_done\n");
      return retval;
    }
    errno=0;
    d_fraction = strtod(argv[2], &endptr);
    if (errno!=0 || argv[2]==endptr) {
      fprintf(stderr,"ERROR: invalid parameter for fraction_done_percent ('%s'->%lf)\n", argv[2], d_fraction);
      return retval;            
    }
    fraction_done(d_fraction);
  } else if (strcmp(argv[1], "finish")==0) {
    if (argc<=2) {
      fprintf(stderr,"ERROR: missing parameter for finish\n");
      return retval;
    }
        errno = 0;
        status = strtol(argv[2], &endptr, 10);
        if (errno!=0 || argv[2]==endptr) {
	  fprintf(stderr,"ERROR: invalid parameter for finish ('%s'->%d)\n", argv[2], status);
	  return retval;            
        }
	exit(status);
  } else {
    fprintf(stderr, "ERROR: invalid command '%s'\n", argv[1]);
  }
  return retval;
}
