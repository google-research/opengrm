// List of common command line flags that multiple modules may need to include.

#include <fst/compat.h>
#include <thrax/compat/compat.h>

DEFINE_bool(save_symbols, false,
            "Keep symbol tables associated with generated fsts.");

DEFINE_string(indir, "", "The directory with the source files.");
DEFINE_string(outdir, "", "The directory in which we'll write the output.");
