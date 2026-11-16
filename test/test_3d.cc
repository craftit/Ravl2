
#include "Ravl2/Catch2checks.hh"
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Ravl2/IO/Cereal.hh"
#include "Ravl2/3D/PinholeCamera0.hh"
#include "Ravl2/3D/PinholeCamera3.hh"
#include "Ravl2/3D/TriMesh.hh"
#include "Ravl2/3D/MeshShapes.hh"
#include "Ravl2/Geometry/Range.hh"
#include "Ravl2/Geometry/Polygon.hh"
#include "Ravl2/Image/DrawPolygon.hh"
#include "Ravl2/Array.hh"

namespace Ravl2
{
  namespace {
    // Test Camera
    template<typename CameraT>
    void testCamera(const CameraT &cam)
    {
      {
        Point<float, 2> pix = {200.0f, 500.0f};
        Point<float, 3> pnt3 = Ravl2::unproject(cam, pix, 1.5f);
        CHECK((euclidDistance(cam.origin(), pnt3) - 1.5f) < 0.001f);

        SPDLOG_INFO("Pnt2 {} -> Pnt3: {}", pix, pnt3);

        Point<float, 2> pixReprojected = project(cam, pnt3);

        Point<float, 2> pixTest = Point<float, 2>::Zero();
        CHECK(cam.projectCheck(pixTest, pnt3));
        CHECK(euclidDistance(pixTest, pix) < 0.003f);

        Point<float, 3> pnt3n = Ravl2::unproject(cam, pix, -1.5f);
        CHECK(!cam.projectCheck(pixTest, pnt3n));
        CHECK(euclidDistance(pixReprojected, pix) < 0.003f);
        CHECK((euclidDistance(cam.origin(), pnt3n) - 1.5f) < 0.001f);
      }

      auto camDir = cam.direction();
      camDir.normalize();
      auto cameraOrigin = cam.origin();

      // Do some tests with the camera
      constexpr float delta = 0.1f;
      constexpr int rangeSize = 10;
      for(int zi = -rangeSize; zi < rangeSize; zi++) {
        float z = static_cast<float>(zi) * delta;
        for(int yi = -rangeSize; yi < rangeSize; yi++) {
          float y = static_cast<float>(yi) * delta;
          for(int xi = -rangeSize; xi < rangeSize; xi++) {
            float x = static_cast<float>(xi) * delta;
            Point<float,3> pnt = {x,y,z};
            Point<float,2> pix = Point<float,2>::Zero();
            if(cam.projectCheck(pix,pnt)) {
              // Check we're in the frame
              if(cam.frame().contains(toIndex(pix))) {
                auto ray = projectRay(cam, pix);
                if(!isNearZero(ray.distance(pnt),1e-4f)) {
                  SPDLOG_INFO("Pnt: {}  Proj: {}  Dist: {}   ", pnt, pix, ray.distance(pnt));
                }
                CHECK(isNearZero(ray.distance(pnt),1e-4f));
                // Check we're in front of the camera
                CHECK(ray.ParClosest(pnt) > 0);

                float dist = euclidDistance(cam.origin(),pnt);
                Point<float,3> pnt3 = unproject(cam,pix,dist);
                if(!isNearZero(euclidDistance(pnt,pnt3),1e-4f)) {
                  SPDLOG_INFO("Pnt: {}  Origin:{}  Proj: {}  Dist: {}   At:{} ", pnt, cam.origin(), pix, dist, pnt3);
                }
                REQUIRE(isNearZero(euclidDistance(pnt,pnt3),1e-4f));

                // Check unprojectZ is sane
                auto unPnt3 = cam.unprojectZ(pix,2.5f);
                CHECK(projectRay(cam,pix).distance(unPnt3) < 1e-4f);
                // We should project to the same z along the camera direction
                CHECK(isNearZero((unPnt3 - cameraOrigin).dot(camDir) - 2.5f, 1e-4f));
              }
            }
          }
        }
      }

    }
  }

