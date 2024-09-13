


include(GenerateExportHeader)

add_library (Ravl2Geometry
        Geometry.hh Geometry.cc
        Affine.cc Affine.hh
        ScaleTranslate.cc ScaleTranslate.hh
        Affine2f.cc Affine2f.hh
        Affine3d.cc Affine3d.hh
        PointSet.cc PointSet.hh
        Moments2.cc Moments2.hh
        CircleIter.cc CircleIter.hh
        Circle.cc Circle.hh
        LinePP.hh LinePP.cc
        LinePP2d.hh LinePP2d.cc
        LineABC2d.cc LineABC2d.hh
        Line2dIter.hh Line2dIter.cc
        Polygon.cc Polygon.hh
        PolygonRasterIter.cc PolygonRasterIter.hh
        PolyLine.hh PolyLine.cc
        Range.hh Range.cc
        Projection.cc Projection.hh
        Transform.cc Transform.hh
        FitLine.cc FitLine.hh
        PolyApprox.hh
        Conic2.cc Conic2.hh
        FitConic.hh
        Ellipse.cc Ellipse.hh
        VectorOffset.cc VectorOffset.hh
        FitVectorOffset.hh FitVectorOffset.cc
        Quaternion.cc Quaternion.hh
        Isometry3.cc Isometry3.hh
        LinePV.cc LinePV.hh
        PlaneABCD3d.cc PlaneABCD3d.hh
        PlanePVV3d.cc PlanePVV3d.hh
        FitProjection.hh
        FitProjection.cc
        FitAffine.cc
        FitAffine.hh

)

add_library(ravl2::Ravl2Geometry ALIAS Ravl2Geometry)

target_link_libraries(Ravl2Geometry PRIVATE RAVL2_options RAVL2_warnings
)

target_link_libraries(Ravl2Geometry PUBLIC Ravl2Core xtensor-blas Ravl2Math)

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


