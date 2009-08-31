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

PyObject * av_list_2_py(AV * avalues, int size)
{
  int i;
  PyObject * plist = PyList_New(0);

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
  return av_list_2_py(array->elements, array->size);
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

AK_STRING * pyobj_2_string(POOL * ppool, AK_STRING * str, PyObject * pyobj);
AV * py_2_av(POOL * ppool, AV * dest, PyObject * pyobj, int type);

AV * pydict_2_hash(POOL * ppool, AV * dest, PyObject * pyobj)
{
  PyObject * key, *value;
  Py_ssize_t pos = 0;
  int i = 0;
  Py_ssize_t dict_size = PyDict_Size(pyobj);
  AK_TABLE_ELEM * elements = (AK_TABLE_ELEM *)rt_pool_alloc(ppool, 
					   dict_size * sizeof(AK_TABLE_ELEM));
  while(PyDict_Next(pyobj, &pos, &key, &value)) {
    if(PyString_Check(key)) {
      pyobj_2_string(ppool, &elements[i].key, key);
      py_2_av(ppool, &(elements[i].value), value, 0);
    } else {
      printf("It %d is not string\n", pos);
    }
    i++;
  }
  amf_new_hash(ppool, dest, dict_size, elements);
  return dest;  
}

AV * pylist_2_array(POOL * ppool, AV * dest, PyObject * pyobj)
{
  Py_ssize_t list_size = PyList_Size(pyobj);
  Py_ssize_t i;
  AV * elements = (AV*)rt_pool_alloc(ppool, list_size * sizeof(AV));
  for(i=0; i< list_size; i++) {
    PyObject * elem = PyList_GetItem(pyobj, i);
    py_2_av(ppool, elements + i, elem, 0);
  }
  amf_new_array(ppool, dest, list_size, elements);
  return dest;
}


AK_STRING * pyobj_2_string(POOL * ppool, AK_STRING * str, PyObject * pyobj)
{
  char * buffer;
  Py_ssize_t length;
  PyString_AsStringAndSize(pyobj, &buffer, &length);
  amf_new_count_string(ppool, str, length, buffer);
  return str;
}

AV * py_2_av(POOL * ppool, AV * dest, PyObject * pyobj, int type)
{
  if(PyDict_Check(pyobj)) {
    dest->type = AMF_HASH;
    pydict_2_hash(ppool, dest, pyobj);
  } else if(PyList_Check(pyobj)) {
    dest->type = AMF_ARRAY;
    pylist_2_array(ppool, dest, pyobj);
  } else if(PyString_Check(pyobj)) {
    dest->type = AMF_STRING;
    pyobj_2_string(ppool, &dest->value.string_t, pyobj);
  } else if(PyBool_Check(pyobj)) {
    dest->type = AMF_BOOLEAN;
    int boolv = (pyobj == Py_True);
    amf_new_bool(dest, boolv);
  } else if(PyFloat_Check(pyobj)) {
    dest->type = AMF_NUMBER;
    amf_new_number(dest, PyFloat_AsDouble(pyobj));
  } else if(type != 0) {
    dest->type = type;    
  }
  return dest;
}
