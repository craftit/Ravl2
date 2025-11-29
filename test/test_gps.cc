//
// Created by charles galambos on 29/11/2025.
//

#include "Ravl2/Geometry/GPSCoordinate.hh"
#include "Ravl2/Catch2checks.hh"
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{
  TEST_CASE("GPSCoordinate")
  {
    {
      std::string strTestCoord = "52 39' 27.2531\",1 43' 4.5177\",24.7m";
      GPSCoordinate testCoord(strTestCoord);
      SPDLOG_INFO("DMS=[{}]",testCoord.textDMS());

      GPSCoordinate restoredCoord(testCoord.textDMS());
      SPDLOG_INFO("RestoredDMS=[{}]",restoredCoord.textDMS());

      SPDLOG_INFO("Raw:{}  Restored:{} ",testCoord.raw(),restoredCoord.raw());

      CHECK((testCoord.raw() - restoredCoord.raw()).norm() < 0.0001);

      Point<double,3> cartesianPlace = testCoord.cartesian();
      SPDLOG_INFO("Metric=[{}]",cartesianPlace);

      GPSCoordinate recoveredGps = GPSCoordinate::cartesian2GPS(cartesianPlace,1e-12);
      SPDLOG_INFO("Restored DMS=[{}]",recoveredGps.textDMS());
    }

    const unsigned ntestdata = 7;
    const GPSCoordinate testdata[ntestdata] = {
      GPSCoordinate(-26.2025543,28.032913,1730),
      GPSCoordinate( 120,60,-20 ),
      GPSCoordinate( -10,-10,0 ),
      GPSCoordinate( -10,15,10 ),
      GPSCoordinate( -80,34.3,123 ),
      GPSCoordinate( 170,170,100 ),
      GPSCoordinate(" 51.240322 N, 0.614352 W" )
    };

    for(unsigned i = 0;i < ntestdata;i++) {
      const GPSCoordinate &gps = testdata[i];

      // Check text conversion.
      std::string stdCoordDMS = gps.textDMS();
      SPDLOG_INFO("gps=[{}]",stdCoordDMS);

      GPSCoordinate restoredTextCoord(stdCoordDMS);

      SPDLOG_INFO("Raw:{}  Restored:{} ",gps.raw(),restoredTextCoord.raw());
      CHECK((gps.raw() - restoredTextCoord.raw()).norm() < 0.0001);

      // Check Cartesian conversion.
      Point<double,3> cart = gps.cartesian();
      GPSCoordinate recoveredCartGps = GPSCoordinate::cartesian2GPS(cart,1e-12);
      CHECK(euclidDistance(cart,recoveredCartGps.cartesian()) < 0.01);

      // Check differentials.
      Vector<double,3> dlat,dlong,dheight;
      gps.differential(dlat,dlong,dheight);

      SPDLOG_INFO("DiffLat=[{}]",dlat);
      SPDLOG_INFO("DiffLong=[{}]",dlong);
      SPDLOG_INFO("DiffHeight=[{}]",dheight);

      SPDLOG_INFO("lat.long=[{}]",dlat.dot(dlong));
      SPDLOG_INFO("lat.vert=[{}]",dlat.dot(dheight));
      SPDLOG_INFO("long.vert=[{}]",dlong.dot(dheight));
    }
  }
}