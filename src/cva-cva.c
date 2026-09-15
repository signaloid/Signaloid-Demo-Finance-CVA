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
 *	cva-cva.c
 *
 *	Shared CVA support: the counterparty hazard-rate curve and the survival
 *	and default probabilities derived from it. These helpers are deterministic
 *	and are used by both the UxHw single-pass kernel (`cva-uxhw.c`) and the
 *	native Monte Carlo kernel (`cva-monte-carlo.c`).
 *
 *	The hazard rate curve is the backward-flat piecewise-constant curve
 *	from the notebook with pillars at integer years 0..10 and rates
 *	0, 0.02, 0.04, ..., 0.20.
 */

#include <math.h>
#include "cva-config.h"

/*
 *	Backward-flat hazard rates matching the notebook's
 *	`ql.HazardRateCurve(pd_dates, hzrates, ql.Actual365Fixed())`
 *	where hzrates[i] = 0.02 * i and pd_dates[i] = today + i years.
 *
 *	BackwardFlat interpretation: lambda(t) = kCvaHazardRates[i] for
 *	t in (i - 1, i], so the curve is integrated piecewise to obtain
 *	the cumulative hazard and survival probability.
 */
static const double kCvaHazardRates[kCvaNumHazardPillars] = {
	0.00, 0.02, 0.04, 0.06, 0.08, 0.10,
	0.12, 0.14, 0.16, 0.18, 0.20
};

double
cvaSurvivalProbability(double t)
{
	if (t <= 0.0)
	{
		return 1.0;
	}

	/*
	 *	Cumulative hazard H(t) = integral_0^t lambda(s) ds.
	 *
	 *	For t in (i - 1, i] with i = 1..10:
	 *		H(t) = sum_{k=1}^{i-1} lambda_k + lambda_i (t - (i - 1)).
	 *
	 *	Beyond the last pillar we extrapolate flat with lambda_10.
	 */
	double  cumulativeHazard    = 0.0;
	size_t  fullIntervals       = (size_t) floor(t);

	if (fullIntervals >= kCvaNumHazardPillars)
	{
		fullIntervals = kCvaNumHazardPillars - 1;
	}

	for (size_t ii = 1; ii <= fullIntervals; ii++)
	{
		cumulativeHazard += kCvaHazardRates[ii];
	}

	double  remainder   = t - (double) fullIntervals;
	size_t  nextIdx     = fullIntervals + 1;

	if (nextIdx >= kCvaNumHazardPillars)
	{
		nextIdx = kCvaNumHazardPillars - 1;
	}

	cumulativeHazard += kCvaHazardRates[nextIdx] * remainder;

	return exp(-cumulativeHazard);
}

double
cvaDefaultProbability(double t1, double t2)
{
	if (t2 <= t1)
	{
		return 0.0;
	}

	return cvaSurvivalProbability(t1) - cvaSurvivalProbability(t2);
}
