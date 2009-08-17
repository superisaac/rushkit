#include <Python.h>
#include "rushkit.h"

PyObject * av_table_2_py(AK_TABLE * table_t)
{
  return Py_None;
}

PyObject * av_array_2_py(AK_TABLE * table_t)
{
  return Py_None;
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
