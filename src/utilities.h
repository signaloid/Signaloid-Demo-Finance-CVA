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

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include "common.h"
#include "cva-config.h"
#include "kernel.h"

typedef struct
{
	CommonCommandLineArguments  common;
	KernelParameters            kernelParameters;
} CommandLineArguments;

/**
 *	@brief	Print out command line usage.
 */
void
printUsage(void);

/**
 *	@brief	Set the default values for the command line arguments.
 *
 *	@param	arguments	: Pointer to struct to store arguments.
 *	@return			: `kCommonConstantReturnTypeSuccess` if successful, else `kCommonConstantReturnTypeError`.
 */
CommonConstantReturnType
setDefaultCommandLineArguments(CommandLineArguments * arguments);

/**
 *	@brief	Get command line arguments.
 *
 *		In the no-OS build there is no command line: `argc` and `argv` are
 *		ignored and the hard-coded configuration set by
 *		`setNoOSCommandLineArguments()` is used instead.
 *
 *	@param	argc		: argument count from `main()`.
 *	@param	argv		: argument vector from `main()`.
 *	@param	arguments	: Pointer to struct to store arguments.
 *	@return			: `kCommonConstantReturnTypeSuccess` if successful, else `kCommonConstantReturnTypeError`.
 */
CommonConstantReturnType
getCommandLineArguments(int argc, char *  argv[], CommandLineArguments *  arguments);

#ifdef NO_OS_AVAILABLE

/**
 *	@brief	Set the hard-coded command line arguments used by the no-OS build.
 *
 *		This is the single place to change the configuration that no-OS
 *		runs use, since those runs cannot be given command line arguments.
 *		Fields not set here keep the values from
 *		`setDefaultCommandLineArguments()`.
 *
 *	@param	arguments	: Pointer to struct to store arguments.
 *	@return			: `kCommonConstantReturnTypeSuccess` if successful, else `kCommonConstantReturnTypeError`.
 */
CommonConstantReturnType
setNoOSCommandLineArguments(CommandLineArguments * arguments);
#endif
