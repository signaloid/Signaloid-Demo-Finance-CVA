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

#include <stddef.h>
#include <uxhw.h>
#include "kernel.h"
#include "utilities.h"
#include "common.h"
#include "cva-config.h"
#include "cva-uxhw.h"
#include "cva-monte-carlo.h"

/*
 *	The portfolio mirrors the notebook's two-swap netting set:
 *
 *		makeSwap(today + 2d, Period("5Y"), 1e6, 0.03, euribor6m, Payer)
 *		makeSwap(today + 2d, Period("4Y"), 5e5, 0.03, euribor6m, Receiver)
 *
 *	We approximate the +2d spot lag with t_start = 0, treat the fixed leg
 *	as annual Thirty360 (tau = 1.0) and the floating leg as semi-annual
 *	Actual/360 (tau = 0.5). Both schedules use the same flat discount
 *	curve (single-curve approximation).
 */
static const CvaSwap kCvaPortfolio[kCvaNumPortfolioSwaps] = {
	{
		.startYears = 0.0,
		.endYears   = 5.0,
		.notional   = 1e6,
		.fixedRate  = 0.03,
		.fixedTau   = 1.0,
		.floatTau   = 0.5,
		.type       = kCvaSwapTypePayer
	},
	{
		.startYears = 0.0,
		.endYears   = 4.0,
		.notional   = 5e5,
		.fixedRate  = 0.03,
		.fixedTau   = 1.0,
		.floatTau   = 0.5,
		.type       = kCvaSwapTypeReceiver
	}
};

/*
 *	Resolve the Hull-White volatility for one kernel invocation. When the
 *	volatility was fixed on the command line (-x) or read from an input CSV
 *	(-i), that value is used. Otherwise the volatility is the uncertain model
 *	input. In UxHw mode UxHwDoubleGaussDist() returns the full Gaussian
 *	distribution (one distributional pass). In native Monte Carlo mode it
 *	returns a fresh sample on each call, so each iteration draws its own
 *	volatility.
 */
static double
cvaResolveVolatility(
	const CommandLineArguments *    arguments,
	const double *                  inputVariables)
{
	if (arguments->isInputVariableSet[kCvaInputIndexVolatility] ||
	    arguments->common.isInputFromFileEnabled)
	{
		return inputVariables[kCvaInputIndexVolatility];
	}

	return UxHwDoubleGaussDist(kCvaDefaultVolatility, kCvaDefaultVolatilityStdDev);
}

double
calculateOutputUxHw(
	CommandLineArguments *  arguments,
	double *                inputVariables,
	double *                outputVariables,
	double *                monteCarloOutputSamples)
{
	double  volatility  = cvaResolveVolatility(arguments, inputVariables);
	double  cva         = cvaCalculatePathCvaUxHw(
		kCvaPortfolio,
		kCvaNumPortfolioSwaps,
		volatility
	);

	outputVariables[kCvaOutputIndexCva] = cva;
	monteCarloOutputSamples[0]          = cva;

	return cva;
}

double
calculateOutputMonteCarlo(
	CommandLineArguments *  arguments,
	double *                inputVariables,
	double *                outputVariables,
	double *                monteCarloOutputSamples)
{
	size_t          numberOfMonteCarloIterations = arguments->common.numberOfMonteCarloIterations;
	MeanAndVariance meanAndVariance;

	for (size_t ii = 0; ii < numberOfMonteCarloIterations; ii++)
	{
		double volatility = cvaResolveVolatility(arguments, inputVariables);

		monteCarloOutputSamples[ii] = cvaCalculatePathCvaMonteCarlo(
			kCvaPortfolio,
			kCvaNumPortfolioSwaps,
			volatility
		);
	}

	/*
	 *	The Monte Carlo estimator of the CVA is the mean of the per-path
	 *	samples. Compute it before overwriting `monteCarloOutputSamples[0]`
	 *	so that the single value persisted to `data.out` is the converged CVA.
	 */
	meanAndVariance = calculateMeanAndVarianceOfDoubleSamples(
		monteCarloOutputSamples,
		numberOfMonteCarloIterations
	);

	outputVariables[kCvaOutputIndexCva] = meanAndVariance.mean;
	monteCarloOutputSamples[0]          = meanAndVariance.mean;

	return outputVariables[kCvaOutputIndexCva];
}
