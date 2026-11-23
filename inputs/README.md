# Input Files

## `input.csv`
This CSV file holds the one input of the demo, the Hull-White volatility. Its header is `volatility` and its value is `0.0075`.

Pass the file with `-i inputs/input.csv`. The header must match `expectedInputHeaders` in `src/main.c`.

## Options
- `-x` or `--volatility` sets a fixed volatility. It must be a positive number.
- Without `-x` or `-i`, the volatility is `Gauss(0.0075, 0.0005)`.
- Monte Carlo mode (`-M`) does not support `-i`. Use `-x` instead.
