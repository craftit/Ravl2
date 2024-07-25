#! /bin/bash

# Get the directory of the script
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# If an arg is provided, use it as the source directory
if [ $# -eq 1 ]; then
    SRC_DIR=$1
else
    SRC_DIR=$DIR/../src
fi

# Find all the header and source files in the src directory
files=$(find $SRC_DIR -type f -name "*.hh" -o -name "*.cc")

# Create a sed script with all the substitution commands
sed_script=$(cat <<'EOF'
s/IndexC/int/g
s/Index2dC/Index<2>/g
s/IndexRange2dC/IndexRange<2>/g
s/IndexRange1dC/IndexRange<1>/g
s/RealRange1dC/Range<1,float>/g
s/RealRange2dC/Range<2,float>/g
s/ImageC<\([^>]*>\)>/Array<\1,2>/g
s/SArray1dC<\([^>]*>\)>/std::vector<\1,1>/g
s/Array1dC<\([^>]*>\)>/Array<\1,1>/g
# replace function calls .Frame() with .range()
s/.Frame(/.range(/g
# replace function calls to Size() with size()
s/.Size(/.size(/g
# replace function calls to .Row() with [0]
s/.Row()/[0]/g
# replace function calls to .Col() with [1]
s/.Col()/[1]/g
# Replace includes lie  '#include "Ravl/Array.hh"' with '#include "Ravl2/Array.hh"'
s/#include "Ravl\//#include "Ravl2\//g
EOF
)

echo "$files" | xargs sed -i -f <(echo "$sed_script")
