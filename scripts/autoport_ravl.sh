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
#echo "$files"

# Create a temporary file for the sed script
sed_script_file=$(mktemp)

echo "Creating sed script at $sed_script_file"

# Write the sed script to the temporary file
cat <<'EOF' > "$sed_script_file"
s/IndexC/int/g
s/Index2dC/Index<2>/g
s/IndexRange2dC/IndexRange<2>/g
s/IndexRange1dC/IndexRange<1>/g
s/RealRange1dC/Range<1,float>/g
s/RealRange2dC/Range<2,float>/g
s/ImageC<\([^>]*>\)>/Array<\1,2>/g
s/SArray1dC<\([^>]*>\)>/std::vector<\1,1>/g
s/Array1dC<\([^>]*>\)>/Array<\1,1>/g
# Replace Frame().TRow() with .range().range(0).min()
s/\.TRow()/\.range(0).min()/g
s/\.BRow()/\.range(0).max()/g
s/\.LCol()/\.range(1).min()/g
s/\.RCol()/\.range(1).max()/g
s/\.Frame()/\.range()/g
s/\.Size()/\.size()/g
s/\.Row()/[0]/g
s/\.Col()/[1]/g
s/\#include "Ravl\//#include "Ravl2\//g
# Delete lines with "using namespace RavlN;" and "using namespace RavlImageN;" after any whitespace
/^\s*using namespace RavlN;/d
/^\s*using namespace RavlImageN;/d
s/namespace RavlN/namespace Ravl2/g
s/namespace RavlImageN/namespace Ravl2/g
EOF

# Apply the sed script to the files
echo "$files" | xargs sed -i -f "$sed_script_file"

# Clean up the temporary file
#rm "$sed_script_file"