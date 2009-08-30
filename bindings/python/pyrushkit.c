#include <Python.h>
#include "rushkit.h"

typedef  PyObject * PYOBJ;
static RTMP_METHOD_TABLE method_table;

extern PyObject * av_2_py(AV*);
extern PyObject * av_list_2_py(PYOBJ, AV*, int);

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

void on_py_amf_call(PPROTO proto, int cid, int reqid, AV * method_name, int argc, AV * args)
{
  PYOBJ func = _get_member(proto, "handle_invoke");
  if(func) {
    Py_XINCREF(func);
    
    PYOBJ func_name = av_2_py(method_name);
    Py_XINCREF(func_name);
    
    PYOBJ arglist = PyList_New(argc + 2);
    Py_XINCREF(arglist);
    
    PyList_Append(arglist, _get_py_data(proto));
    
    PyList_Append(arglist, func_name);
    Py_XDECREF(func_name);
    
    av_list_2_py(arglist, args, argc);
    PyEval_CallObject(func, arglist);
    
    Py_XDECREF(func);
  } else {
    printf("no handle_invoke() defined\n");
  }
}

void init_responder(PPROTO proto, PYOBJ responder) {
  rtmp_proto_init(proto, &method_table);
  method_table.on_amf_call = on_py_amf_call;

  PYOBJ old = _get_py_data(proto);
  Py_XDECREF(old);
  Py_XINCREF(responder);
  rtmp_proto_set_user_data(proto, responder);
}

int feed_data(PPROTO proto, PYOBJ data) {
  char * buffer;
  int length;
  Py_XINCREF(data);
  PyString_AsStringAndSize(data, &buffer, &length);
  rtmp_proto_feed_data(proto, buffer, length);
  Py_XDECREF(data);
  return length;
}

void free_responder(PPROTO proto)
{
  PYOBJ data = _get_py_data(proto);
  Py_XDECREF(data);
  rtmp_proto_set_user_data(proto, NULL);
}

