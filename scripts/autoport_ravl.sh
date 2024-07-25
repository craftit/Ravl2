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

# Update indexing
s/IndexC/int/g
# Remove .V() calls entirely, they were used to convert IndexC to int
/\.V()/d
s/Index2dC/Index<2>/g
s/IndexRange2dC/IndexRange<2>/g
s/IndexRange1dC/IndexRange<1>/g

# Update Arrays
s/SArray1dC<\([^>]*\)>/std::vector<\1>/g
s/Array1dC<\([^>]*\)>/Array<\1,1>/g
# Replace Frame().TRow() with .range().range(0).min()
s/\.TRow()/\.min(0)/g
s/\.BRow()/\.max(0)/g
s/\.LCol()/\.min(1)/g
s/\.RCol()/\.max(1)/g
s/\.Frame()/\.range()/g
s/\.Size()/\.size()/g
s/\.Row()/[0]/g
s/\.Col()/[1]/g

# Update math
s/Matrix2dC/Matrix<RealT,2,2>/g
s/Matrix3dC/Matrix<RealT,3,3>/g
s/Point2dC/Point<RealT,2>/g
s/Point3dC/Point<RealT,3>/g
s/RealRange1dC/Range<1,float>/g
s/RealRange2dC/Range<2,float>/g

# Deal with images
s/ImageC<\([^>]*\)>/Array<\1,2>/g

# Header files
# Change Ravl/Index2d.hh to Ravl2/Index.hh
s/Ravl\/Index2d.hh/Ravl2\/Index.hh/g

# Replace 'Ravl/Image/Image.hh' with 'Ravl2/Array.hh'
s/Ravl\/Image\/Image.hh/Ravl2\/Array.hh/g

# Change all the #include "Ravl/..." to #include "Ravl2/..."
s/\#include "Ravl\//#include "Ravl2\//g

# Namespaces
# Delete lines with "using namespace RavlN;" and "using namespace RavlImageN;" after any whitespace
/^\s*using namespace RavlN;/d
/^\s*using namespace RavlImageN;/d
s/namespace RavlN/namespace Ravl2/g
s/namespace RavlImageN/namespace Ravl2/g
s/UIntT/unsigned/g
EOF

# Apply the sed script to the files
echo "$files" | xargs sed -i -f "$sed_script_file"

# Clean up the temporary file
#rm "$sed_script_file"