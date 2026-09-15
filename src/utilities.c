/*
 *	Copyright (c) 2024-2026, Signaloid.
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

#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <uxhw.h>
#include <assert.h>
#include "utilities.h"
#include "common.h"
#include "cva-config.h"

void
printUsage(void)
{
	fprintf(
		stderr,
		"CVA Monte Carlo demo: replicates the QuantLib notebook "
		"`CVA calculation with QuantLib and Python` for a portfolio "
		"of two vanilla interest rate swaps under the Hull-White / "
		"GSR short-rate model.\n"
	);
	fprintf(stderr, "\n");
	printCommonUsage();
	fprintf(
		stderr,
		"	[-x, --volatility <Hull-White volatility sigma: double (Default: Gauss(%lf, %lf))>]\n",
		kCvaDefaultVolatility,
		kCvaDefaultVolatilityStdDev
	);
	fprintf(stderr, "\n");

	return;
}

/**
 *	@brief	Set the default values for the command line arguments.
 */
static CommonConstantReturnType
setDefaultCommandLineArguments(CommandLineArguments * arguments)
{
	if (arguments == NULL)
	{
		fprintf(stderr, "Error: The provided pointer to arguments is NULL.\n");

		return kCommonConstantReturnTypeError;
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
	*arguments = (CommandLineArguments) {
		.common = (CommonCommandLineArguments) { 0 },
	};
#pragma GCC diagnostic pop

	for (size_t ii = 0; ii < kCvaInputIndexMax; ii++)
	{
		arguments->inputVariables[ii]       = 0.0;
		arguments->isInputVariableSet[ii]   = false;
	}

	return kCommonConstantReturnTypeSuccess;
}

CommonConstantReturnType
getCommandLineArguments(int argc, char *  argv[], CommandLineArguments *  arguments)
{
	const char * volatilityArg = NULL;

	if (arguments == NULL)
	{
		fprintf(stderr, "Error: The provided pointer to arguments is NULL.\n");

		return kCommonConstantReturnTypeError;
	}

	if (setDefaultCommandLineArguments(arguments) != kCommonConstantReturnTypeSuccess)
	{
		return kCommonConstantReturnTypeError;
	}

	DemoOption options[] = {
		{ .opt = "x", .optAlternative = "volatility", .hasArg = true, .foundArg = &volatilityArg, .foundOpt = NULL },
		{ 0 },
	};

	if (parseArgs(argc, argv, &arguments->common, options) != kCommonConstantReturnTypeSuccess)
	{
		fprintf(stderr, "Error: Parsing command line arguments failed.\n");
		printUsage();

		return kCommonConstantReturnTypeError;
	}

	if (arguments->common.isHelpEnabled)
	{
		printUsage();

		exit(EXIT_SUCCESS);
	}

	/*
	 *	If no output is selected, set `outputSelect` to `kCvaOutputIndexMax`.
	 *	This triggers the demo to compute all outputs.
	 */
	if (!arguments->common.isOutputSelected)
	{
		arguments->common.outputSelect = kCvaOutputIndexMax;
	}

	/*
	 *	When `outputSelect` is set to `kCvaOutputIndexMax`, we cannot be
	 *	in benchmarking mode or Monte Carlo mode.
	 */
	if (arguments->common.outputSelect == kCvaOutputIndexMax)
	{
		if ((arguments->common.isBenchmarkingMode) || (arguments->common.isMonteCarloMode))
		{
			fprintf(stderr, "Error: Please select a single output when in benchmarking mode or Monte Carlo mode.\n");

			return kCommonConstantReturnTypeError;
		}
	}
	else if (arguments->common.outputSelect > kCvaOutputIndexMax)
	{
		fprintf(stderr, "Error: Wrong output selection.\n");

		return kCommonConstantReturnTypeError;
	}

	if ((arguments->common.isMonteCarloMode) && (arguments->common.isInputFromFileEnabled))
	{
		fprintf(stderr, "Error: Monte Carlo mode does not support input from file.\n");

		return kCommonConstantReturnTypeError;
	}

	if (arguments->common.isVerbose)
	{
		fprintf(stderr, "Warning: Verbose mode not supported. Continuing in non-verbose mode.\n");
	}

	if (volatilityArg != NULL)
	{
		double volatility;

		if (parseDoubleChecked(volatilityArg, &volatility) != kCommonConstantReturnTypeSuccess)
		{
			fprintf(stderr, "Error: The volatility must be a real number.\n");
			printUsage();

			return kCommonConstantReturnTypeError;
		}

		if (volatility <= 0.0)
		{
			fprintf(stderr, "Error: The volatility must be positive.\n");
			printUsage();

			return kCommonConstantReturnTypeError;
		}

		arguments->inputVariables[kCvaInputIndexVolatility]     = volatility;
		arguments->isInputVariableSet[kCvaInputIndexVolatility] = true;
	}

	return kCommonConstantReturnTypeSuccess;
}
