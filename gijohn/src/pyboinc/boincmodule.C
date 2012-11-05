#include <Python.h>
#include "boinc/boinc_api.h"
#include "boinc/filesys.h"
#include "boinc/error_numbers.h"

// Exception used when BOINC clearly produced an error
static PyObject *BoincError;

static PyObject *boinc_boinc_init(PyObject *self, PyObject *args);
static PyObject *boinc_boinc_finish(PyObject *self, PyObject *args);
static PyObject *boinc_boinc_resolve_filename(PyObject *self, PyObject *args);
static PyObject *boinc_boinc_fopen(PyObject *self, PyObject *args);
static PyObject *boinc_boinc_time_to_checkpoint(PyObject *self, PyObject *args);
static PyObject *boinc_boinc_checkpoint_completed(PyObject *self, PyObject *args);
static PyObject *boinc_boinc_begin_critical_section(PyObject *self, PyObject *args);
static PyObject *boinc_boinc_end_critical_section(PyObject *self, PyObject *args);
static PyObject *boinc_boinc_fraction_done(PyObject *self, PyObject *args);
static PyObject *boinc_boinc_get_fraction_done(PyObject *self, PyObject *args);
static PyObject *boinc_boinc_is_standalone(PyObject *self, PyObject *args);
static PyObject *boinc_boinc_need_network(PyObject *self, PyObject *args);
static PyObject *boinc_boinc_network_poll(PyObject *self, PyObject *args);
static PyObject *boinc_boinc_network_done(PyObject *self, PyObject *args);


static PyMethodDef BoincMethods[] = {
{"boinc_init", boinc_boinc_init, METH_VARARGS, "Start BOINC work"},
{"boinc_finish", boinc_boinc_finish, METH_VARARGS, "Finish BOINC work"}, 
{"boinc_resolve_filename", boinc_boinc_resolve_filename, METH_VARARGS, "Resolve a filename"}, 
{"boinc_fopen", boinc_boinc_fopen, METH_VARARGS, "Safely open a file"}, 
{"boinc_time_to_checkpoint", boinc_boinc_time_to_checkpoint, METH_VARARGS, "Find out if we need to checkpoint"}, 
{"boinc_checkpoint_completed", boinc_boinc_checkpoint_completed, METH_VARARGS, "Find out if checkpointing has been completed"}, 
{"boinc_begin_critical_section", boinc_boinc_begin_critical_section, METH_VARARGS, "Inform BOINC we are in a critical section"},
{"boinc_end_critical_section", boinc_boinc_end_critical_section, METH_VARARGS, "Inform BOINC we have finished the critical section"},
{"boinc_fraction_done", boinc_boinc_fraction_done, METH_VARARGS, "Set the fraction of the workunit completed in range [0,1]"},
{"boinc_get_fraction_done", boinc_boinc_get_fraction_done, METH_VARARGS, "Get the fraction of the workunit completed in range [0,1]"},
{"boinc_is_standalone", boinc_boinc_is_standalone, METH_VARARGS, "Check if the BOINC client is running standalone"},
{"boinc_need_network", boinc_boinc_need_network, METH_VARARGS, "Inform BOINC that we need access to the internet"},
{"boinc_network_poll", boinc_boinc_network_poll, METH_VARARGS, "Check if we have access to the internet"},
{"boinc_network_done", boinc_boinc_network_done, METH_VARARGS, "Inform BOINC we are finished with the internet"},
{NULL,NULL,0,NULL}};


/* Initialises BOINC module; obligatory for python extension modules. */
PyMODINIT_FUNC initboinc(void)
{
	PyObject *m;

	m  = Py_InitModule("boinc",BoincMethods);

	BoincError = PyErr_NewException("boinc.error",NULL,NULL);
	Py_INCREF(BoincError);
	PyModule_AddObject(m,"error",BoincError);
}


