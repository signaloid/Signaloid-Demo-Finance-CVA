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

/*
 *	cva-monte-carlo.c
 *
 *	Native Monte Carlo implementation of the per-path CVA accumulation. Each
 *	call simulates one scalar short-rate trajectory and returns a single sample
 *	of the path CVA. The outer Monte Carlo loop (in `kernel.c`) averages these
 *	samples to estimate the expected CVA. The positive exposure is computed
 *	with ordinary `fmax` payoff math rather than the distributional UxHw API
 *	used by `cva-uxhw.c`.
 */

#include <math.h>
#include <uxhw.h>
#include "cva-config.h"
#include "cva-monte-carlo.h"

double
cvaCalculatePathCvaMonteCarlo(
	const CvaSwap * swaps,
	size_t          numSwaps,
	double          volatility)
{
	double  dt              = kCvaTimeGridYears / (double) kCvaNumTimeSteps;
	double  x               = 0.0;
	double  t               = 0.0;
	double  prevSurvival    = 1.0;
	double  cvaContribution = 0.0;

	double variance =
		(volatility * volatility) *
		(1.0 - exp(-2.0 * kCvaMeanReversion * dt)) /
		(2.0 * kCvaMeanReversion);

	double stdDev = sqrt(variance);

	for (size_t ii = 1; ii <= (size_t) kCvaNumTimeSteps; ii++)
	{
		/*
		 *	Advance the OU state x by one time step.
		 */
		double z = UxHwDoubleGaussDist(0.0, 1.0);
		x   = x * exp(-kCvaMeanReversion * dt) + stdDev * z;
		t   = (double) ii * dt;

		double v = cvaPortfolioValue(swaps, numSwaps, kCvaMeanReversion, volatility, t, x);

		/*
		 *	Positive exposure of this trajectory.
		 */
		double  ee  = fmax(v, 0.0);
		double  df  = exp(-kCvaFlatRate * t);

		/*
		 *	Default probability over the previous interval.
		 */
		double  currentSurvival = cvaSurvivalProbability(t);
		double  dPd             = prevSurvival - currentSurvival;

		cvaContribution += ee * df * dPd;
		prevSurvival    = currentSurvival;
	}

	return (1.0 - kCvaRecoveryRate) * cvaContribution;
}
