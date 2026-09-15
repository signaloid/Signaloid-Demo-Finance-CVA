# Input Files

## input.csv
This CSV file contains samples for the two inputs that the template C demo has
(see `src/main.c`). The C demo reads these samples to generate the two input
distributions when one provides the command-line argument `-i input.csv`.

**Important Note:** The headers in the CSV file (in this case `firstInputVariableName`
and`secondInputVariableName`) should match the `expectedInputHeaders`
specified in `src/main.c` (the order of headers is not important).
