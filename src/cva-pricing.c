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
 *	cva-pricing.c
 *
 *	Vanilla interest rate swap pricing under Hull-White.
 *
 *	The notebook creates each swap with
 *		ql.VanillaSwap(type, nominal, fixedSchedule, fixedRate,
 *		               fixedLegDC, floatSchedule, euribor6m,
 *		               spread = 0, floatLegDC)
 *	i.e. an annual Thirty360 fixed leg against semi-annual Actual/360
 *	float leg, with the same discount curve used to project the floating
 *	coupons (single-curve approximation).
 *
 *	The floating leg is priced period by period:
 *	  - Periods that have already paid are skipped.
 *	  - Forward-starting periods (current valuation date t earlier than the
 *	    reset date) contribute the standard telescoping difference of zero
 *	    bonds, N (P(t, t_reset_i) - P(t, t_pay_i)).
 *	  - The active period (t between reset and pay) contributes the locked
 *	    coupon L_i tau_i N P(t, t_pay_i), with L_i tau_i taken from the
 *	    initial forward curve P^M(t_reset_i, t_pay_i) = exp(-r0 tau_i).
 *
 *	Treating L_i as the deterministic forward from the initial curve
 *	rather than as the path-dependent fixing keeps the kernel cheap and
 *	reproduces the notebook's CVA to within Monte Carlo noise (the
 *	notebook's 586 EUR has a per-1500-path standard error of ~18 EUR).
 */

#include <math.h>
#include "cva-config.h"

/*
 *	Common swap pricing routine. Internally produces the payer value
 *	(receive float, pay fixed). The receiver value is its negation.
 */
static double
cvaSwapPayerValue(
	const CvaSwap * swap,
	double          meanReversion,
	double          volatility,
	double          t,
	double          x)
{
	if (t >= swap->endYears)
	{
		return 0.0;
	}

	/*
	 *	Float leg PV. Iterate each semi-annual period. Combine
	 *	forward-starting periods (telescoping) with the in-life
	 *	period (locked coupon under the initial flat forward curve).
	 */
	size_t  numFloatPeriods = (size_t) ((swap->endYears - swap->startYears) / swap->floatTau + 0.5);
	double  floatPv         = 0.0;

	for (size_t ii = 0; ii < numFloatPeriods; ii++)
	{
		double  tReset  = swap->startYears + (double) ii * swap->floatTau;
		double  tPay    = swap->startYears + (double) (ii + 1) * swap->floatTau;

		if (tPay <= t)
		{
			continue;
		}

		double pPay = cvaHwZeroBond(meanReversion, volatility, t, tPay, x);

		if (t < tReset)
		{
			/*
			 *	Forward-starting: still resets in the future. Contribution
			 *	is the telescoping diff of two zero bonds at t.
			 */
			double pReset = cvaHwZeroBond(meanReversion, volatility, t, tReset, x);
			floatPv += swap->notional * (pReset - pPay);
		}
		else
		{
			/*
			 *	Active period: rate L_i was fixed at t_reset_i. We use
			 *	the initial-curve forward L_i tau_i = exp(r0 tau_i) - 1.
			 */
			double lTau = exp(kCvaFlatRate * swap->floatTau) - 1.0;
			floatPv += lTau * swap->notional * pPay;
		}
	}

	/*
	 *	Fixed leg PV. Coupon dates run annually from t_start + tau_fix
	 *	to t_end inclusive.
	 */
	size_t  numFixedPeriods = (size_t) ((swap->endYears - swap->startYears) / swap->fixedTau + 0.5);
	double  fixedPv         = 0.0;

	for (size_t ii = 0; ii < numFixedPeriods; ii++)
	{
		double tj = swap->startYears + (double) (ii + 1) * swap->fixedTau;

		if (tj <= t)
		{
			continue;
		}

		double pj = cvaHwZeroBond(meanReversion, volatility, t, tj, x);
		fixedPv += swap->fixedTau * pj;
	}

	fixedPv *= swap->fixedRate * swap->notional;

	return floatPv - fixedPv;
}

double
cvaSwapValue(
	const CvaSwap * swap,
	double          meanReversion,
	double          volatility,
	double          t,
	double          x)
{
	double payerValue = cvaSwapPayerValue(swap, meanReversion, volatility, t, x);

	if (swap->type == kCvaSwapTypeReceiver)
	{
		return -payerValue;
	}

	return payerValue;
}

double
cvaPortfolioValue(
	const CvaSwap * swaps,
	size_t          numSwaps,
	double          meanReversion,
	double          volatility,
	double          t,
	double          x)
{
	double total = 0.0;

	for (size_t ii = 0; ii < numSwaps; ii++)
	{
		total += cvaSwapValue(&swaps[ii], meanReversion, volatility, t, x);
	}

	return total;
}
