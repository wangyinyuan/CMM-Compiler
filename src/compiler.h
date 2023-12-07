#ifndef CMM_COMPILER_H
#define CMM_COMPILER_H

#include <stdio.h>
#include <stdlib.h>

enum
{
    SUCCESS,
    FAILURE
};

enum output_type
{
    OUTPUT_TYPE_ASSEMBLY,
    OUTPUT_TYPE_OBJECT,
    OUTPUT_TYPE_EXECUTABLE
};

/**
 * @brief Represents the compile process information.
 *
 * This struct contains the necessary information for the compile process,
 * including the filename, output filename,
 * and output type.
 */
typedef struct compile_process_input_file
{
    FILE *file;
    const char *abs_path;
} cfile;

typedef struct compile_process
{
    cfile *input_file; ///< The input file.
    FILE *ofile;       ///< The output file.
    int output_type;   ///< The type of output for the compiler.
} compile_process;

int compile_file(const char *filename, const char *output_filename, int output_type);
compile_process *compile_process_create(const char *filename, const char *output_filename, int output_type);
#endif // CMM_COMPILER_H