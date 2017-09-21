import os
import random
import sys
from tempfile import mkstemp

import numpy as np
import pytest

from range_coder import RangeEncoder, RangeDecoder
from range_coder import prob_to_cum_freq, cum_freq_to_prob


def test_range_coder_overflow():
    """
    Cumulative frequencies must fit in an unsigned integer (assumed to be represented by 32 bits).
    This test checks that no error is thrown if the frequencies exceed that limit.
    """

    numBytes = 17
    filepath = mkstemp()[1]

    # encoding one sequence should require 1 byte
    prob = [4, 6, 8]
    prob = np.asarray(prob, dtype=np.float64) / np.sum(prob)
    cumFreq = prob_to_cum_freq(prob, 128)
    cumFreq[-1] = 2**32

    sequence = [2, 2]
    data = sequence * numBytes

    encoder = RangeEncoder(filepath)
    with pytest.raises(OverflowError):
        encoder.encode(data, cumFreq)
    encoder.close()


def test_range_encoder():
    """
    Tests that RangeEncoder writes the expected bits.

    Tests that writing after closing file throws an exception.
    """

    numBytes = 17
    filepath = mkstemp()[1]

    # encoding one sequence should require 1 byte
    cumFreq = [0, 4, 6, 8]
    sequence = [0, 0, 0, 0, 1, 2]
    sequenceByte = b'\x0b'
    data = sequence * numBytes

    encoder = RangeEncoder(filepath)
    encoder.encode(data, cumFreq)
    encoder.close()

    with pytest.raises(RuntimeError):
        # file is already closed, should raise an exception
        encoder.encode(sequence, cumFreq)

    assert os.stat(filepath).st_size == numBytes

    with open(filepath, 'rb') as handle:
        # the first 4 bytes are special
        handle.read(4)

        for _ in range(numBytes - 4):
            assert handle.read(1) == sequenceByte

    encoder = RangeEncoder(filepath)
    with pytest.raises(OverflowError):
        # cumFreq contains negative frequencies
        encoder.encode(data, [-1, 1])
    with pytest.raises(ValueError):
        # cumFreq does not start at zero
        encoder.encode(data, [1, 2, 3])
    with pytest.raises(ValueError):
        # cumFreq too short
        encoder.encode(data, [0, 1])
    with pytest.raises(ValueError):
        # symbols with zero probability cannot be encoded
        encoder.encode(data, [0, 8, 8, 8])
    with pytest.raises(ValueError):
        # invalid frequency table
        encoder.encode(data, [])
    with pytest.raises(ValueError):
        # invalid frequency table
        encoder.encode(data, [0])
    encoder.close()

    os.remove(filepath)


def test_range_decoder():
    """
    Tests whether RangeDecoder reproduces symbols encoded by RangeEncoder.
    """

    random.seed(558)

    filepath = mkstemp()[1]

    # encoding one sequence should require 1 byte
    cumFreq0 = [0, 4, 6, 8]
    cumFreq1 = [0, 2, 5, 7, 10, 14]
    data0 = [random.randint(0, len(cumFreq0) - 2) for _ in range(10)]
    data1 = [random.randint(0, len(cumFreq1) - 2) for _ in range(17)]

    encoder = RangeEncoder(filepath)
    encoder.encode(data0, cumFreq0)
    encoder.encode(data1, cumFreq1)
    encoder.close()

    decoder = RangeDecoder(filepath)
    dataRec0 = decoder.decode(len(data0), cumFreq0)
    dataRec1 = decoder.decode(len(data1), cumFreq1)
    decoder.close()

    # encoded and decoded data should be the same
    assert data0 == dataRec0
    assert data1 == dataRec1

    # make sure reference counting is implemented correctly (call to getrefcount increases it by 1)
    assert sys.getrefcount(dataRec0) == 2
    assert sys.getrefcount(dataRec1) == 2

    decoder = RangeDecoder(filepath)
    with pytest.raises(ValueError):
        # invalid frequency table
        decoder.decode(len(data0), [])
    with pytest.raises(ValueError):
        # invalid frequency table
        decoder.decode(len(data0), [0])
    assert decoder.decode(0, cumFreq0) == []

    os.remove(filepath)


def test_range_decoder_fuzz():
    """
    Test random inputs to the decoder.
    """

    random.seed(827)
    randomState = np.random.RandomState(827)

    for _ in range(10):
        # generate random frequency table
        numSymbols = random.randint(1, 20)
        maxFreq = random.randint(2, 100)
        cumFreq = np.cumsum(randomState.randint(1, maxFreq, size=numSymbols))
        cumFreq = [0] + [int(i) for i in cumFreq]  # convert numpy.int64 to int

        # decode random symbols
        decoder = RangeDecoder('/dev/urandom')
        decoder.decode(100, cumFreq)


def test_range_encoder_fuzz():
    """
    Test random inputs to the encoder.
    """

    random.seed(111)
    randomState = np.random.RandomState(111)

    filepath = mkstemp()[1]

    for _ in range(10):
        # generate random frequency table
        numSymbols = random.randint(1, 20)
        maxFreq = random.randint(2, 100)
        cumFreq = np.cumsum(randomState.randint(1, maxFreq, size=numSymbols))
        cumFreq = [0] + [int(i) for i in cumFreq]  # convert numpy.int64 to int

        # encode random symbols
        dataLen = randomState.randint(0, 10)
        data = [random.randint(0, numSymbols - 1) for _ in range(dataLen)]
        encoder = RangeEncoder(filepath)
        encoder.encode(data, cumFreq)
        encoder.close()

    os.remove(filepath)


def test_prob_to_cum_freq():
    """
    Tests whether prob_to_cum_freq produces a table with the expected number
    of entries, number of samples, and that non-zero probabilities are
    represented by non-zero increases in frequency.

    Tests that cum_freq_to_prob is normalized and consistent with prob_to_cum_freq.
    """

    randomState = np.random.RandomState(190)
    resolution = 1024

    p0 = randomState.dirichlet([.1] * 50)
    cumFreq0 = prob_to_cum_freq(p0, resolution)
    p1 = cum_freq_to_prob(cumFreq0)
    cumFreq1 = prob_to_cum_freq(p1, resolution)

    # number of hypothetical samples should correspond to resolution
    assert cumFreq0[-1] == resolution
    assert len(cumFreq0) == len(p0) + 1

    # non-zero probabilities should have non-zero frequencies
    assert np.all(np.diff(cumFreq0)[p0 > 0.] > 0)

    # probabilities should be normalized.
    assert np.isclose(np.sum(p1), 1.)

    # while the probabilities might change, frequencies should not
    assert cumFreq0 == cumFreq1


def test_prob_to_cum_freq_zero_prob():
    """
    Tests whether prob_to_cum_freq handles zero probabilities as expected.
    """

    prob1 = [0.5, 0.25, 0.25]
    cumFreq1 = prob_to_cum_freq(prob1, resolution=8)

    prob0 = [0.5, 0., 0.25, 0.25, 0., 0.]
    cumFreq0 = prob_to_cum_freq(prob0, resolution=8)

    # removing entries corresponding to zeros
    assert [cumFreq0[0]] + [cumFreq0[i + 1] for i, p in enumerate(prob0) if p > 0.] == cumFreq1
