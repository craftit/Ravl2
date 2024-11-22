
#include "checks.hh"

#include "Ravl2/IO/Cereal.hh"
#include "Ravl2/3D/PinholeCamera0.hh"
#include "Ravl2/3D/TriMesh.hh"
#include "Ravl2/3D/MeshShapes.hh"
#include "Ravl2/Geometry/Range.hh"
#include "Ravl2/Geometry/Polygon.hh"
#include "Ravl2/Image/DrawPolygon.hh"
#include "Ravl2/Array.hh"

TEST_CASE("PinholeCamera0")
{
  using namespace Ravl2;

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
  
  SECTION("Construct Frame")
  {
    //! Construct a camera that fills the image at the given distance
//    static PinholeCamera0 fromFrame(const IndexRange<2> &frame,
//                                    float horizontalSize,
//                                    float distance);
    IndexRange<2> frame = {{10, 99},{10, 99}};
    float distance = 1.5;
    auto cam = PinholeCamera0<float>::fromFrame(frame,0.2f,distance);
    Vector<float, 2> pix;
    cam.project(pix,toVector<float>(0.0,0.1,distance));
    SPDLOG_INFO("Pix {}", pix);
    EXPECT_FLOAT_EQ(pix[1], float(frame.max(1)));
    cam.project(pix,toVector<float>(0.0,-0.1,distance));
    SPDLOG_INFO("Pix {}", pix);
    EXPECT_FLOAT_EQ(pix[1], float(frame.min(1)));
  }
  
  SECTION("Construct Frame Origin")
  {
    IndexRange<2> frame = {{-50, 50},{-50, 50}};
    float distance = 1.5;
    auto cam = PinholeCamera0<float>::fromFrame(frame,0.2f,distance);
    Vector<float, 2> pix;
    cam.project(pix,toVector<float>(0,0,distance));
    EXPECT_FLOAT_EQ(pix[0], 0);
    EXPECT_FLOAT_EQ(pix[1], 0);
    cam.project(pix,toVector<float>(0.0,0.1,distance));
    SPDLOG_INFO("Pix {}", pix);
    EXPECT_FLOAT_EQ(pix[1], float(frame.max(1)));
    cam.project(pix,toVector<float>(0.0,-0.1,distance));
    SPDLOG_INFO("Pix {}", pix);
    EXPECT_FLOAT_EQ(pix[1], float(frame.min(1)));
  }
  
  
}

// Display the texture coordinates of a mesh.

bool testTexCoords(const Ravl2::TriMesh<float> &mesh)
{
  using namespace Ravl2;

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
  using namespace Ravl2;

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
