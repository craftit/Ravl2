


include(GenerateExportHeader)

add_library (Ravl2Geometry
        Geometry.hh Geometry.cc
        Affine.cc Affine.hh
        ScaleTranslate.cc ScaleTranslate.hh
        PointSet.cc PointSet.hh
        Moments2.cc Moments2.hh
        CircleIter.cc CircleIter.hh
        Circle.cc Circle.hh
        LinePP.hh LinePP.cc
        Line2PP.hh Line2PP.cc
        Line2ABC.cc Line2ABC.hh
        Line2Iter.hh Line2Iter.cc
        Polygon.cc Polygon.hh
        PolygonRasterIter.cc PolygonRasterIter.hh
        PolyLine.hh PolyLine.cc
        Range.hh Range.cc
        Projection.cc Projection.hh
        Transform.cc Transform.hh
        Translate.cc Translate.hh
        FitLine.cc FitLine.hh
        Conic2.cc Conic2.hh
        FitConic.cc FitConic.hh
        Ellipse.cc Ellipse.hh
        VectorOffset.cc VectorOffset.hh
        FitVectorOffset.hh FitVectorOffset.cc
        Quaternion.cc Quaternion.hh
        Isometry3.cc Isometry3.hh
        LinePV.cc LinePV.hh
        Plane3ABCD.cc Plane3ABCD.hh
        Plane3PVV.cc Plane3PVV.hh
        FitProjection.hh FitProjection.cc
        FitAffine.cc FitAffine.hh
        FitSimilarity.cc FitSimilarity.hh
        PolyApprox.hh PolyApprox.cc
        Quad.cc Quad.hh
        NSphere.cc
        NSphere.hh
)

add_library(ravl2::Ravl2Geometry ALIAS Ravl2Geometry)

target_link_libraries(Ravl2Geometry PRIVATE RAVL2_options RAVL2_warnings
)

target_link_libraries(Ravl2Geometry PUBLIC Ravl2Core Ravl2Math)

target_include_directories(Ravl2Geometry ${WARNING_GUARD} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)

target_compile_features(Ravl2Geometry PUBLIC cxx_std_20)

set_target_properties(
        Ravl2Geometry
        PROPERTIES VERSION ${PROJECT_VERSION}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(Ravl2Geometry EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/ravl2/Ravl2Core_export.hpp)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(Ravl2Geometry PUBLIC SAMPLE_LIBRARY_STATIC_DEFINE)
endif()


