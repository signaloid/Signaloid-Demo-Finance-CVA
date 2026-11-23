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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <uxhw.h>
#include "utilities.h"
#include "kernel.h"
#include "cva-config.h"

#ifdef NO_OS_AVAILABLE

void
returnZeroNoOS(void);
#endif

int
main(int argc, char *  argv[])
{
	CommandLineArguments    arguments               = (CommandLineArguments) { 0 };
	const char *            applicationDescription  =
		"Computes the Credit Valuation Adjustment (CVA) for a netting "
		"set of two vanilla interest rate swaps under the Hull-White / "
		"GSR short-rate model.";
	const char *    expectedInputHeaders[kCvaInputIndexMax] = {
		"volatility"
	};
	double          outputVariables[kCvaOutputIndexMax] = { 0 };
	const char *    outputVariableNames[kCvaOutputIndexMax] = {
		"CVA"
	};
	const char *    outputVariableDescriptions[kCvaOutputIndexMax] = {
		"Credit Valuation Adjustment (currency units)"
	};
	/*
	 *	The CVA is a scalar output (the expected discounted exposure), so the
	 *	Monte Carlo estimator is the mean of the per-path samples rather than a
	 *	sampled distribution. This mirrors the `OutputObject: Scalar` entry in
	 *	`signaloid.yaml`. The two must agree. Outputs are indexed explicitly so
	 *	that the mapping stays correct if one ever becomes a distribution.
	 */
	kOutputVariableTypeIndex    outputVariableTypes[kCvaOutputIndexMax] = {
		[kCvaOutputIndexCva] = kOutputVariableTypeScalar
	};
	double *                    monteCarloOutputSamples = NULL;
	clock_t                     start                   = 0;
	clock_t                     end                     = 0;
	double                      cpuTimeUsedInSeconds    = 0.0;

	/*
	 *	Get command line arguments.
	 */
	if (getCommandLineArguments(argc, argv, &arguments) != kCommonConstantReturnTypeSuccess)
	{
		return EXIT_FAILURE;
	}

	/*
	 *	The input variables hold the command-line values (the `-x` value is
	 *	0.0 and unused unless `isInputVariableSet` is true). Override them
	 *	from the input CSV if `-i` was supplied, before timing starts. The
	 *	kernels use these values via `cvaResolveVolatility`.
	 */
	if (arguments.common.isInputFromFileEnabled)
	{
		if (readInputDoubleDistributionsFromCSV(
				arguments.common.inputFilePath,
				expectedInputHeaders,
				arguments.kernelParameters.inputVariables,
				kCvaInputIndexMax
		))
		{
			fprintf(stderr, "Error: Could not read from input CSV file \"%s\".\n", arguments.common.inputFilePath);

			return EXIT_FAILURE;
		}
	}

	/*
	 *	Allocate `monteCarloOutputSamples`. Even in UxHw mode the kernel stores
	 *	its single distributional result at index 0, so at least one element is
	 *	always required.
	 */
	monteCarloOutputSamples = (double *) checkedMalloc(
		arguments.common.numberOfMonteCarloIterations * sizeof(double),
		__FILE__,
		__LINE__
	);

	/*
	 *	Start timing if timing is enabled.
	 */
	if (arguments.common.isTimingEnabled)
	{
		start = clock();
	}

	/*
	 *	Dispatch to the mode-specific kernel.
	 */
	if (arguments.common.isMonteCarloMode)
	{
		calculateOutputMonteCarlo(
			&arguments.kernelParameters,
			arguments.common.numberOfMonteCarloIterations,
			outputVariables,
			monteCarloOutputSamples
		);
	}
	else
	{
		calculateOutputUxHw(&arguments.kernelParameters, outputVariables, monteCarloOutputSamples);
	}

	if (arguments.common.isTimingEnabled)
	{
		end = clock();
		cpuTimeUsedInSeconds = ((double) (end - start)) / CLOCKS_PER_SEC;
	}

	const bool isScalarOutputSelected = isSelectedOutputScalar(
		&arguments.common,
		outputVariableTypes,
		kCvaOutputIndexMax
	);

	if (arguments.common.isOutputJSONMode)
	{
		printJSONFormattedOutput(
			&arguments.common,
			monteCarloOutputSamples,
			outputVariables,
			outputVariableNames,
			kCvaOutputIndexMax,
			applicationDescription,
			outputVariableTypes
		);
	}
	else
	{
		printHumanConsumableOutput(
			&arguments.common,
			kCvaOutputIndexMax,
			outputVariables,
			outputVariableNames,
			outputVariableDescriptions,
			monteCarloOutputSamples,
			outputVariableTypes
		);
	}

	if (arguments.common.isTimingEnabled)
	{
		printf("\nCPU time used: %" SignaloidParticleModifier "lf seconds\n", cpuTimeUsedInSeconds);
	}

	if (arguments.common.isWriteToFileEnabled)
	{
		if (writeOutputDoubleDistributionsToCSV(
				arguments.common.outputFilePath,
				outputVariables,
				outputVariableNames,
				kCvaOutputIndexMax
		))
		{
			fprintf(stderr, "Error: Could not write to output CSV file \"%s\".\n", arguments.common.outputFilePath);

			return EXIT_FAILURE;
		}
	}

	/*
	 *	Write the Monte Carlo samples to `data.out`.
	 */
	if (arguments.common.isMonteCarloMode)
	{
		size_t samplesToSave = isScalarOutputSelected
		            ? 1
		            : arguments.common.numberOfMonteCarloIterations;

		saveMonteCarloDoubleDataToDataDotOutFile(
			monteCarloOutputSamples,
			(uint64_t) (cpuTimeUsedInSeconds * 1000000),
			samplesToSave
		);
	}

	free(monteCarloOutputSamples);
	monteCarloOutputSamples = NULL;
#ifdef NO_OS_AVAILABLE
	returnZeroNoOS();
#else

	return EXIT_SUCCESS;

#endif
}
