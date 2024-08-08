#! /bin/bash

# Get the directory of the script
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# If an arg is provided, use it as the source directory
if [ $# -eq 1 ]; then
    SRC_DIR=$1
else
    SRC_DIR=$DIR/../src
fi

# For each argument passed to the script
for arg in "$@"; do
  # Check if the target is a text file
  if [ -f $arg ]; then
    # Format the file
    echo "Formatting $arg"
    #clang-format -i $arg
  else
    # Format all source files in the directory
    echo "Formatting director $arg"
    # find $SRC_DIR -type f \( -name "*.hh" -o -name "*.cc" \) -print0 | xargs -0 clang-format -i
  fi
done

