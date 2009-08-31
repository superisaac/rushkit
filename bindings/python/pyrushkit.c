#include <Python.h>
#include "rushkit.h"

typedef  PyObject * PYOBJ;
static RTMP_METHOD_TABLE method_table;

extern PyObject * av_2_py(AV*);
extern PyObject * av_list_2_py(AV*, int);

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

int on_py_write_data(PPROTO proto, byte * data, int len)
{
  PyObject * func = _get_member(proto, "handleWriteData");
  if(func) {
    Py_XINCREF(func);

    PyObject * arglist = Py_BuildValue("(s#)",data, len);
    Py_XINCREF(arglist);

    PyObject_CallObject(func, arglist);    
    Py_XDECREF(arglist);
    Py_XDECREF(func);
    return len;
  } else {
    printf("no method handleWriteData()");
  }
  return 0;
}

void on_py_amf_call(PPROTO proto, int cid, double reqid, AV * method_name, int argc, AV * args)
{
  PYOBJ func = _get_member(proto, "handleInvoke");
  if(func) {
    Py_XINCREF(func);
    
    PYOBJ func_name = av_2_py(method_name);
    Py_XINCREF(func_name);
    
    PYOBJ call_args = av_list_2_py(args, argc);
    Py_XINCREF(call_args);
    
    PYOBJ arglist = Py_BuildValue("(dOO)", reqid, func_name, call_args);
    Py_XINCREF(arglist);

    PyObject_CallObject(func, arglist);

    Py_XDECREF(call_args);
    Py_XDECREF(func_name);
    Py_XDECREF(func);
  } else {
    printf("no handleInvoke() defined\n");
  }
}

void init_responder(PPROTO proto, PYOBJ responder) {
  rtmp_proto_init(proto, &method_table);
  method_table.on_amf_call = on_py_amf_call;
  method_table.on_write_data = on_py_write_data;

  PYOBJ old = _get_py_data(proto);
  Py_XDECREF(old);
  Py_XINCREF(responder);
  rtmp_proto_set_user_data(proto, responder);
}

int feed_data(PPROTO proto, PYOBJ data) {
  char * buffer;
  Py_ssize_t length;
  Py_XINCREF(data);
  PyString_AsStringAndSize(data, &buffer, &length);
  rtmp_proto_feed_data(proto, (byte*)buffer, length);
  Py_XDECREF(data);
  return length;
}

void free_responder(PPROTO proto)
{
  PYOBJ data = _get_py_data(proto);
  Py_XDECREF(data);
  rtmp_proto_set_user_data(proto, NULL);
}

void amf_return(PPROTO proto, double reqid, PyObject * retv)
{
  POOL pool;
  rt_pool_init(&pool);
  AV dest;
  py_2_av(&pool, &dest, retv, 0);
  rtmp_proto_packet_return(proto, &pool, 3, reqid, &dest);
  rt_pool_free(&pool);
}