  TEST_CASE("PinholeCamera0")
  {

    SECTION("Cereal")
    {
      Matrix<float,3,3> rot  = Matrix<float,3,3>::Identity();
      Vector<float,3> trans =  {5,6,7};
      PinholeCamera0<float> cam(1.0f, 2.0f, 3.0f, 4.0f, rot, trans, IndexRange<2>({{0, 100},{0, 100}}));
      std::stringstream ss;
      {
        cereal::JSONOutputArchive oarchive(ss);
        oarchive(cam);
      }
      //SPDLOG_INFO("Cam {}", ss.str());
      {
        cereal::JSONInputArchive iarchive(ss);
        PinholeCamera0<float> cam2;
        iarchive(cam2);
        EXPECT_FLOAT_EQ(cam.cx(), cam2.cx());
        EXPECT_FLOAT_EQ(cam.cy(), cam2.cy());
        EXPECT_FLOAT_EQ(cam.fx(), cam2.fx());
        EXPECT_FLOAT_EQ(cam.fy(), cam2.fy());
        EXPECT_TRUE(cam.R().isApprox(cam2.R()));
        EXPECT_TRUE(cam.t().isApprox(cam2.t()));
      }
    }
#if 1
    SECTION("Construct Frame")
    {
      //! Construct a camera that fills the image at the given distance
      //    static PinholeCamera0 fromFrame(const IndexRange<2> &frame,
      //                                    float horizontalSize,
      //                                    float distance);
      IndexRange<2> frame = {{20, 89},{10, 99}};
      float distance = 1.5;
      auto cam = PinholeCamera0<float>::fromFrame(frame,0.2f,distance);
      Vector<float, 2> pix;
      Point<float,3> pnt1 = {-0.1f,-0.1f,distance};
      CHECK(cam.projectCheck(pix,pnt1));
      SPDLOG_INFO("Pix1 {}", pix);
      EXPECT_FLOAT_EQ(pix[1], float(frame.min(1)));

      Point<float,3> pnt2 = {0.1f,0.1f,distance};
      CHECK(cam.projectCheck(pix,pnt2));
      SPDLOG_INFO("Pix2 {}", pix);
      EXPECT_FLOAT_EQ(pix[1], float(frame.max(1)+1));

      {
        Point<float,2> pix2 = {50,50};
        auto unPnt3 = cam.unprojectZ(pix2,2.5f);
        CHECK(projectRay(cam,pix2).distance(unPnt3) < 1e-5f);
        CHECK(isNearZero((unPnt3[2] - cam.origin()[2]) - 2.5f, 1e-4f));
      }
      {
        Point<float,2> pix2 = toPoint<float>(toRange<float>(frame).center());
        auto unPnt3 = cam.unprojectZ(pix2,2.5f);
        SPDLOG_INFO(" UnPnt3: {} ", unPnt3);
        CHECK(isNearZero(unPnt3[0]));
        CHECK(isNearZero(unPnt3[1]));
        Point<float,2> pix2r;
        CHECK(cam.projectCheck(pix2r,unPnt3));
        CHECK(isNearZero(euclidDistance(pix2,pix2r),1e-4f));
      }

      testCamera(cam);
    }
#endif
#if 1
    SECTION("Construct Frame Origin")
    {
      IndexRange<2> frame = {{-50, 50},{-50, 50}};
      const float distance = 1.5;
      const float horizontalSize = 0.2f;
      auto cam = PinholeCamera0<float>::fromFrameOrigin(frame,horizontalSize,distance);
      Vector<float, 2> pix;
      cam.project(pix,toVector<float>(0,0,distance));
      EXPECT_FLOAT_EQ(pix[0], 0);
      EXPECT_FLOAT_EQ(pix[1], 0);
      cam.project(pix,toVector<float>(horizontalSize/2,0.0,distance));
      SPDLOG_INFO("Pix {}", pix);
      EXPECT_FLOAT_EQ(pix[1], float(frame.max(1)));
      cam.project(pix,toVector<float>(-horizontalSize/2,0.0,distance));
      SPDLOG_INFO("Pix {}", pix);
      EXPECT_FLOAT_EQ(pix[1], float(frame.min(1)));

      testCamera(cam);
    }
#endif
#if 1
    SECTION("Check Projective Matrix")
    {
      IndexRange<2> frame = {{0, 480},{0, 640}};
      float f = 1.5;

      auto cam = PinholeCamera0<float>(frame, f, Isometry3<float>::fromEulerXYZTranslation({0.0f,0.1f,0.05f},{0.0f,0.0f,0.0f}));
      //auto cam = PinholeCamera0<float>::fromFrame(frame,0.2f,distance);

      auto mat = cam.projectionMatrix<Ravl2::CameraCoordinateSystemT::Native>();
      SPDLOG_INFO("Mat: {}", mat);

      Vector<float, 3> pnt = {1,2,3};
      Vector<float, 3> pPix = mat * toHomogeneous(pnt);
      Vector<float, 2> pix;
      CHECK(cam.projectCheck(pix,pnt));

      SPDLOG_INFO("Pnt: {}  Proj: {}  Pix: {}  Dist:{} ", pnt, fromHomogeneous(pPix), pix, euclidDistance(fromHomogeneous(pPix),pix));

      CHECK(euclidDistance(fromHomogeneous(pPix),pix) < 0.001f);
      CHECK(cam.isInView(pnt));

      testCamera(cam);
    }
#endif

#if 1
    SECTION("Pinhole0 OpenCV")
    {
      IndexRange<2> frame = {{0, 480},{0, 640}};
      SPDLOG_INFO("Test Pinhole0 OpenCV");
      auto pose = Isometry3<float>::fromEulerXYZTranslation({0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f});
      auto cam = PinholeCamera0<float>::fromParameters<Ravl2::CameraCoordinateSystemT::OpenCV>(318.5f, 241.2f,
                                                                                                                     526.6f, 526.6f,
                                                                                                                     pose.rotation().toMatrix(),
                                                                                                                     pose.translation(),
                                                                                                                     frame);

      Vector<float, 2> pix1;
      CHECK(cam.projectCheck(pix1,toVector<float>(-0.5,-0.2,2.0)));
      Vector<float, 2> pix2;
      CHECK(cam.projectCheck(pix2,toVector<float>(0.5,0.2,2.0)));

      SPDLOG_INFO("Pix1 {}  Pix2 {} ", pix1,pix2);
      CHECK(pix1[1] < pix2[1]);
      CHECK(pix1[0] < pix2[0]);

      CHECK(cam.isInView(toPoint<float>(0,0,1.0)));

      testCamera(cam);
    }
#endif

#if 1
    SECTION("Pinhole3 OpenCV")
    {
      IndexRange<2> frame = {{0, 480},{0, 640}};

      SPDLOG_INFO("Test Pinhole3 OpenCV");
      auto pose = Isometry3<float>::fromEulerXYZTranslation({0.0f,0.1f,0.05f},{0.0f,0.0f,0.0f});
      auto cam = PinholeCamera3<float>::fromParameters<Ravl2::CameraCoordinateSystemT::OpenCV>(318.5f, 241.2f,
                                                                                                                     526.6f, 526.6f,
                                                                                                                     pose.rotation().toMatrix(),
                                                                                                                     pose.translation(),
                                                                                                                     frame,0.1f,0.2f);
      testCamera(cam);
    }
#endif


  }

