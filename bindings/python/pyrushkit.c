#include <Python.h>
#include "rushkit.h"

static RTMP_METHOD_TABLE method_table;

void environment_init() {
  rtmp_proto_method_table_init(&method_table);
}

void init_responder(PPROTO proto, PyObject * responder) {
  rtmp_proto_init(proto, &method_table);
  PyObject * old = (PyObject *)rtmp_proto_get_user_data(proto);
  if(old != NULL) {
    Py_DECREF(old);
  }
  Py_INCREF(responder);
  rtmp_proto_set_user_data(proto, responder);
}
/*static PyObject * (PyObject * self, PyObject * args) {
  return Py_BuildValue("i", 5);
}

static PyMethodDef _methods[] = {
  {"init_test", init_test, METH_VARARGS, "Init a test"},
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initpyrushkit(void) {
  (void)Py_InitModule("pyrushkit", _methods);
  }*/

