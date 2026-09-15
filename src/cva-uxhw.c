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
 *	cva-uxhw.c
 *
 *	UxHw (single distributional pass) implementation of the per-path CVA
 *	accumulation. The auxiliary OU state and the resulting portfolio exposure
 *	are carried as probability distributions, so one pass over the time grid
 *	yields the expected positive exposure at every date without an outer Monte
 *	Carlo loop. See `cva-monte-carlo.c` for the native Monte Carlo counterpart.
 */

#include <float.h>
#include <math.h>
#include <uxhw.h>
#include "cva-config.h"
#include "cva-uxhw.h"

double
cvaCalculatePathCvaUxHw(
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
		 *	Advance the OU state x by one time step. UxHwDoubleGaussDist()
		 *	returns the full standard-normal distribution, so x carries its
		 *	complete distribution forward.
		 */
		double z = UxHwDoubleGaussDist(0.0, 1.0);
		x   = UxHwDoubleGetIndependentCopy(x) * exp(-kCvaMeanReversion * dt) + stdDev * z;
		t   = (double) ii * dt;

		double v = cvaPortfolioValue(swaps, numSwaps, kCvaMeanReversion, volatility, t, x);

		/*
		 *	Expected positive exposure E[max(V, 0)]. We cannot apply fmax to a
		 *	distribution, so we take the fraction of probability mass with
		 *	positive support and mix the positively-truncated distribution with
		 *	a Dirac delta at zero.
		 */
		double  fraction    = UxHwDoubleProbabilityGT(v, 0);
		double  ee          = 0.0;

		if (fraction > 0.0)
		{
			double truncated = UxHwDoubleLimitDistributionSupport(v, 0, DBL_MAX);
			ee = UxHwDoubleMixture(truncated, 0, fraction);
		}

		double df = exp(-kCvaFlatRate * t);

		/*
		 *	Default probability over the previous interval.
		 */
		double  currentSurvival = cvaSurvivalProbability(t);
		double  dPd             = prevSurvival - currentSurvival;

		cvaContribution += ee * df * dPd;
		prevSurvival    = currentSurvival;
	}

	/*
	 *	CVA is, by definition, the expectation of the path-CVA. On UxHw we
	 *	extract the first moment of the accumulated distributional value so
	 *	that the output variable is the scalar CVA rather than theper-path
	 * 	CVA distribution.
	 */
	return (1.0 - kCvaRecoveryRate) * UxHwDoubleNthMoment(cvaContribution, 1);
}
