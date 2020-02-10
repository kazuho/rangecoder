#ifndef RANGE_CODER_INTERFACE_H
#define RANGE_CODER_INTERFACE_H

#include <Python.h>
#include <fstream>
#include <iterator>
#include "range_coder.hpp"

#define MAX_NUM_SYMBOLS 1024

extern const char* RangeEncoder_doc;
extern const char* RangeEncoder_encode_doc;
extern const char* RangeEncoder_close_doc;
extern const char* RangeDecoder_doc;
extern const char* RangeDecoder_decode_doc;
extern const char* RangeDecoder_close_doc;

typedef std::ostream_iterator<rc_type_t::byte> OutputIterator;
typedef std::istreambuf_iterator<char> InputIterator;
typedef rc_decoder_search_t<unsigned int, MAX_NUM_SYMBOLS, 0> SearchType;

struct RangeEncoderObject {
	PyObject_HEAD
	rc_encoder_t<OutputIterator>* encoder;
	OutputIterator* iter;
	std::ofstream* fout;
};

struct RangeDecoderObject {
	PyObject_HEAD
	rc_decoder_t<InputIterator, SearchType>* decoder;
	InputIterator* begin;
	InputIterator* end;
	std::ifstream* fin;
};

PyObject* RangeEncoder_new(PyTypeObject*, PyObject*, PyObject*);
PyObject* RangeDecoder_new(PyTypeObject*, PyObject*, PyObject*);

int RangeEncoder_init(RangeEncoderObject*, PyObject*, PyObject*);
int RangeDecoder_init(RangeDecoderObject*, PyObject*, PyObject*);

void RangeEncoder_dealloc(RangeEncoderObject*);
void RangeDecoder_dealloc(RangeDecoderObject*);

PyObject* RangeEncoder_encode(RangeEncoderObject*, PyObject*, PyObject*);
PyObject* RangeDecoder_decode(RangeDecoderObject*, PyObject*, PyObject*);

PyObject* RangeEncoder_close(RangeEncoderObject*, PyObject*, PyObject*);
PyObject* RangeDecoder_close(RangeDecoderObject*, PyObject*, PyObject*);

#endif
