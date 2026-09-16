[<img src="https://assets.signaloid.io/add-to-signaloid-cloud-logo-dark-latest.png#gh-dark-mode-only" alt="[Add to signaloid.io]" height="30">](https://signaloid.io/repositories?connect=https://github.com/signaloid/Signaloid-Demo-Finance-CVA#gh-dark-mode-only)
[<img src="https://assets.signaloid.io/add-to-signaloid-cloud-logo-light-latest.png#gh-light-mode-only" alt="[Add to signaloid.io]" height="30">](https://signaloid.io/repositories?connect=https://github.com/signaloid/Signaloid-Demo-Finance-CVA#gh-light-mode-only)

# Credit Valuation Adjustment for an Interest-Rate-Swap Netting Set

A financial application that computes the unilateral Credit Valuation Adjustment (CVA) of a two-trade interest-rate-swap portfolio under the Hull-White / GSR one-factor short-rate model.

## Overview

Credit Valuation Adjustment is the price of counterparty default risk. It is the amount by which the value of a derivatives portfolio must be reduced to account for the possibility that the counterparty defaults before the trades mature. Computing it requires simulating the future value of the portfolio across a grid of dates, taking the positive exposure at each date, and weighting it by the counterparty's discounted default probability. This application implements that calculation for a netting set of two vanilla interest-rate swaps, reproducing the reference QuantLib notebook of Groncki[^1].

Traditional Monte Carlo CVA simulations draw thousands of short-rate paths, reprice the portfolio along every path, and average the discounted positive exposure. When running on the Signaloid Cloud Compute Engine, the application instead computes on probability-distribution representations directly. The short-rate state and the resulting mark-to-market exposure are carried as distributions through a single pass over the time grid, and the CVA is recovered as the expectation of the accumulated discounted-exposure distribution. This yields a result equivalent to a large Monte Carlo ensemble in one execution.

The two approaches live in separate kernels selected at run time. One is a single distributional pass ([`cva-uxhw.c`](src/cva-uxhw.c)). The other is a native Monte Carlo sampler ([`cva-monte-carlo.c`](src/cva-monte-carlo.c)) that uses the GNU Scientific Library[^2] via the UxHw® compatibility layer. The native Monte Carlo kernel runs when Monte Carlo mode (`-M`) is selected. Otherwise the single-pass UxHw kernel runs.

### Modeling framework

**Portfolio.** Two vanilla interest-rate swaps against the same counterparty, both starting at $t = 0$:

| # | Tenor | Notional | Fixed rate | Type |
|---|-------|----------|------------|------|
| 1 | 5Y    | 1,000,000 | 3% | Payer |
| 2 | 4Y    | &nbsp;&nbsp;500,000 | 3% | Receiver |

**Short-rate model.** The interest-rate environment follows the Hull-White / GSR one-factor model with constant mean reversion $a = 0.02$, constant volatility $\sigma = 0.0075$, and a flat 3% initial curve. The auxiliary Ornstein-Uhlenbeck state $x(t)$ and the analytic zero-coupon bond are

```math
\begin{aligned}
dx &= -a x \, dt + \sigma \, dW, \qquad r(t) = x(t) + \alpha(t) \\
P(t, T) &= A(t, T) \exp\big(-B(t, T) \, r(t)\big) \\
B(t, T) &= \frac{1 - e^{-a(T - t)}}{a}
\end{aligned}
```

**Hazard curve.** A backward-flat counterparty hazard-rate curve with pillars $\lambda_i = 0.02 \cdot i$ at integer years, giving the survival probability $S(t) = \exp\big(-\int_0^t \lambda(s) \, ds\big)$.

**CVA.** With recovery $R = 0.4$ and discount factor $\mathrm{DF}(t) = e^{-r_0 t}$, the unilateral CVA is

```math
\mathrm{CVA} = (1 - R) \sum_{n} \mathbb{E}\big[\max\big(V(t_n, x_{t_n}), 0\big)\big] \cdot \mathrm{DF}(t_n) \cdot \big[S(t_{n-1}) - S(t_n)\big]
```

where $V(t, x)$ is the netting-set mark-to-market value at time $t$.

### Features

This application computes:
- **Credit Valuation Adjustment (`-S 0`)**: the unilateral CVA of the netting set, reported in currency units. The Hull-White volatility $\sigma$ is treated as the uncertain model input (default `Gauss(0.0075, 0.0005)`), and the CVA is reported as the expectation of the resulting exposure distribution — a single particle value.

## Getting started

### Cloning the Repository

The correct way to clone this repository to get the submodules is:
```sh
git clone --recursive git@github.com:signaloid/Signaloid-Demo-Finance-CVA.git
```

If you forgot to clone with `--recursive` and end up with empty submodule directories, you can remedy this with:
```sh
git submodule update --init
```

## Running on the Signaloid Cloud Developer Platform

To run this application on the [Signaloid Cloud Developer Platform](https://signaloid.io), you need a Signaloid account. You can sign up using [this link](https://get.signaloid.io).

Once you have an account, click the "add to signaloid.io" button at the top of this README to connect this repository to the platform and run the application. On the platform the application performs a single distributional pass, so you do not need to set `-M` greater than 1. The core's uncertainty tracking provides the converged CVA in one execution.


## Running locally

You can compile and run this application locally as a native Monte Carlo implementation using the GNU Scientific Library[^2] to generate samples for the volatility input distribution.

### Prerequisites

The native build needs GNU Make, a C compiler, and the GNU Scientific Library (GSL).

On macOS, install the Xcode Command Line Tools (which provide `make` and the C
compiler) and then install GSL with [Homebrew](https://brew.sh):
```bash
xcode-select --install
brew install gsl
```

On Linux (Debian/Ubuntu):
```bash
sudo apt-get install -y build-essential libgsl-dev
```

The top-level `Makefile` detects the GSL install location automatically, covering
MacPorts (`/opt/local`), Homebrew on Apple Silicon (`/opt/homebrew`) and Homebrew
on Intel (`/usr/local`), so no further configuration is needed on macOS.

### Compilation

From the repository root, build the native Monte Carlo binary with the provided Makefile:
```bash
make build-local            # produces ./demo-native-mc
```

### Execution

Run with Monte Carlo mode (required for local execution):
```bash
./demo-native-mc -M 1500 -S 0 -x 0.0075 -T
```

This runs 1,500 Monte Carlo iterations to compute the CVA with the Hull-White volatility fixed at $\sigma = 0.0075$ and prints the kernel timing. The converged CVA mean is written to `data.out`, where the first line contains the execution time in microseconds (μs) and the following line contains the output value.

View the result:
```bash
cat data.out
```

## Command-line options

### Common options

| Short | Long | Type | Default | Description |
|-------|------|------|---------|-------------|
| `-i` | `--input` | string | - | Path to input CSV file (read the volatility input from file) |
| `-o` | `--output` | string | - | Path to output CSV file |
| `-S` | `--select-output` | int | 0 | Select which output to compute (only `0`, the CVA, is defined) |
| `-M` | `--multiple-executions` | int | 1 | Number of Monte Carlo iterations (only needed for local execution) |
| `-T` | `--time` | flag | false | Timing mode: time and print the kernel execution |
| `-b` | `--benchmarking` | flag | false | Enable benchmarking output format |
| `-j` | `--json` | flag | false | Output results in JSON format |
| `-v` | `--verbose` | flag | false | Verbose mode (not used by this demo) |
| `-h` | `--help` | flag | false | Display the help message |

### Model parameters

| Short | Long | Type | Default | Description |
|-------|------|------|---------|-------------|
| `-x` | `--volatility` | double | `Gauss(0.0075, 0.0005)` | Hull-White short-rate volatility $\sigma$. When omitted, $\sigma$ is treated as an uncertain (Gaussian) input. When set, $\sigma$ is fixed to the given value. |

**Note**: When running on Signaloid's platform you do not need to set `-M` greater than 1. The platform's uncertainty tracking provides the converged CVA in a single execution.

## Output

### `-S 0`: Credit valuation adjustment

Computes the unilateral CVA of the two-swap netting set. The result is a single scalar in currency units (EUR), recovered as the expectation of the accumulated discounted positive-exposure distribution. With the default uncertain volatility input, the demo reports a CVA that matches the reference notebook's **586.18 EUR** to within that notebook's own Monte Carlo noise, whose per-1500-path standard error is about 18 EUR.

Human-readable output:
```
Credit Valuation Adjustment (currency units): 589.763667
```

The application also supports JSON output (`-j`) and CSV output (`-o`), and can read the volatility input from a CSV file (`-i inputs/input.csv`).


## Repository layout

```
src/
 ├── main.c              # CLI parsing and mode dispatch
 ├── kernel.c            # portfolio table, volatility resolution, output dispatch
 ├── cva-config.h        # model constants and declarations
 ├── cva-hw.c            # Hull-White zero-bond and OU helpers (shared)
 ├── cva-pricing.c       # vanilla IR-swap pricing (shared)
 ├── cva-cva.c           # hazard-rate curve, survival / default probability (shared)
 ├── cva-uxhw.c/.h       # UxHw single distributional-pass CVA kernel
 ├── cva-monte-carlo.c/.h # native Monte Carlo CVA kernel (fmax exposure)
 ├── utilities.c/.h      # CLI argument parsing
 ├── common.*            # CSV / JSON helpers (submodule symlink)
 └── uxhw.*              # GSL-backed compat layer for the native build (submodule symlink)

Makefile                 # build-local, run-local, scce-build, scce-run
signaloid.yaml           # SCCE configuration
notebook.nb              # original QuantLib notebook
inputs/input.csv         # example volatility input
```

## References

[^1]: Groncki, M. 2015. *CVA calculation with QuantLib and Python.* https://ipythonquant.wordpress.com/2015/05/30/cva-calculation-with-quantlib-and-python/

[^2]: [GNU Scientific Library](https://www.gnu.org/software/gsl/)

Additional background:
- Brigo, D. and Mercurio, F. 2006. *Interest Rate Models — Theory and Practice.* Springer, Chapter 3.
- Gregory, J. *The xVA Challenge.* Wiley, Chapter 14.
