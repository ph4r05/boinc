#include <ctype.h>
#include "busybox.h"
#include "quote.h"

#ifdef _WIN32
/*
 * Disable wildcard commandline parameter expansion for
 * MinGW ('globbing'). It is defined by CRT_noglob.o
 * (in the MinGW library directory), instead of linking
 * we define it here.
 *
 */
int _CRT_glob = 0;
#endif

static void prepend_to_path(const char *dir, int len)
{
	const char *old_path = getenv("PATH");
	char *path;
	int path_len = len;

	if (!old_path)
		old_path = "/usr/local/bin:/usr/bin:/bin";

	path_len = len + strlen(old_path) + 1;

	path = xmalloc(path_len + 1);

	memcpy(path, dir, len);
	path[len] = PATH_SEPARATOR;
	memcpy(path + len + 1, old_path, path_len - len);

	setenv("PATH", path, 1);

	free(path);
}

int main(int argc, char **argv)
{
	const char *exec_path = NULL;
	char *slash;

 	if (getenv("GIT_TRACE")) {
		fprintf(stderr, "argv[0]=%s\n", argv[0]);
	}

	/* Try to find our path in the environment first */
 	slash = getenv("BB_BUSYBOX_EXEC_PATH");
 	if (slash) {
		*(char **)&bb_busybox_exec_path = slash;
	} else {
		/*
		 * Take the basename of argv[0] as the command
		 * name, and the dirname as the default exec_path
		 * if it's an absolute path and we don't have
		 * anything better or at least the current working directory.
		 */
                slash = strrchr(argv[0], DIRECTORY_SEPARATOR);
		if(slash) {
			*slash++ = '\0';
#ifdef _WIN32
			if (argv[0][1] == ':') {
#else
			if (argv[0][0] == '/') {
#endif
			/* If absolute path */
				exec_path = argv[0];
			} else {
				char *cwd;

				cwd = getcwd(NULL, 0);
				chdir(argv[0]);
				/* chdir may fail but then we take the current dir */
				exec_path = getcwd(NULL, 0);
				if (cwd) {
					chdir(cwd);
					free(cwd);
				}
			}
			argv[0] = slash;
            } else {
                /* No directory separator found. On Windows box may be executed
                 * by typing forexample "gitbox.exe". In this case the above code 
                 * fails, we fall back to argv[0]. 
                 */
                slash = argv[0];
                if (getenv("GIT_TRACE")) {
                        fprintf(stderr, "Using argv[0] ('%s') for exec name", argv[0]);
                }
            }
            if(exec_path) {
		        slash = xzalloc(strlen(exec_path) + strlen(argv[0]) + 2);
		        strcpy(slash, exec_path);
		        slash[strlen(exec_path)] = DIRECTORY_SEPARATOR;
		        strcat(slash, argv[0]);
		    } 
            if (slash) {
                *(char **)&bb_busybox_exec_path = slash;
                setenv("BB_BUSYBOX_EXEC_PATH", bb_busybox_exec_path, 1);
            }                        
        }
	if (!bb_busybox_exec_path) die("Could not determine my exec name");

	/*
	 * We search for git commands in the following order:
	 *  - git_exec_path()
	 *  - the path of the "git" command if we could find it
	 *    in $0
	 *  - the regular PATH.
	 */
/*	if (exec_path)
		prepend_to_path(exec_path, strlen(exec_path));
	exec_path = git_exec_path();
	prepend_to_path(exec_path, strlen(exec_path)); */

 	if (getenv("GIT_TRACE")) {
		char *argv_str = sq_quote_argv((const char**)argv, -1);

		fprintf(stderr, "exec_path=%s, basename=%s\n", exec_path, argv[0]);
		fprintf(stderr, "bb_busybox_exec_path=%s\n", bb_busybox_exec_path);
		fprintf(stderr, "git-box:%s\n", argv_str);
		free(argv_str);
	}
	return lbb_main(argv);
}
