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

#include <stdio.h>
#include <libgen.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <errno.h>
#ifdef _WIN32
#include "boinc_win.h"
#else
#include <syslog.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif // _WIN32
#include "boinc_api.h"
#include "diagnostics.h" 
// for boinc_sleep()
#include "util.h"
// for parse_command_line()
#include "str_util.h"
#include "gw_common.h"
#include "common.h"
#include "task.h"
#include "error_numbers.h"

#ifdef _WIN32
#define GENWRAPPER_EXE   "gitbox.exe"
#ifndef S_ISGRP // not defined on Windows
#define S_ISGRP(x) 0
#define S_IRGRP 0
#define S_IWGRP 0
#define S_IXGRP 0
#define S_ISGID 0
#define S_IROTH 0
#define S_IXOTH 0
#endif // S_ISGRP
#else
#define GENWRAPPER_EXE   "./gitbox"
#endif // _WIN32
#define PROFILE_SCRIPT   "profile.sh"
#define EXEC_SCRIPT      "gw_tmp.sh"
#ifndef _MAX_PATH
#define _MAX_PATH        255
#endif


#ifdef WANT_DCAPI
static const char* dc_files[] = {
  DC_LABEL_STDOUT,
  DC_LABEL_STDERR,
  DC_LABEL_CLIENTLOG,
  CKPT_LABEL_OUT,
  NULL
};
#endif

const char* WU_SCRIPT = "wu_script.sh";

// from unzip.c in libbb
extern "C" {
  int unzip_main(int argc, char **argv);
}


void poll_boinc_messages(TASK& task) {
  BOINC_STATUS status;
  boinc_get_status(&status);
  if (status.no_heartbeat) {
    task.kill();
    exit(0);
  }
  if (status.quit_request) {
    task.kill();
    exit(0);
  }
  if (status.abort_request) {
    task.kill();
    exit(0);
  }
  if (status.suspended) {
    if (!task.suspended) {
      task.stop();
    }
  } else {
    if (task.suspended) {
      task.resume();
    }
  }
}


