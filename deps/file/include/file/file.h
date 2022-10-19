#pragma once

#include <stdbool.h>
#include <str/str.h>
#include <sum/sum.h>

typedef SUM_RESULT_TYPE(str, str) SlurpFileResult;

SlurpFileResult slurp_file(str filename);

bool file_exists(str filename);
bool file_isdir(str filename);

typedef SUM_MAYBE_TYPE(str) FileMkdirRecError;

FileMkdirRecError file_mkdir_rec(str path);
