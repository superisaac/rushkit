#include <Python.h>
#include "rushkit.h"

PyObject * av_2_py(AV * pvalue);

PyObject * av_table_2_py(AK_TABLE * table)
{
  int i;
  PyObject * pdict = PyDict_New();
  Py_XINCREF(pdict);
  for(i = 0; i< table->size; i++) {
    PyObject * key = PyString_FromStringAndSize((char*)(table->elements[i].key.content),
						table->elements[i].key.size);
    Py_XINCREF(key);
    PyObject * value = av_2_py(&(table->elements[i].value));
    Py_XINCREF(value);
    PyDict_SetItem(pdict, key, value);
    Py_XDECREF(key);
    Py_XDECREF(value);
  }
  Py_XDECREF(pdict);
  return Py_None;
}

PyObject * av_list_2_py(PyObject * plist, AV * avalues, int size)
{
  int i;
  if(plist == NULL) {
    plist = PyList_New(size);
  }
  Py_XINCREF(plist);
  for(i = 0; i< size; i++) {
    PyObject * elem = av_2_py(avalues + i);
    Py_XINCREF(elem);
    PyList_Append(plist, elem);
    Py_XDECREF(elem);
  }
  Py_XDECREF(plist);
  return plist;
}

PyObject * av_array_2_py(AK_ARRAY * array)
{
  return av_list_2_py(NULL, array->elements, array->size);
}

PyObject * av_2_py(AV * pvalue)
{
  switch(pvalue->type) {
  case AMF_NUMBER:
    return Py_BuildValue("f", pvalue->value.number_t);
  case AMF_BOOLEAN:
    return Py_BuildValue("i", pvalue->value.bool_t);
  case AMF_STRING:
    return Py_BuildValue("z#", 
			 pvalue->value.string_t.content,
			 pvalue->value.string_t.size);
  case AMF_ASSOC_ARRAY:
  case AMF_HASH:
    return av_table_2_py(&(pvalue->value.table_t));
  case AMF_ARRAY:
    return av_array_2_py(&(pvalue->value.array_t));
  case AMF_LINK:
    return Py_BuildValue("i", pvalue->value.link_t);
  case AMF_NIL:
  case AMF_END:
  case AMF_UNDEF:
  case AMF_UNDEF_OBJ:
  default:
    return Py_None;
  }
}
