/*
 *	Copyright (c) 2023-2026, Signaloid.
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

#include "utilities.h"
#include "cva-config.h"

/**
 *	@brief	UxHw calculation kernel. Computes the CVA in a single distributional
 *		pass (see `cva-uxhw.c`) and writes it into `outputVariables` and
 *		`monteCarloOutputSamples[0]`.
 *
 *	@param	arguments		: Pointer to command line arguments struct.
 *	@param	inputVariables		: The input variables (resolved volatility).
 *	@param	outputVariables		: Array of size `kCvaOutputIndexMax` to fill.
 *	@param	monteCarloOutputSamples	: Single-element array for the distributional result.
 *	@return	double			: The value of the selected output (the CVA).
 */
double
calculateOutputUxHw(
	CommandLineArguments *  arguments,
	double *                inputVariables,
	double *                outputVariables,
	double *                monteCarloOutputSamples);

/**
 *	@brief	Monte Carlo calculation kernel. Runs
 *		`arguments->common.numberOfMonteCarloIterations` independent scalar
 *		short-rate paths (see `cva-monte-carlo.c`) into
 *		`monteCarloOutputSamples`, then writes their mean (the converged CVA)
 *		into `outputVariables`.
 *
 *	@param	arguments		: Pointer to command line arguments struct.
 *	@param	inputVariables		: The input variables (resolved volatility).
 *	@param	outputVariables		: Array of size `kCvaOutputIndexMax` to fill.
 *	@param	monteCarloOutputSamples	: Array of `numberOfMonteCarloIterations` doubles.
 *	@return	double			: The value of the selected output (the CVA).
 */
double
calculateOutputMonteCarlo(
	CommandLineArguments *  arguments,
	double *                inputVariables,
	double *                outputVariables,
	double *                monteCarloOutputSamples);
