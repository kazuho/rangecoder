#include "range_coder_interface.h"
#include <limits>

PyObject* RangeEncoder_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
	PyObject* self = type->tp_alloc(type, 0);

	if (self)
		reinterpret_cast<RangeEncoderObject*>(self)->encoder = 0;

	return self;
}


PyObject* RangeDecoder_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
	PyObject* self = type->tp_alloc(type, 0);

	if (self)
		reinterpret_cast<RangeDecoderObject*>(self)->decoder = 0;

	return self;
}


const char* RangeEncoder_doc = "A fast implementation of a range encoder.";

int RangeEncoder_init(RangeEncoderObject* self, PyObject* args, PyObject* kwds) {
	const char* kwlist[] = {"filepath", 0};
	const char* filepath = 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", const_cast<char**>(kwlist), &filepath))
		return -1;

	self->fout = new std::ofstream(filepath, std::ios::out | std::ios::binary);
	self->iter = new OutputIterator(*(self->fout));
	self->encoder = new rc_encoder_t<OutputIterator>(*(self->iter));

	return 0;
}


const char* RangeDecoder_doc = "A fast implementation of a range decoder.";

int RangeDecoder_init(RangeDecoderObject* self, PyObject* args, PyObject* kwds) {
	const char* kwlist[] = {"filepath", 0};
	const char* filepath = 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", const_cast<char**>(kwlist), &filepath))
		return -1;

	self->fin = new std::ifstream(filepath, std::ios::in | std::ios::binary);
	self->begin = new InputIterator(*(self->fin));
	self->end = new InputIterator();
	self->decoder = new rc_decoder_t<InputIterator, SearchType>(*(self->begin), *(self->end));

	return 0;
}


void RangeEncoder_dealloc(RangeEncoderObject* self) {
	if (self->encoder) {
		// flush buffer
		self->encoder->final();

		delete self->encoder;
		delete self->iter;
		delete self->fout;
	}

	Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}


void RangeDecoder_dealloc(RangeDecoderObject* self) {
	if (self->decoder) {
		delete self->decoder;
		delete self->begin;
		delete self->end;
		delete self->fin;
	}

	Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}


const char* RangeEncoder_encode_doc =
	"encode(data, cumFreq)\n"
	"\n"
	"Encodes a list of indices using the given cumulative frequency table.\n"
	"\n"
	"The length of the frequency table should be the number of possible symbols plus one.\n"
	"\n"
	"Parameters\n"
	"----------\n"
	"data : list[int]\n"
	"	A list of positive integers representing indices into cumulative frequency table\n"
	"\n"
	"cumFreq : list[int]\n"
	"	List of increasing positive integers representing cumulative frequencies";

PyObject* RangeEncoder_encode(RangeEncoderObject* self, PyObject* args, PyObject* kwds) {
	const char* kwlist[] = {"data", "cumFreq", 0};

	PyObject* data;
	PyObject* cumFreq;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", const_cast<char**>(kwlist), &data, &cumFreq))
		return 0;

	if (!self->fout->is_open()) {
		PyErr_SetString(PyExc_RuntimeError, "File closed.");
		return 0;
	}

	if (!PyList_Check(data)) {
		PyErr_SetString(PyExc_TypeError, "`data` needs to be a list.");
		return 0;
	}

	if (!PyList_Check(cumFreq)) {
		PyErr_SetString(PyExc_TypeError, "`cumFreq` needs to be a list.");
		return 0;
	}

	// load cumulative frequency table
	Py_ssize_t cumFreqLen = PyList_Size(cumFreq);

	if (cumFreqLen < 2) {
		PyErr_SetString(PyExc_ValueError, "`cumFreq` should have at least 2 entries (1 symbol).");
		return 0;
	}

	unsigned long* cumFreqArr = new unsigned long[cumFreqLen];

	for (Py_ssize_t i = 0; i < cumFreqLen; ++i) {
		cumFreqArr[i] = PyLong_AsUnsignedLong(PyList_GetItem(cumFreq, i));

		if (!PyErr_Occurred() and i > 0 and cumFreqArr[i - 1] > cumFreqArr[i])
			PyErr_SetString(PyExc_ValueError, "Entries in `cumFreq` have to be non-negative and non-decreasing.");

		if (PyErr_Occurred()) {
			delete[] cumFreqArr;
			return 0;
		}
	}

	if (cumFreqArr[0] != 0) {
		PyErr_SetString(PyExc_ValueError, "`cumFreq` should start with 0.");
		delete[] cumFreqArr;
		return 0;
	}

	if(cumFreqArr[cumFreqLen - 1] > std::numeric_limits<unsigned int>::max()) {
		PyErr_SetString(PyExc_OverflowError, "Maximal allowable resolution of frequency table exceeded.");
		return 0;
	}

	// load data
	Py_ssize_t dataLen = PyList_Size(data);
	Py_ssize_t* dataArr = new Py_ssize_t[dataLen];

	for (Py_ssize_t i = 0; i < dataLen; ++i) {
		#if PY_MAJOR_VERSION >= 3
		dataArr[i] = PyLong_AsSsize_t(PyList_GetItem(data, i));
		#else
		dataArr[i] = PyInt_AsSsize_t(PyList_GetItem(data, i));
		#endif

		if (!PyErr_Occurred() and dataArr[i] > cumFreqLen - 2)
			PyErr_SetString(PyExc_ValueError, "An entry in `data` is too large or `cumFreq` is too short.");

		if (PyErr_Occurred())
		{
			delete[] cumFreqArr;
			delete[] dataArr;
			return 0;
		}
	}

	bool positive = true;
	for (int i = 0; i < cumFreqLen - 1; ++i) {
		if (cumFreqArr[i] == cumFreqArr[i + 1]) {
			positive = false;
			break;
		}
	}

	// encode data
	if (positive) {
		// no extra checks necessary
		for (int i = 0; i < dataLen; ++i) {
			self->encoder->encode(
				cumFreqArr[dataArr[i]],
				cumFreqArr[dataArr[i] + 1],
				cumFreqArr[cumFreqLen - 1]);
		}
	} else {
		for (int i = 0; i < dataLen; ++i) {
			unsigned long start = cumFreqArr[dataArr[i]];
			unsigned long end = cumFreqArr[dataArr[i] + 1];

			if (start == end) {
				PyErr_SetString(PyExc_ValueError, "Cannot encode symbol due to zero frequency.");
				delete[] cumFreqArr;
				delete[] dataArr;
				return 0;
			}

			self->encoder->encode(start, end, cumFreqArr[cumFreqLen - 1]);
		}
	}

	delete[] cumFreqArr;
	delete[] dataArr;

	Py_INCREF(Py_None);
	return Py_None;
}


