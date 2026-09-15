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
 *	cva-hw.c
 *
 *	Hull-White / GSR one-factor short-rate model.
 *
 *	The notebook builds the IR model with
 *		ql.Gsr(t0_curve, [today+100], [0.0075], [0.02], 16.0)
 *	which, because the volatility and mean reversion are each given a
 *	single value, reduces to the standard Hull-White model with constant
 *	parameters a = 0.02 and sigma = 0.0075, calibrated against the flat
 *	3% initial yield curve.
 *
 *	The implementation here uses the standard Brigo-Mercurio (2006)
 *	closed-form expressions for a flat initial forward curve. The
 *	auxiliary state x(t) (a mean-reverting Ornstein-Uhlenbeck process
 *	with zero drift to zero) is what we simulate. The short rate is
 *	r(t) = x(t) + alpha(t), and the zero-coupon bond P(t, T) is given
 *	by the affine formula P(t, T) = A(t, T) * exp(-B(t, T) * r(t)).
 */

#include <math.h>
#include "cva-config.h"

/*
 *	B(t, T) = (1 - exp(-a (T - t))) / a, evaluated as -expm1(-a (T - t)) / a
 *	to avoid catastrophic cancellation when a (T - t) is tiny (where
 *	exp(-a (T - t)) is numerically indistinguishable from 1.0).
 *	For a == 0 the limit is just T - t.
 */
double
cvaHwB(double meanReversion, double t, double T)
{
	double dt = T - t;

	if (dt <= 0.0)
	{
		return 0.0;
	}

	if (meanReversion == 0.0)
	{
		return dt;
	}

	return -expm1(-meanReversion * dt) / meanReversion;
}

/*
 *	alpha(t) = f^M(0, t) + sigma^2 / (2 a^2) * (1 - exp(-a t))^2
 *
 *	For a flat instantaneous forward rate f^M(0, t) = kCvaFlatRate this
 *	is the constant kCvaFlatRate plus the standard Hull-White convexity
 *	term. See Brigo-Mercurio (2006), eq. (3.37).
 */
double
cvaHwAlpha(double meanReversion, double volatility, double t)
{
	if (meanReversion == 0.0)
	{
		return kCvaFlatRate;
	}

	double  oneMinusExpAt   = 1.0 - exp(-meanReversion * t);
	double  convexity       =
		(volatility * volatility) /
		(2.0 * meanReversion * meanReversion) *
		oneMinusExpAt * oneMinusExpAt;

	return kCvaFlatRate + convexity;
}

double
cvaHwStateStddev(double meanReversion, double volatility, double t)
{
	if (t <= 0.0)
	{
		return 0.0;
	}

	if (meanReversion == 0.0)
	{
		return volatility * sqrt(t);
	}

	double variance =
		(volatility * volatility) *
		(1.0 - exp(-2.0 * meanReversion * t)) /
		(2.0 * meanReversion);

	return sqrt(variance);
}


double
cvaHwConditionalStddev(double meanReversion, double volatility, double dt)
{
	if (dt <= 0.0)
	{
		return 0.0;
	}

	if (meanReversion == 0.0)
	{
		return volatility * sqrt(dt);
	}

	double variance =
		(volatility * volatility) *
		(1.0 - exp(-2.0 * meanReversion * dt)) /
		(2.0 * meanReversion);

	return sqrt(variance);
}

/*
 *	P(t, T) = A(t, T) * exp(-B(t, T) * r(t))
 *
 *	For a flat initial forward curve f^M(0, t) = kCvaFlatRate:
 *
 *	A(t, T) = (P^M(0, T) / P^M(0, t)) * exp(B(t, T) * f^M(0, t)
 *	          - sigma^2 / (4 a) * (1 - exp(-2 a t)) * B(t, T)^2)
 *
 *	        = exp(-r0 * (T - t)) * exp(B(t, T) * r0
 *	          - sigma^2 / (4 a) * (1 - exp(-2 a t)) * B(t, T)^2)
 *
 *	r(t) = x(t) + alpha(t), and we receive x(t) as input.
 *	See Brigo-Mercurio (2006), eq. (3.39).
 */
double
cvaHwZeroBond(
	double  meanReversion,
	double  volatility,
	double  t,
	double  T,
	double  x)
{
	if (T <= t)
	{
		return 1.0;
	}

	double  bb      = cvaHwB(meanReversion, t, T);
	double  alpha   = cvaHwAlpha(meanReversion, volatility, t);
	double  rt      = x + alpha;

	double  logRatio        = -kCvaFlatRate * (T - t);
	double  forwardTerm     = bb * kCvaFlatRate;
	double  convexityTerm   = 0.0;

	if (meanReversion != 0.0)
	{
		double oneMinusExp2At = 1.0 - exp(-2.0 * meanReversion * t);
		convexityTerm =
			(volatility * volatility) / (4.0 * meanReversion) *
			oneMinusExp2At * bb * bb;
	}
	else
	{
		convexityTerm = (volatility * volatility) * t * bb * bb;
	}

	double logA = logRatio + forwardTerm - convexityTerm;

	return exp(logA - bb * rt);
}
