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
 *	cva-config.h
 *
 *	Configuration and types for the CVA Monte Carlo demonstration.
 *
 *	The demo reproduces the CVA calculation from the QuantLib notebook
 *	"CVA calculation with QuantLib and Python" (Matthias Groncki, 2015).
 *
 *	A portfolio of two vanilla interest rate swaps is exposed to a single
 *	counterparty whose hazard rate curve grows linearly with maturity.
 *	The interest rate environment is described by the Hull-White / GSR
 *	one-factor short-rate model with constant mean reversion and constant
 *	volatility. Each Monte Carlo iteration simulates one short-rate path,
 *	reprices the portfolio at every point on the time grid, and
 *	accumulates the discounted positive exposure into a per-path CVA.
 *
 *	Reference Notebook: notebook.nb (CVA = 586.18 EUR for 1500 paths)
 *	Reference Textbook: Brigo & Mercurio (2006), Chapter 3.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
 *	Market data: flat instantaneous forward rate at 3% (Actual/365), matching
 *	the notebook's `ql.FlatForward(today, 0.03, Actual365Fixed())`.
 */
#define kCvaFlatRate (0.03)

/*
 *	Hull-White / GSR parameters: constant mean reversion and constant
 *	volatility, matching the notebook's
 *	`ql.Gsr(t0_curve, [today+100], [0.0075], [0.02], 16.0)`.
 */
#define kCvaMeanReversion       (0.02)
#define kCvaDefaultVolatility   (0.0075)

/*
 *	Volatility uncertainty used in Signaloid (UxHw) mode and when
 *	sampling volatility per Monte Carlo iteration in native mode.
 *	Choose a small uncertainty so the mean CVA stays close to the
 *	notebook's deterministic CVA at vol = 0.0075.
 */
#define kCvaDefaultVolatilityStdDev (0.0005)

/*
 *	Counterparty recovery rate. Matches the notebook's `recovery = 0.4`.
 */
#define kCvaRecoveryRate (0.40)

/*
 *	Time grid. The notebook uses
 *		[today + ql.Period(i, ql.Months) for i in range(0, 12 * 6)]
 *	augmented with the swaps' fixing dates, which gives a non-uniform grid
 *	of 78 unique points covering six years. A uniform partition of the
 *	same span and cardinality is a faithful approximation and keeps the
 *	per-iteration work small.
 */
#define kCvaTimeGridYears   (6.0)
#define kCvaNumTimeSteps    (78)

/*
 *	Hazard rate curve. The notebook uses a backward-flat hazard-rate curve
 *	with pillars at integer years 0..10 and rates 0.02 * i.
 *	BackwardFlat interpretation: lambda(t) = hazardPillarRates[i] for
 *	t in (i - 1, i].
 */
#define kCvaNumHazardPillars (11)

/*
 *	The portfolio: two vanilla interest rate swaps against a single
 *	counterparty. Both start two business days after the valuation date
 *	(approximated as t = 0 here).
 */
typedef enum
{
	kCvaSwapTypePayer       = 0,
	kCvaSwapTypeReceiver    = 1
} CvaSwapType;

typedef struct
{
	double      startYears;     /*	Effective date (years from t=0) */
	double      endYears;       /*	Termination date (years from t=0) */
	double      notional;       /*	Nominal in currency units */
	double      fixedRate;      /*	Annual fixed coupon (decimal) */
	double      fixedTau;       /*	Fixed leg accrual fraction (years) */
	double      floatTau;       /*	Floating leg accrual fraction (years) */
	CvaSwapType type;           /*	Payer or receiver */
} CvaSwap;

#define kCvaNumPortfolioSwaps (2)

/*
 *	Output selection. The notebook reports CVA = 586.18 EUR as the
 *	headline number. That is the single output of this demo.
 */
typedef enum
{
	kCvaOutputIndexCva = 0,
	kCvaOutputIndexMax
} CvaOutputIndex;

/*
 *	Input distribution(s) read from the input CSV file or set from CLI.
 */
typedef enum
{
	kCvaInputIndexVolatility = 0,
	kCvaInputIndexMax
} CvaInputIndex;

/* ========================================================================
 *	Hull-White / GSR
 * ======================================================================== */

/**
 *	@brief	Hull-White B(t, T) factor, B(t, T) = (1 - exp(-a (T - t))) / a.
 */
double
cvaHwB(double meanReversion, double t, double T);

/**
 *	@brief	Hull-White mean-reversion drift for the OU state x(t).
 *
 *	@return	alpha(t) = f^M(0, t) + sigma^2 / (2 a^2) * (1 - exp(-a t))^2,
 *		evaluated for the flat instantaneous forward rate kCvaFlatRate.
 */
double
cvaHwAlpha(double meanReversion, double volatility, double t);

/**
 *	@brief	Hull-White zero-coupon bond price P(t, T) given the auxiliary
 *		OU state x(t).
 *
 *	@details Uses the closed form for a flat initial forward rate of
 *		kCvaFlatRate. r(t) = x(t) + alpha(t), and
 *		P(t, T) = A(t, T) * exp(-B(t, T) r(t)).
 */
double
cvaHwZeroBond(
	double  meanReversion,
	double  volatility,
	double  t,
	double  T,
	double  x);

/**
 *	@brief	Standard deviation of the OU state x(t) starting from x(0) = 0.
 *
 *	@return	sqrt(sigma^2 (1 - exp(-2 a t)) / (2 a)).
 */
double
cvaHwStateStddev(double meanReversion, double volatility, double t);

/**
 *	@brief	Conditional standard deviation of x(t + dt) given x(t).
 */
double
cvaHwConditionalStddev(double meanReversion, double volatility, double dt);

/* ========================================================================
 *	Swap pricing
 * ======================================================================== */

/**
 *	@brief	Value of a single vanilla interest rate swap at time t given
 *		the auxiliary OU state x(t).
 *
 *	@details Float leg value approximated as N * (P(t, t_anchor) - P(t, t_end))
 *		where t_anchor = max(t, t_start). Fixed leg value computed as
 *		sum over the remaining annual fixed coupons.
 *
 *	@return	Mark-to-market value of the swap at time t.
 */
double
cvaSwapValue(
	const CvaSwap * swap,
	double          meanReversion,
	double          volatility,
	double          t,
	double          x);

/**
 *	@brief	Value of the whole portfolio at time t given the OU state x(t).
 */
double
cvaPortfolioValue(
	const CvaSwap * swaps,
	size_t          numSwaps,
	double          meanReversion,
	double          volatility,
	double          t,
	double          x);

/* ========================================================================
 *	Hazard rate / default probabilities
 * ======================================================================== */

/**
 *	@brief	Survival probability up to time t for the backward-flat hazard
 *		rate curve used by the notebook.
 *
 *	@return	exp(- integral_0^t lambda(s) ds).
 */
double
cvaSurvivalProbability(double t);

/**
 *	@brief	Default probability in the interval (t1, t2].
 */
double
cvaDefaultProbability(double t1, double t2);

/*
 *	The per-path CVA accumulation has two implementations selected at
 *	run time: the UxHw single distributional pass (`cva-uxhw.c`,
 *	`cvaCalculatePathCvaUxHw`) and the native Monte Carlo sampler
 *	(`cva-monte-carlo.c`, `cvaCalculatePathCvaMonteCarlo`).
 */
