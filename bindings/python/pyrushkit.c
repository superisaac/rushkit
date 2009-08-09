#include <Python.h>
#include "rtmp_proto.h"

static PyObject * init_test(PyObject * self, PyObject * args) {
  return Py_BuildValue("i", 5);
}

static PyMethodDef _methods[] = {
  {"init_test", init_test, METH_VARARGS, "Init a test"},
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initpyrushkit(void) {
  (void)Py_InitModule("pyrushkit", _methods);
}