/* Most of the functions in the "Basic API" documentation are implemented here. That's enough to run any basic BOINC application. Anything more 
sophisticated probably deserves to be rewritten in C++. */
static PyObject *boinc_boinc_init(PyObject *self, PyObject *args)
{

	int retval;
	retval = boinc_init();

	if(retval)
	{
		PyErr_SetString(BoincError, "call to boinc_init() failed");
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}


static PyObject *boinc_boinc_finish(PyObject *self, PyObject *args)
{

	int retval;
	int exitVal = 0;

	if (!PyArg_ParseTuple(args, "|i", &exitVal))
		return NULL;

	retval = boinc_finish(exitVal);

	if(retval)
	{
		PyErr_SetString(BoincError, "call to boinc_finish() failed");
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}


static PyObject *boinc_boinc_resolve_filename(PyObject *self, PyObject *args)
{
	char *name;
	int strsize;

	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;
	
	char resolved_name[512];
	int retval;

	retval = boinc_resolve_filename(name, resolved_name, sizeof(resolved_name));

	if(retval == ERR_NULL)
	{
		PyErr_SetString(BoincError, "call to boinc_resolve_filename() failed, returned ERR_NULL");
		return NULL;
	}


/* If we get here the worst that could happen is the file doesn't exist,
 * but that doesn't really concern us... */

	return Py_BuildValue("s", resolved_name); 
}


static PyObject *boinc_boinc_fopen(PyObject *self, PyObject *args)
{

	char *name;
	char *mode;


	if (!PyArg_ParseTuple(args, "ss", &name, &mode))
		return NULL;

	FILE *fp;
	PyObject *object;

	fp = boinc_fopen(name, mode);

	object = PyFile_FromFile(fp, name, mode, fclose);
	Py_INCREF(object);

	return object;
}

static PyObject *boinc_boinc_checkpoint_completed(PyObject *self, PyObject *args)
{

	boinc_checkpoint_completed();

	Py_INCREF(Py_None);
	return Py_None;
}


static PyObject *boinc_boinc_time_to_checkpoint(PyObject *self, PyObject *args)
{

	int isItTime;

	isItTime = boinc_time_to_checkpoint();

	return Py_BuildValue("i", isItTime);
}


static PyObject *boinc_boinc_begin_critical_section(PyObject *self, PyObject *args)
{

	boinc_begin_critical_section();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *boinc_boinc_end_critical_section(PyObject *self, PyObject *args)
{
	boinc_end_critical_section();

	Py_INCREF(Py_None);
	return Py_None;
}


/* Not implemented boinc_ops wrappers as these functions seem a bit 
 * pointless given the overheads of running python. */

static PyObject *boinc_boinc_fraction_done(PyObject *self, PyObject *args)
{

	double fractDone;

	if (!PyArg_ParseTuple(args, "d", &fractDone))
		return NULL;
	
	boinc_fraction_done(fractDone);

	Py_INCREF(Py_None);
	return Py_None;
}



static PyObject *boinc_boinc_get_fraction_done(PyObject *self, PyObject *args)
{

	double fractDone;

	fractDone = boinc_get_fraction_done();

	return Py_BuildValue("d", fractDone);
}


static PyObject *boinc_boinc_is_standalone(PyObject *self, PyObject *args)
{

	int isItStandalone;

	isItStandalone = boinc_is_standalone();

	return Py_BuildValue("i", isItStandalone);

}


static PyObject *boinc_boinc_need_network(PyObject *self, PyObject *args)
{

	boinc_need_network();

	Py_INCREF(Py_None);
	return Py_None;
}


static PyObject *boinc_boinc_network_poll(PyObject *self, PyObject *args)
{

	int haveNetwork;

	haveNetwork = boinc_network_poll();

	return Py_BuildValue("i", haveNetwork);

}


static PyObject *boinc_boinc_network_done(PyObject *self, PyObject *args)
{

	boinc_network_done();

	Py_INCREF(Py_None);
	return Py_None;
}


