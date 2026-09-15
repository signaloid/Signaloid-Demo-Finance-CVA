/*
 *	Copyright (c) 2026, Signaloid.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a copy
 *	of this software and associated documentation files (the "Software"), to deal
 *	in the Software without restriction, including without limitation the rights
 *	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *	copies of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be included in all
 *	copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
 */

#pragma once

#include <stddef.h>
#include "cva-config.h"

/**
 *	@brief	UxHw single-pass CVA kernel.
 *
 *	@details Simulates one distributional short-rate path: the auxiliary OU
 *		state `x(t)` is carried as a probability distribution, so a single
 *		pass over the time grid captures the full cross-sectional
 *		distribution of the portfolio exposure at every date. The positive
 *		exposure `E[max(V(t), 0)]` at each step is computed with the
 *		distributional UxHw API (`UxHwDoubleLimitDistributionSupport`,
 *		`UxHwDoubleProbabilityGT`, `UxHwDoubleMixture`) and the accumulated
 *		discounted exposure is collapsed to its expectation with
 *		`UxHwDoubleNthMoment`. Equivalent to a large Monte Carlo ensemble in
 *		one execution.
 *
 *	@param	swaps		: The netting-set swaps.
 *	@param	numSwaps	: Number of swaps in the netting set.
 *	@param	volatility	: Hull-White short-rate volatility (may be distributional).
 *	@return	double		: The unilateral CVA (scalar expectation).
 */
double
cvaCalculatePathCvaUxHw(
	const CvaSwap * swaps,
	size_t          numSwaps,
	double          volatility);
