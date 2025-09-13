
#include "Ravl2/Catch2checks.hh"

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
}