  // Display the texture coordinates of a mesh.

  bool testTexCoords(const Ravl2::TriMesh<float> &mesh)
  {

    Ravl2::Array<uint8_t,2> img({512,512});
    fill(img,0);

    Range<float,2> texRect({0,0},{1,1});

    for(auto it : mesh.Faces()) {
      Polygon<float> poly;
      // Check texture coordinates are legal.
      if(!texRect.contains(it.TextureCoord(0)))
        return false;
      if(!texRect.contains(it.TextureCoord(1)))
        return false;
      if(!texRect.contains(it.TextureCoord(2)))
        return false;
      // Check area.
      poly.push_back(it.TextureCoord(0)*512);
      poly.push_back(it.TextureCoord(1)*512);
      poly.push_back(it.TextureCoord(2)*512);

      if(poly.area() == 0) {
        SPDLOG_WARN("Warning: Texture map for tri has zero area. ");
        return false;
      }

      // For visual check.
      Ravl2::DrawPolygon(img,uint8_t(255),poly);

    }
#if 0
    // Enable to display mapping for faces onto the texture.
    static int testCount =0;
    save(std::string("@X:Tex") + std::string(testCount++),img);
#endif
    return true;
  }



  TEST_CASE("TriMesh")
  {

    SECTION("MeshPlane")
    {
      TriMesh<float> mesh = createTriMeshPlane(1.0);

      auto ns = mesh.Vertices()[0].normal()[2];
      mesh.UpdateVertexNormals();
      // Check the normals are consistant
      CHECK(sign(ns) == sign(mesh.Vertices()[0].normal()[2]));
      CHECK(testTexCoords(mesh));

    }

    SECTION("MeshCub")
    {
      TriMesh<float> mesh = createTriMeshCube(1.0);
      CHECK(testTexCoords(mesh));
    }

    SECTION("Sphere")
    {
      TriMesh<float> mesh = createTriMeshSphere(3,8,1.0);
      CHECK(testTexCoords(mesh));
    }
  }