int main(int argc, char* argv[]) {
  BOINC_OPTIONS options;

  gw_init();

#ifdef WANT_DCAPI
  boinc_init_diagnostics(BOINC_DIAG_REDIRECTSTDERR | BOINC_DIAG_REDIRECTSTDOUT);
#endif
  gw_do_log(LOG_INFO, "Launcher for GenWrapper (build date %s, %s)", __DATE__, SVNREV);

  memset(&options, 0, sizeof(options));
  options.main_program = true;
  options.check_heartbeat = true;
  options.handle_process_control = true;
  options.send_status_msgs = false;
  boinc_init_options(&options);

#ifdef WANT_DCAPI
  gw_do_log(LOG_INFO, "DC-API enabled version");
  // need to create various files expected by DC-API
  // in case the application fails, DC-API still expects them
  std::string dc_filename_resolved;
  FILE* f;
  for (int i=0; dc_files[i] != NULL; i++) {
    dc_filename_resolved = gw_resolve_filename(dc_files[i]);
    f = fopen(dc_filename_resolved.c_str(), "w");
    if (f) { 
      fclose(f);
    } else {
      gw_do_log(LOG_ERR, "Failed to create DC-API file '%s'", dc_files[i]);
      gw_finish(EXIT_FAILURE);    
    }
  }
#else
  gw_do_log(LOG_INFO, "BOINC enabled version");
#endif

  // Look for & unzip the .zip archive, if any
  std::string filename(basename(argv[0]));

#ifdef _WIN32
  if (filename.compare(filename.length() - 4, 4, ".exe") == 0)
    filename = filename.erase(filename.length() - 4, filename.length()-1);
#endif

  filename.append(".zip");
  std::string zip_filename_resolved = gw_resolve_filename(filename.c_str());
  std::string genwrapper_exe_resolved = gw_resolve_filename(GENWRAPPER_EXE);

  if (access(zip_filename_resolved.c_str(), R_OK) != -1) {
    const char *zip_argv[] = {
      "unzip", "-o", "-X", zip_filename_resolved.c_str(), 0
    };
    const int zip_argc = sizeof(zip_argv) / sizeof(zip_argv[0]) - 1;

    gw_do_log(LOG_INFO, "Unzipping '%s'", filename.c_str());
    if (unzip_main(zip_argc, (char **)zip_argv)) {
      gw_do_log(LOG_ERR, "Failed to unzip '%s'", zip_filename_resolved.c_str());
      gw_finish(EXIT_FAILURE);
    }
  } else {
	gw_do_log(LOG_INFO, "Zipfile not found '%s'",
 		zip_filename_resolved.c_str());
  }

  // Check for the interpreter
  if (access(genwrapper_exe_resolved.c_str(), X_OK)) {
    gw_do_log(LOG_INFO, "Wrapper executable '%s' is not executable : %s. Trying to make it executable...",
      genwrapper_exe_resolved.c_str(), strerror(errno));
    if (chmod(genwrapper_exe_resolved.c_str(), S_IXUSR | S_IXGRP | S_IXOTH |
       S_IRUSR | S_IRGRP | S_IROTH) == -1) {
       gw_do_log(LOG_ERR, "Cannot set executable flag for Wrapper executable '%s': %s",
         genwrapper_exe_resolved.c_str(), strerror(errno));
       gw_finish(EXIT_FAILURE);  
    }     
  }

  std::string wu_script(WU_SCRIPT);
  FILE *filetest; 

  // test whether default wu_script file esists
  filetest = fopen(wu_script.c_str(), "r+");
  if (filetest != NULL) {
   	  fclose(filetest);
	  gw_do_log(LOG_INFO, "Found default workunit script file ('%s') in "
	 	"working directory.", WU_SCRIPT);
  } else {
  	gw_do_log(LOG_INFO, "Default workunit script file ('%s') not found. "
		"Going to test if first command line argument is the workunit script "
		"(legacy behavior).", WU_SCRIPT);
	if (argc > 1) {
    	wu_script = argv[1];
	  	filetest = fopen(wu_script.c_str(), "r+");	
	    if (filetest == NULL) {
	    	gw_do_log(LOG_ERR, "Cannot open workunit script file '%s' (first "
				"command line argument). No other options left, bailing "
				"out...");
      		gw_finish(EXIT_FAILURE);        
	    } else {
			fclose(filetest);
		}
	} else {
    	gw_do_log(LOG_ERR, "Cannot find any workunit scripts. No other "
 			"options left, bailing out...");
      	gw_finish(EXIT_FAILURE);        
	}
  }
  // create script file which execs profile and the wu supplied (argv[1]) script
  std::ofstream exec_script(EXEC_SCRIPT, std::ios::out);
  exec_script << "set -e\n"
    // profile script is optional
    << "if [ -r ./" PROFILE_SCRIPT " ]; then . `boinc resolve_filename ./"
 	<< PROFILE_SCRIPT "`; fi\n"
    << ". `boinc resolve_filename ./" << wu_script << "`\n";
  exec_script.close();
  if (exec_script.fail()) {
    gw_do_log(LOG_ERR, "Failed to create the initialization script");
    gw_finish(EXIT_FAILURE);
  }
  TASK gw_task;
  vector<string> args;
  args.push_back(genwrapper_exe_resolved.c_str());
  args.push_back(string("sh"));
  args.push_back(EXEC_SCRIPT);
  if (argc > 1) {
	  if (strcmp(argv[1], wu_script.c_str())) {
		args.push_back(string(argv[1]));
	  } else {
		gw_do_log(LOG_WARNING, "First command line parameter ('%s') seems like "
			"a GenWrapper work unit script. I am going to remove it from the "
			"list of command line parameters. If this is an error and your "
			"application is missing a parameter, please add the work unit "
			"script name to the command line as the first parameter.", argv[1]);
	  }
  }

  for (int i = 2; i < argc; i ++) {
	args.push_back(string(argv[i]));
  }
  if (gw_task.run(args) == ERR_EXEC) {
    gw_do_log(LOG_ERR, "Could not exec %s\n", genwrapper_exe_resolved.c_str());
    gw_finish(EXIT_FAILURE);
  }
  
  int status;
  while(1) {
    if (gw_task.poll(status)) {
      break;
    }
    poll_boinc_messages(gw_task);
    boinc_sleep(POLL_PERIOD);
  }

  if (status) {
	gw_do_log(LOG_ERR, "'%s' exited with error: %d\n",
		genwrapper_exe_resolved.c_str(), status);
  }

  // we are going to have the stderr/ stdout from gitbox and below to different
  // files (GW_STDERR_FILE and GW_STDOUT_FILE): the gw stdout/stderr overwrote
  // the previous data from the launcher in win32.

  std::ifstream input_file;
  std::string line;

  fflush(stderr);
  input_file.open(GW_STDERR_FILE, std::ios::in | std::ios::binary);
  if (input_file.is_open()) {
   	  fprintf(stderr, "\n");
	  while (input_file.good()) {
	    getline(input_file, line);
		fprintf(stderr, " > %s\n", line.c_str());
	  }
   	  fprintf(stderr, "\n");
	  input_file.close();
  }
  fflush(stderr);

  fflush(stdout);
  input_file.open(GW_STDOUT_FILE, std::ios::in | std::ios::binary);
  if (input_file.is_open()) {
	  while (input_file.good()) {
	    getline(input_file, line);
		fprintf(stdout, "%s\n", line.c_str());
	  }
	  input_file.close();
  }
  fflush(stdout);

  //
  // check if stderr.txt and stdout.txt are requested as output files, and copy
  // the content of the local files (in the slot directory) to the output files 
  // (in the project directory).
  std::string file_stderr = gw_resolve_filename(STDERR_FILE);
  std::string file_stdout = gw_resolve_filename(STDOUT_FILE);
  int cmpresult = 0;
  cmpresult = file_stdout.compare(STDOUT_FILE);
  if (cmpresult != 0) {
  	gw_do_log(LOG_INFO, "'%s' is requested as output file, copying "
		"contents of standard output to this file.", STDOUT_FILE);
	std::ifstream ifs(STDOUT_FILE, std::ios::binary);
	std::ofstream ofs(file_stdout.c_str(), std::ios::binary);
	ofs << ifs.rdbuf();
  } else {
  	gw_do_log(LOG_INFO, "'%s' is NOT requested as output file.",
		STDOUT_FILE);
  	gw_do_log(LOG_DEBUG, "('%s'.compare('%s') == %d)",
		STDOUT_FILE, file_stdout.c_str(), cmpresult);
  }
  cmpresult = file_stderr.compare(STDERR_FILE);
  if (cmpresult != 0) {
  	gw_do_log(LOG_INFO, "'%s' is requested as output file, copying "
		"contents of standard error to this file.", STDERR_FILE);
	std::ifstream ifs(STDERR_FILE, std::ios::binary);
	std::ofstream ofs(file_stderr.c_str(), std::ios::binary);
	ofs << ifs.rdbuf();
  } else {
  	gw_do_log(LOG_INFO, "'%s' is NOT requested as output file.",
		STDERR_FILE);
  	gw_do_log(LOG_DEBUG, "('%s'.compare('%s') == %d)",
		STDERR_FILE, file_stderr.c_str(), cmpresult);
  }

  gw_finish(status, gw_task.final_cpu_time);
  // we never get here
  return 0;	
}


#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR Args, int WinMode) {
  LPSTR command_line;
  char* argv[100];
  int argc;

  command_line = GetCommandLine();
  argc = parse_command_line( command_line, argv );
  return main(argc, argv);
}
#endif