const char* RangeDecoder_decode_doc =
	"decode(size, cumFreq)\n"
	"\n"
	"Decodes a list of indices using the given cumulative frequency table.\n"
	"\n"
	"The length of the frequency table should be the number of possible symbols plus one.\n"
	"\n"
	"Parameters\n"
	"----------\n"
	"size : int\n"
	"	Number of symbols to decode\n"
	"\n"
	"cumFreq : list[int]\n"
	"	List of increasing positive integers representing cumulative frequencies\n"
	"\n"
	"Returns\n"
	"-------\n"
	"list[int]\n"
	"	List of decoded indices";

PyObject* RangeDecoder_decode(RangeDecoderObject* self, PyObject* args, PyObject* kwds) {
	const char* kwlist[] = {"size", "cumFreq", 0};

	Py_ssize_t size;
	PyObject* cumFreq;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "nO", const_cast<char**>(kwlist), &size, &cumFreq))
		return 0;

	if (!self->fin->is_open()) {
		PyErr_SetString(PyExc_RuntimeError, "File closed.");
		return 0;
	}

	if (!PyList_Check(cumFreq)) {
		PyErr_SetString(PyExc_TypeError, "`cumFreq` needs to be a list of frequencies.");
		return 0;
	}

	Py_ssize_t cumFreqLen = PyList_Size(cumFreq);

	if (cumFreqLen < 2) {
		PyErr_SetString(PyExc_ValueError, "`cumFreq` should have at least 2 entries (1 symbol).");
		return 0;
	}

	if (cumFreqLen > MAX_NUM_SYMBOLS + 1) {
		PyErr_SetString(PyExc_ValueError, "Number of symbols can be at most MAX_NUM_SYMBOLS.");
		return 0;
	}

	unsigned long resolution = PyLong_AsUnsignedLong(PyList_GetItem(cumFreq, cumFreqLen - 1));

	if (PyErr_Occurred())
		return 0;

	if(resolution > std::numeric_limits<unsigned int>::max()) {
		PyErr_SetString(PyExc_OverflowError, "Maximal allowable resolution of frequency table exceeded.");
		return 0;
	}

	// load cumulative frequency table
	SearchType::freq_type cumFreqArr[MAX_NUM_SYMBOLS + 1];

	for (Py_ssize_t i = 0; i < cumFreqLen; ++i) {
		cumFreqArr[i] = static_cast<SearchType::freq_type>(
				PyLong_AsUnsignedLong(PyList_GetItem(cumFreq, i)));

		if (!PyErr_Occurred() and i > 0 and cumFreqArr[i - 1] > cumFreqArr[i])
			PyErr_SetString(PyExc_ValueError, "Entries in `cumFreq` have to be non-negative and increasing.");

		if (PyErr_Occurred())
			return 0;
	}

	if (cumFreqArr[0] != 0) {
		PyErr_SetString(PyExc_ValueError, "`cumFreq` should start with 0.");
		return 0;
	}

	// fill up remaining entries
	for (Py_ssize_t i = cumFreqLen; i < MAX_NUM_SYMBOLS + 1; ++i)
		cumFreqArr[i] = cumFreqArr[cumFreqLen - 1];

	// decode data
	PyObject* data = PyList_New(size);
	for (Py_ssize_t i = 0; i < size; ++i) {
		rc_type_t::uint index = self->decoder->decode(cumFreqArr[MAX_NUM_SYMBOLS], cumFreqArr);

		PyList_SetItem(data, i, PyLong_FromUnsignedLong(index));

		if (PyErr_Occurred()) {
			Py_DECREF(data);
			return 0;
		}
	}

	return data;
}


const char* RangeEncoder_close_doc =
	"close()\n"
	"\n"
	"Write remaining bits in buffer and close file.";

PyObject* RangeEncoder_close(RangeEncoderObject* self, PyObject* args, PyObject* kwds) {
	self->encoder->final();
	self->fout->close();

	Py_INCREF(Py_None);
	return Py_None;
}


const char* RangeDecoder_close_doc =
	"close()\n"
	"\n"
	"Close file.";

PyObject* RangeDecoder_close(RangeDecoderObject* self, PyObject* args, PyObject* kwds) {
	self->fin->close();

	Py_INCREF(Py_None);
	return Py_None;
}