  //! The camera convention every constructor must share: image points are (row, col) and the
  //! 3D camera frame is OpenCV's (x right, y down, z forward). The images are non-square with an
  //! off-centre principal point, and the translation is non-zero, so a swapped axis, a crossed
  //! principal point or an unswapped translation each show up.
  TEST_CASE("PinholeCamera conventions")
  {
    const IndexRange<2> frame = {{0, 479}, {0, 639}};
    // OpenCV intrinsics: x = col, y = row.
    const float cxCol = 200.0f, cyRow = 100.0f, fxCol = 500.0f, fyRow = 400.0f;
    const Vector<float, 3> t = {0.1f, -0.2f, 0.3f};
    const Point<float, 3> world = {0.2f, 0.1f, 2.0f};
    const Vector<float, 3> camPt = world + t;// R = I
    const float expectRow = cyRow + fyRow * camPt[1] / camPt[2];
    const float expectCol = cxCol + fxCol * camPt[0] / camPt[2];

    SECTION("fromParameters OpenCV projects to (row, col) with an OpenCV-handed frame")
    {
      auto cam = PinholeCamera3<float>::fromParameters<CameraCoordinateSystemT::OpenCV>(
        cxCol, cyRow, fxCol, fyRow, Matrix<float, 3, 3>::Identity(), t, frame, 0.0f, 0.0f);
      CHECK(cam.hasCoordSwap());
      Vector<float, 2> pix;
      REQUIRE(cam.projectCheck(pix, world));
      CHECK_THAT(pix[0], Catch::Matchers::WithinAbs(expectRow, 1e-3));
      CHECK_THAT(pix[1], Catch::Matchers::WithinAbs(expectCol, 1e-3));
      CHECK(cam.isInView(world));

      const Point<float, 3> back = cam.unprojectZ(toPoint<float>(expectRow, expectCol), camPt[2]);
      CHECK(euclidDistance(back, world) < 1e-4f);

      // The translation is part of the extrinsic: origin and extrinsics are the external ones.
      CHECK(euclidDistance(cam.origin(), Point<float, 3>(-t)) < 1e-5f);
      CHECK(cam.extrinsicTranslation().isApprox(t));
      CHECK(cam.extrinsicRotation().isApprox(Matrix<float, 3, 3>::Identity()));

      // The OpenCV projection matrix gives (x, y) = (col, row); the Native one (row, col).
      const Vector<float, 3> xyw = cam.projectionMatrix<CameraCoordinateSystemT::OpenCV>() * toHomogeneous(world);
      CHECK_THAT(xyw[0] / xyw[2], Catch::Matchers::WithinAbs(expectCol, 1e-3));
      CHECK_THAT(xyw[1] / xyw[2], Catch::Matchers::WithinAbs(expectRow, 1e-3));
      const Vector<float, 3> rcw = cam.projectionMatrix<CameraCoordinateSystemT::Native>() * toHomogeneous(world);
      CHECK_THAT(rcw[0] / rcw[2], Catch::Matchers::WithinAbs(expectRow, 1e-3));
      CHECK_THAT(rcw[1] / rcw[2], Catch::Matchers::WithinAbs(expectCol, 1e-3));
    }

    SECTION("fromParameters Native takes (row, col)-ordered parameters")
    {
      auto cam = PinholeCamera0<float>::fromParameters<CameraCoordinateSystemT::Native>(
        cyRow, cxCol, fyRow, fxCol, Matrix<float, 3, 3>::Identity(), t, frame);
      Vector<float, 2> pix;
      REQUIRE(cam.projectCheck(pix, world));
      CHECK_THAT(pix[0], Catch::Matchers::WithinAbs(expectRow, 1e-3));
      CHECK_THAT(pix[1], Catch::Matchers::WithinAbs(expectCol, 1e-3));
    }

    SECTION("Configuration (default OpenCV) matches fromParameters")
    {
      Configuration config = Configuration::fromJSONString(
        R"({"cx": 200, "cy": 100, "fx": 500, "fy": 400, "t": [0.1, -0.2, 0.3], "frame": "0 479 0 639"})");
      PinholeCamera0<float> cam(config);
      Vector<float, 2> pix;
      REQUIRE(cam.projectCheck(pix, world));
      CHECK_THAT(pix[0], Catch::Matchers::WithinAbs(expectRow, 1e-3));
      CHECK_THAT(pix[1], Catch::Matchers::WithinAbs(expectCol, 1e-3));
    }

    SECTION("The frame constructor and fromFrame are OpenCV-handed and centred")
    {
      const IndexRange<2> wide = {{0, 99}, {0, 199}};// 100 rows x 200 cols
      const PinholeCamera0<float> camA(wide, 100.0f);
      const auto camB = PinholeCamera0<float>::fromFrame(wide, 1.0f, 1.0f);
      for(const PinholeCamera0<float> *cam : {&camA, &camB}) {
        Vector<float, 2> centre, right, down;
        REQUIRE(cam->projectCheck(centre, toPoint<float>(0.0f, 0.0f, 1.0f)));
        REQUIRE(cam->projectCheck(right, toPoint<float>(0.1f, 0.0f, 1.0f)));
        REQUIRE(cam->projectCheck(down, toPoint<float>(0.0f, 0.1f, 1.0f)));
        CHECK_THAT(centre[0], Catch::Matchers::WithinAbs(49.5, 0.6));
        CHECK_THAT(centre[1], Catch::Matchers::WithinAbs(99.5, 0.6));
        CHECK(right[1] > centre[1]);// +x is right: column increases
        CHECK_THAT(right[0], Catch::Matchers::WithinAbs(centre[0], 1e-4));
        CHECK(down[0] > centre[0]);// +y is down: row increases
        CHECK_THAT(down[1], Catch::Matchers::WithinAbs(centre[1], 1e-4));
      }
    }

    SECTION("Undistortion works across a non-square image")
    {
      // 100 rows x 1000 cols: a point near the right edge is far outside the ROW extent.
      const IndexRange<2> strip = {{0, 99}, {0, 999}};
      auto cam = PinholeCamera3<float>::fromParameters<CameraCoordinateSystemT::OpenCV>(
        500.0f, 50.0f, 500.0f, 500.0f, Matrix<float, 3, 3>::Identity(), Vector<float, 3>::Zero(), strip, 0.1f, 0.05f);
      const Point<float, 3> edge = {0.7f, 0.05f, 1.0f};// col ~871, far outside the 100-row extent
      Vector<float, 2> pix;
      REQUIRE(cam.projectCheck(pix, edge));
      REQUIRE(strip.contains(toIndex(pix)));
      CHECK(euclidDistance(cam.unprojectZ(pix, 1.0f), edge) < 1e-3f);
    }

    SECTION("A version-1 archive has its translation swapped on load")
    {
      const auto cam = PinholeCamera0<float>::fromParameters<CameraCoordinateSystemT::OpenCV>(
        cxCol, cyRow, fxCol, fyRow, Matrix<float, 3, 3>::Identity(), t, frame);
      // Version 1 stored the external translation unswapped.
      PinholeCamera0<float> v1 = cam;
      v1.t() = t;
      std::stringstream out;
      {
        cereal::JSONOutputArchive oarchive(out);
        oarchive(v1);
      }
      std::string text = out.str();
      const std::string v2Tag = "\"cereal_class_version\": 2";
      const auto at = text.find(v2Tag);
      REQUIRE(at != std::string::npos);
      text.replace(at, v2Tag.size(), "\"cereal_class_version\": 1");
      std::stringstream in(text);
      PinholeCamera0<float> loaded;
      {
        cereal::JSONInputArchive iarchive(in);
        iarchive(loaded);
      }
      CHECK(loaded.t().isApprox(cam.t()));
      CHECK(loaded.extrinsicTranslation().isApprox(t));
    }
  }
}
