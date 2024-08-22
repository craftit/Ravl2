#! /bin/bash

# Get the directory of the script
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# If an arg is provided, use it as the source directory
if [ $# -eq 1 ]; then
    SRC_DIR=$1
else
    SRC_DIR=$DIR/../src
fi

# Create a temporary file for the sed script
sed_script_file=$(mktemp)

# Write the sed script to the temporary file
cat <<'EOF' > "$sed_script_file"

# Remove old Ravl includes
/include "Ravl\/RefCounter\.hh"/d
/include "Ravl\/Array2dIter\.hh"/d
/include "Ravl\/Array1dIter\.hh"/d
/include "Ravl\/SArray1dIter\.hh"/d
/include "Ravl\/SArray2dIter\.hh"/d
/include "Ravl\/Hash\.hh"/d
/include "Ravl\/DListExtra\.hh"/d
/include "Ravl\/DLIter\.hh"/d
/include "Ravl\/Stream\.hh"/d

s/include "Ravl\/IndexRange1d\.hh"/include "Ravl2\/Index.hh"/g
s/include "Ravl\/SArray1d\.hh"/include <vector>/g
s/include "Ravl2\/Tuple2\.hh"/include <tuple>/g
s/include "Ravl\/Tuple2\.hh"/include <tuple>/g
s/include "Ravl\/SysLog\.hh"/include <spdlog\/spdlog.h>/g

# Delete useless header info
/\/\/! rcsid=/d
/\/\/! lib=/d
/\/\/! file=/d
/\/\/! userlevel=/d

# Update documentation
s/\/\/!param:/\/\/! @param /g
s/\/\/!return:/\/\/! @return /g

# Update some container types
# Use std::vector< instead of DListC< by default
s/DListC/std::vector/g
s/Tuple2C/std::tuple/g

# Replace 'for(DLIterC<CrackC> edge(list);edge;edge++)' with 'for(auto &edge : list)'
s/for[ ]+(DLIterC<\([^>]*\)> \([^(]*\)(\([^;]*\));[^;]*;[^;^,]*++)/for(auto \&\2 : \3)/g
s/for[ ]+(SArray1dIterC<\([^>]*\)> \([^(]*\)(\([^;]*\));[^;^,]*;[^;]*++)/for(auto \&\2 : \3)/g

# Replace things like 'for(DLIterC<CrackC> edge(list);edge;edge++)' with 'for(auto &edge : list)

# If we see cerr << replace with std::cerr <<, provided it hasn't already been done
s/(cerr <</(std::cerr <</g

# Replace .Origin() with .min()
s/\.Origin()/\.min()/g

# Update indexing
s/IndexC/int/g
# Remove .V() calls entirely, they were used to convert IndexC to int
s/\.Max()\.V()/\.max()/g
s/\.Min()\.V()/\.min()/g

# Replace .Expand() with .expand()
s/\.Expand(/\.expand(/g
s/\.Shrink(/\.shrink(/g
s/\.Erode()/\.shrink(1)/g
s/\.Dilate()/\.expand(1)/g
s/Contains(/contains(/g
s/\.Next()/\.next()/g
s/\.Data1()/\.data<0>()/g
s/\.Data2()/\.data<1>()/g
s/\.Data3()/\.data<2>()/g

s/\.Range1()/\.range(0)/g
s/\.Range2()/\.range(1)/g
s/\.Max()/\.max()/g
s/\.Min()/\.min()/g
s/\.Frame()/\.range()/g

s/\.V()//g
s/Index2dC/Index<2>/g
s/IndexRange2dC/IndexRange<2>/g
s/ImageRectangleC/IndexRange<2>/g
s/IndexRangeC/IndexRange<1>/g
s/\.ClipBy(/\.clipBy(/g
s/Round(/int_round(/g

# Update Arrays
s/SArray1dC<\([^>]*\)>/std::vector<\1>/g
s/Array1dC<\([^>]*\)>/Array<\1,1>/g
s/Array2dC<\([^>]*\)>/Array<\1,2>/g
s/CollectionC/std::vector/g
s/TFVectorC/Vector/g
s/TFMatrixC/Matrix/g

# Image has .TRow() and .BRow() methods, replace with range().min(0) and range().max(0) where we're the variable is called images
s/image\.TRow()/image\.range().min(0)/g
s/image\.BRow()/image\.range().max(0)/g
s/image\.LCol()/image\.range().min(1)/g
s/image\.RCol()/image\.range().max(1)/g
# same for img
s/img\.TRow()/img\.range().min(0)/g
s/img\.BRow()/img\.range().max(0)/g
s/img\.LCol()/img\.range().min(1)/g
s/img\.RCol()/img\.range().max(1)/g

# .Rows() and .Cols()
s/\.Rows()/\.range(0)\.size()/g
s/\.Cols()/\.range(1)\.size()/g

# Replace Frame().TRow() with .range().range(0).min()
s/\.TRow()/\.min(0)/g
s/\.BRow()/\.max(0)/g
s/\.LCol()/\.min(1)/g
s/\.RCol()/\.max(1)/g
s/\.Frame()/\.range()/g
s/\.Rectangle()/\.range()/g
s/\.Size()/\.size()/g
s/\.Fill(/\.fill(/g
s/\.Apply(/\.apply(/g
s/\.Row()/[0]/g
s/\.Col()/[1]/g
s/\.Number()/\.count()/g
s/MeanVarianceC/MeanVariance/g
s/\.X()/[0]/g
s/\.Y()/[1]/g
s/[[:blank:]]Size()/ size()/g
s/\.TopLeft()/.min()/g
s/\.BottomRight()/.max()/g

# When we see pxl.UpN() replace with up(pxl)

s/IsEmpty()/empty()/g
s/\.IsElm()/\.valid()/g
s/\.Area()/\.area()/g
s/ Area()/ area()/g
s/:Area()/:area()/g
s/\.Involve(/\.involve(/g
s/\.IsOverlapping(/\.overlaps(/g
s/\.InsLast(/\.push_back(/g
s/\.Add(/\.add(/g
s/:Add(/:add(/g
s/ Add(/ add(/g
s/\.Index()/\.index()/g

# Update math
s/Matrix2dC/Matrix<RealT,2,2>/g
s/Matrix3dC/Matrix<RealT,3,3>/g
s/Point2dC/Point<RealT,2>/g
s/Point3dC/Point<RealT,3>/g
s/Vector2dC/Vector<RealT,2>/g
s/Vector3dC/Vector<RealT,3>/g
s/RealRange1dC/Range<float,1>/g
s/RealRange2dC/Range<float,2>/g
s/IndexRange2dSetC/IndexRangeSet<2>/g
s/Affine2dC/Affine<RealT,2>/g
s/Tuple2C</std::tuple</g
s/Tuple3C</std::tuple</g
s/ComplexC/std::complex<RealT>/g
# Replace Abs() with std::abs()
s/Abs(/std::abs(/g
s/ Sqrt(/ std::sqrt(/g
s/ Exp(/ std::exp(/g
s/ Sqr(/ sqr(/g
s/ Log(/ std::log(/g
s/ Cos(/ std::cos(/g
s/ Sin(/ std::sin(/g
s/ Pow(/ std::pow(/g
s/RavlConstN::pi/std::numbers::pi_v<RealT>/g
s/IsAlmostZero(/isNearZero(/g
s/IsSmall(/isNearZero(/g
s/CentroidX()/centroid<0>()/g
s/CentroidY()/centroid<1>()/g

# Do this later to avoid conflicts
s/VectorC/VectorT<RealT>/g
s/MatrixC/Tensor<RealT,2>/g

# Deal with images
s/ImageC<\([^>]*\)>/Array<\1,2>/g

# Header files
# Change Ravl/Index2d.hh to Ravl2/Index.hh
s/Ravl\/Index2d.hh/Ravl2\/Index.hh/g

# Replace 'Ravl/Image/Image.hh' with 'Ravl2/Array.hh'
s/Ravl\/Image\/Image.hh/Ravl2\/Array.hh/g

# Change all the #include "Ravl/..." to #include "Ravl2/..."
s/\#include "Ravl\//#include "Ravl2\//g

# Change ostream to std::ostream, if it hasn't already been done
s/ ostream / std::ostream /g
s/ istream / std::istream /g
s/(ostream /(std::ostream /g
s/(istream /(std::istream /g

# Indexing
s/[0][0]/(0,0)/g
s/[0][1]/(0,1)/g
s/[0][2]/(0,2)/g
s/[1][0]/(1,0)/g
s/[1][1]/(1,1)/g
s/[1][2]/(1,2)/g
s/[2][0]/(2,0)/g
s/[2][1]/(2,1)/g
s/[2][2]/(2,2)/g

# Namespaces
# Delete lines with "using namespace RavlN;" and "using namespace RavlImageN;" after any whitespace
/^\s*using namespace RavlN;/d
/^\s*using namespace RavlImageN;/d
s/namespace RavlN/namespace Ravl2/g
s/namespace RavlImageN/namespace Ravl2/g

# Convert SysLog to spdlog.
# In text RavlDebug(x), RavlWarning(x), RavlInfo(x), RavlError(x), -> SPDLOG_TRACE(x), SYSLOG_WARN(x), SPDLOG_INFO(x), SPDLOG_ERROR(x),
# and in the text part change %s %f %d %e %u to {} %0.2f to {:.2f}

/.*Ravl\(Debug\|Warning\|Info\|Error\).*/ {
  s/%s/{}/g
  s/%f/{}/g
  s/%d/{}/g
  s/%e/{}/g
  s/%u/{}/g
  s/%\([0-9]*\.[0-9]*\)f/{:\1f}/g
}

s/RavlDebug/SPDLOG_TRACE/g
s/RavlWarning/SYSLOG_WARN/g
s/RavlInfo/SPDLOG_INFO/g
s/RavlError/SPDLOG_ERROR/g

# Other types, do them last to avoid conflicts
s/UIntT/unsigned/g
s/SizeT/size_t/g

# Classes
s/StringC/std::string/g
s/ByteT/uint8_t/g
s/UByteT/uint8_t/g
s/FontC/BitmapFont/g


EOF

# For each argument passed to the script
for arg in "$@" ; do
  # Is the target a text file?
  if [ -f $arg ]; then
    # Format the file
    echo "Porting $arg"
    sed -i -f "$sed_script_file" $arg
  else
    # Format all source files in the directory
    # Find all the header and source files in the src directory
    #files=$(find $SRC_DIR -type f -name "*.hh" -o -name "*.cc")
    #echo "$files"
    echo "Porting directory $arg"
    find $SRC_DIR -type f \( -name "*.hh" -o -name "*.cc" \) -print0 | xargs -0 sed -i -f "$sed_script_file"
  fi
done

# Clean up the temporary file
rm "$sed_script_file"