#include <Python.h>
#include "rushkit.h"

typedef  PyObject * PYOBJ;
static RTMP_METHOD_TABLE method_table;

void environment_init() {
  rtmp_proto_method_table_init(&method_table);
}

static PYOBJ _get_py_data(PPROTO proto) {
  return (PYOBJ)rtmp_proto_get_user_data(proto);
}

static PYOBJ _get_member(PPROTO proto, const char * member_name) {
  PYOBJ data = _get_py_data(proto);
  PYOBJ member = PyObject_GetAttrString(data, member_name);
  return member;
}

PYOBJ get_py_data(PPROTO proto) {
  PYOBJ obj = _get_py_data(proto);
  Py_XINCREF(obj);
  return obj;
}

void init_responder(PPROTO proto, PYOBJ responder) {
  rtmp_proto_init(proto, &method_table);
  PYOBJ old = _get_py_data(proto);
  Py_XDECREF(old);
  Py_XINCREF(responder);
  rtmp_proto_set_user_data(proto, responder);
}

void free_responder(PPROTO proto)
{
  PYOBJ data = _get_py_data(proto);
  Py_XDECREF(data);
  rtmp_proto_set_user_data(proto, NULL);
}


/*static PYOBJ (PYOBJ self, PYOBJ args) {
  return Py_BuildValue("i", 5);
}

static PyMethodDef _methods[] = {
  {"init_test", init_test, METH_VARARGS, "Init a test"},
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initpyrushkit(void) {
  (void)Py_InitModule("pyrushkit", _methods);
  }*/

