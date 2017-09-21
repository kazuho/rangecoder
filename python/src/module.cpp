#include <Python.h>
#include "range_coder_interface.h"

static PyMethodDef RangeEncoder_methods[] = {
  {"encode",
    (PyCFunction)RangeEncoder_encode,
    METH_VARARGS | METH_KEYWORDS,
    RangeEncoder_encode_doc},
  {"close",
    (PyCFunction)RangeEncoder_close,
    METH_VARARGS | METH_KEYWORDS,
    RangeEncoder_close_doc},
  {0}
};


static PyGetSetDef RangeEncoder_getset[] = {
  {0}
};


PyTypeObject RangeEncoder_type = {
  PyVarObject_HEAD_INIT(0, 0)
  "range_coder.RangeEncoder",       /*tp_name*/
  sizeof(RangeEncoderObject),       /*tp_basicsize*/
  0,                                /*tp_itemsize*/
  (destructor)RangeEncoder_dealloc, /*tp_dealloc*/
  0,                                /*tp_print*/
  0,                                /*tp_getattr*/
  0,                                /*tp_setattr*/
  0,                                /*tp_compare*/
  0,                                /*tp_repr*/
  0,                                /*tp_as_number*/
  0,                                /*tp_as_sequence*/
  0,                                /*tp_as_mapping*/
  0,                                /*tp_hash */
  0,                                /*tp_call*/
  0,                                /*tp_str*/
  0,                                /*tp_getattro*/
  0,                                /*tp_setattro*/
  0,                                /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,               /*tp_flags*/
  RangeEncoder_doc,                 /*tp_doc*/
  0,                                /*tp_traverse*/
  0,                                /*tp_clear*/
  0,                                /*tp_richcompare*/
  0,                                /*tp_weaklistoffset*/
  0,                                /*tp_iter*/
  0,                                /*tp_iternext*/
  RangeEncoder_methods,             /*tp_methods*/
  0,                                /*tp_members*/
  RangeEncoder_getset,              /*tp_getset*/
  0,                                /*tp_base*/
  0,                                /*tp_dict*/
  0,                                /*tp_descr_get*/
  0,                                /*tp_descr_set*/
  0,                                /*tp_dictoffset*/
  (initproc)RangeEncoder_init,      /*tp_init*/
  0,                                /*tp_alloc*/
  RangeEncoder_new,                 /*tp_new*/
};

static PyMethodDef RangeDecoder_methods[] = {
  {"decode",
    (PyCFunction)RangeDecoder_decode,
    METH_VARARGS | METH_KEYWORDS,
    RangeDecoder_decode_doc},
  {"close",
    (PyCFunction)RangeDecoder_close,
    METH_VARARGS | METH_KEYWORDS,
    RangeDecoder_close_doc},
  {0}
};


static PyGetSetDef RangeDecoder_getset[] = {
  {0}
};


PyTypeObject RangeDecoder_type = {
  PyVarObject_HEAD_INIT(0, 0)
  "range_coder.RangeDecoder",       /*tp_name*/
  sizeof(RangeDecoderObject),       /*tp_basicsize*/
  0,                                /*tp_itemsize*/
  (destructor)RangeDecoder_dealloc, /*tp_dealloc*/
  0,                                /*tp_print*/
  0,                                /*tp_getattr*/
  0,                                /*tp_setattr*/
  0,                                /*tp_compare*/
  0,                                /*tp_repr*/
  0,                                /*tp_as_number*/
  0,                                /*tp_as_sequdece*/
  0,                                /*tp_as_mapping*/
  0,                                /*tp_hash */
  0,                                /*tp_call*/
  0,                                /*tp_str*/
  0,                                /*tp_getattro*/
  0,                                /*tp_setattro*/
  0,                                /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,               /*tp_flags*/
  RangeDecoder_doc,                 /*tp_doc*/
  0,                                /*tp_traverse*/
  0,                                /*tp_clear*/
  0,                                /*tp_richcompare*/
  0,                                /*tp_weaklistoffset*/
  0,                                /*tp_iter*/
  0,                                /*tp_iternext*/
  RangeDecoder_methods,             /*tp_methods*/
  0,                                /*tp_members*/
  RangeDecoder_getset,              /*tp_getset*/
  0,                                /*tp_base*/
  0,                                /*tp_dict*/
  0,                                /*tp_descr_get*/
  0,                                /*tp_descr_set*/
  0,                                /*tp_dictoffset*/
  (initproc)RangeDecoder_init,      /*tp_init*/
  0,                                /*tp_alloc*/
  RangeDecoder_new,                 /*tp_new*/
};

#if PY_MAJOR_VERSION >= 3
static PyModuleDef range_coder_module = {
  PyModuleDef_HEAD_INIT,
  "_range_coder",
  "A fast implementation of a range encoder and decoder."
  -1, 0, 0, 0, 0, 0
};
#endif


#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit__range_coder() {
  // create module object
  PyObject* module = PyModule_Create(&range_coder_module);
#define RETVAL 0;
#else
PyMODINIT_FUNC init_range_coder() {
  PyObject* module = Py_InitModule3(
    "_range_coder", 0, "A fast implementation of a range encoder and decoder.");
#define RETVAL void();
#endif

  if(!module)
    return RETVAL;

  // initialize types
  if(PyType_Ready(&RangeEncoder_type) < 0)
    return RETVAL;
  if(PyType_Ready(&RangeDecoder_type) < 0)
    return RETVAL;

  // add types to module
  Py_INCREF(&RangeEncoder_type);
  PyModule_AddObject(module, "RangeEncoder", reinterpret_cast<PyObject*>(&RangeEncoder_type));
  Py_INCREF(&RangeDecoder_type);
  PyModule_AddObject(module, "RangeDecoder", reinterpret_cast<PyObject*>(&RangeDecoder_type));

#if PY_MAJOR_VERSION >= 3
  return module;
#endif
}
