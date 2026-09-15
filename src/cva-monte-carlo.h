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
 *	@brief	Native Monte Carlo per-path CVA kernel.
 *
 *	@details Simulates a single scalar short-rate trajectory (each Gaussian
 *		increment is one sample), reprices the portfolio at every date, and
 *		accumulates the discounted positive exposure using ordinary `fmax`
 *		payoff math (no distributional UxHw API). The caller runs this over
 *		many iterations and averages to estimate the expected CVA. See
 *		`cva-uxhw.c` for the single-pass distributional counterpart.
 *
 *	@param	swaps		: The netting-set swaps.
 *	@param	numSwaps	: Number of swaps in the netting set.
 *	@param	volatility	: Hull-White short-rate volatility (one sample or fixed value).
 *	@return	double		: One sample of the unilateral CVA.
 */
double
cvaCalculatePathCvaMonteCarlo(
	const CvaSwap * swaps,
	size_t          numSwaps,
	double          volatility);
