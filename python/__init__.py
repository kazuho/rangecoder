from warnings import warn
from range_coder._range_coder import RangeEncoder, RangeDecoder  # noqa: F401

try:
    import numpy as np
except ImportError:
    pass


def prob_to_cum_freq(prob, resolution=1024):
    """
    Converts probability distribution into a cumulative frequency table.

    Makes sure that non-zero probabilities are represented by non-zero frequencies,
    provided that :samp:`len({prob}) <= {resolution}`.

    Parameters
    ----------
    prob : ndarray or list
        A one-dimensional array representing a probability distribution

    resolution : int
        Number of hypothetical samples used to generate integer frequencies

    Returns
    -------
    list
        Cumulative frequency table
    """

    if len(prob) > resolution:
        warn('Resolution smaller than number of symbols.')

    prob = np.asarray(prob, dtype=np.float64)
    freq = np.zeros(prob.size, dtype=int)

    # this is similar to gradient descent in KL divergence (convex)
    with np.errstate(divide='ignore', invalid='ignore'):
        for _ in range(resolution):
            freq[np.nanargmax(prob / freq)] += 1

    return [0] + np.cumsum(freq).tolist()


def cum_freq_to_prob(cumFreq):
    """
    Converts a cumulative frequency table into a probability distribution.

    Parameters
    ----------
    cumFreq : list
        Cumulative frequency table

    Returns
    -------
    ndarray
        Probability distribution
    """
    return np.diff(cumFreq).astype(np.float64) / cumFreq[-1]
