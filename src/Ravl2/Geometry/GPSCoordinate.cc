// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2010, OmniPerception Ltd
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
// Modified for Ravl2 29/11/2025 by Charles Galambos

#include "Ravl2/Geometry/GPSCoordinate.hh"

#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include "Ravl2/StringUtils.hh"

namespace Ravl2
{

  GPSCoordinate::GPSCoordinate(const std::string &text) {
    text2GPS(text,*this);
  }

  //! Convert a cartesian position to gps.
  
  GPSCoordinate GPSCoordinate::cartesian2GPS(const Point<RealT,3> &position,RealT precision)
  {
    const RealT x = position[0];
    const RealT y = position[1];
    const RealT z = position[2];
    
    RealT lambda = std::atan2(y,x);
    RealT p = std::sqrt(sqr(x) + sqr(y));
    
    RealT theta = atan(z/(p * (1.0-m_e2)));
    unsigned iterLimit = 10;
    
    while(iterLimit-- > 0) {
      //std::cout << "Theta=" << (RavlConstN::rad2deg * theta) << "\n";
      
      RealT sinTheta = std::sin(theta);
      RealT v = m_a / std::sqrt(1-m_e2*sqr(sinTheta));
      
      RealT oldTheta = theta;
      
      theta = std::atan((z+m_e2*v*sinTheta)/p);
      
      if(std::abs(oldTheta - theta) < precision)
        break;
      
    }
    
    RealT sinTheta = std::sin(theta);
    RealT v = m_a / std::sqrt(1-m_e2*sqr(sinTheta));
    
    RealT height = (p / std::cos(theta)) - v;
    
    return GPSCoordinate(rad2deg(theta),rad2deg(lambda),height);
  }
  
  bool GPSCoordinate::usesNS(const std::string &coord)
  { return coord.find('N') != std::string::npos ||
           coord.find('S') != std::string::npos ||
           coord.find('n') != std::string::npos ||
           coord.find('s') != std::string::npos;
  }
  
  bool GPSCoordinate::usesEW(const std::string &coord)
  {
    return coord.find('E') != std::string::npos ||
           coord.find('W') != std::string::npos ||
           coord.find('e') != std::string::npos ||
           coord.find('w') != std::string::npos;
  }
  
  // Convert a gps coordinate from text form to a GPS class.
  // Returns false if text is not correctly formatted
  
  bool GPSCoordinate::text2GPS(const std::string &text,GPSCoordinate &gps)
  {
    std::vector<std::string_view> strComps = split(text, ',');
    if(strComps.size() != 2 && strComps.size() != 3) {
      //SPDLOG_ERROR("Unexpected number of components ({}) in GPS coordinates [{}] ",strComps.size(),text);
      return false;
    }
    
    RealT degLat = std::nan("");
    RealT degLong = std::nan("");
    
    std::string coord1(strComps[0]);
    std::string coord2(strComps[1]);
    
    bool c1useNS = usesNS(coord1);
    bool c1useEW = usesEW(coord1);
    
    bool c2useNS = usesNS(coord2);
    bool c2useEW = usesEW(coord2);
    
    bool useNSEW = c1useNS || c1useEW || c2useNS || c2useEW;
    
    if(!useNSEW) {
      if(!textDMSToAngle(coord1,degLat,AF_None)) {
        return false;
      }
      if(!textDMSToAngle(coord2,degLong,AF_None)) {
        return false;
      }      
    } else {
      if((c1useNS && c1useEW)) {
        //SPDLOG_ERROR("Failed to parse GPS coordinates, confusing use of NS & EW designators. Angle 1=[{}]",coord1);
        return false;
      }
      if((c2useNS && c2useEW)) {
        //SPDLOG_ERROR("Failed to parse GPS coordinates, confusing use of NS & EW designators. Angle 2=[{}]",coord2);
        return false;
      }
      
      if(c1useNS) {
        if(!textDMSToAngle(coord1,degLat,AF_NS))
          return false;
        if(!textDMSToAngle(coord2,degLong,AF_EW))
          return false;
      } else {
        if(!textDMSToAngle(coord1,degLong,AF_EW))
          return false;
        if(!textDMSToAngle(coord2,degLat,AF_NS))
          return false;
      }
    }
    
    // Sort out hight.
    RealT height = 0;
    if(strComps.size() == 3) {
      std::string coord3(strComps[2]);

      height = std::stod(coord3);
    }
    
    gps = GPSCoordinate(degLat,degLong,height);
    return true;
  }

  
  //! Normalise GPS coordinates.
  
  Point<double,3> GPSCoordinate::normalise() const
  {
    // Normalise
    RealT rLat = mAt[0];
    RealT rLong = mAt[1];
    if(rLat > std::numbers::pi)
      rLat -= std::floor(rLat/360) * 360;
    
    if(rLat < -std::numbers::pi)
      rLat += std::floor(-rLat/360) * 360;
    
    if(rLong > std::numbers::pi)
      rLong -= std::floor(rLong/360) * 360;
    
    if(rLong < -std::numbers::pi)
      rLong += std::floor(-rLong/360) * 360;
    
    return Point<double,3> {rLat,rLong,mAt[2]};
  }

  // Print as text in degree's minutes seconds.
  
  std::string GPSCoordinate::textDMS() const {
    Point<double,3> val = normalise();
    
    std::string ret = angleToTextDMS(std::abs(latitude()));
    if(latitude() > 0) {
      ret += " N, ";
    } else {
      ret += " S, ";
    }
    
    ret += angleToTextDMS(std::abs(longitude()));
    if(longitude() > 0) {
      ret += " E, ";
    } else {
      ret += " W, ";
    }
    
    std::string elev = fmt::format("{:3.2f}m",val[2]);
    ret += elev;
    return ret;
  }
  
  std::string GPSCoordinate::angleToTextDMS(RealT value) {
    auto degrees = intFloor(value);
    RealT tmp = (value - static_cast<RealT>(degrees)) * 60;
    auto minutes = intFloor(tmp);
    RealT seconds = (tmp - static_cast<RealT>(minutes)) * 60;
    return fmt::format("{} {}'{:2.4f}\"",
                       degrees,
                       minutes,
                       seconds);
  }
  
  
  // Convert a angle in text Degree's minutes seconds or decimal notation
  
  bool GPSCoordinate::textDMSToAngle(const std::string &value,RealT &angle,AngleFormT af) {
    std::string rest = topAndTail(value);
    //SPDLOG_DEBUG("Parsing angle [{}] ",value);
    
    // Decide the format of the string.
    auto degreesAt = rest.find(' ');
    auto minutesAt = rest.find('\'');
    auto secondsAt = rest.find('\"');
    //IntT desingator = -1;
    RealT tv = 0;
        
    // Is angle in decimal format ?
    
    if(minutesAt == std::string::npos && secondsAt == std::string::npos) {
      angle = std::stod(rest);
    } else {
      std::string tmp;
      if(degreesAt != std::string::npos) {
        tmp = rest.substr(0, degreesAt);
        //SPDLOG_DEBUG("Degrees=[%s] ",tmp.chars());

        angle = std::stod(tmp);
        if(modf(angle,&tv) != 0) {
          SPDLOG_ERROR("Fractional value in degree's gps coordinate. ");
          return false;
        }
      } else {
        if(minutesAt > 0 || secondsAt > 0) {
          SPDLOG_ERROR("Minutes or seconds specified in GPS angle without integer component. ");
          return false;
        }
      }
      
      if(minutesAt != std::string::npos) {
        tmp = rest.substr(degreesAt+1,minutesAt - degreesAt - 1);
        //SPDLOG_DEBUG("Minutes=[%s] ",tmp.chars());
        RealT rv = std::stod(tmp);
        if(secondsAt != std::string::npos) {
          if(modf(angle,&tv) != 0) {
            SPDLOG_ERROR("Fractional value in minutes of gps coordinate.");
            return false;
          }
        }
        if(rv < 0) {
          SPDLOG_ERROR("Negative minutes values in gps coordinate.");
          return false; 
        }
        angle += rv * (1.0/60.0);
        
      } else {
        if(secondsAt > 0) {
          SPDLOG_ERROR("Seconds specified in GPS angle minutes component. [{}] ",rest);
          return false;
        }
      }
      
      if(secondsAt != std::string::npos) {
        tmp = rest.substr(minutesAt+1,secondsAt - minutesAt -1);
        //SPDLOG_DEBUG("Seconds=[%s] ",tmp.chars());
        RealT rv = std::stod(tmp);
        if(rv < 0) {
          SPDLOG_ERROR("Negative values specified in GPS angle seconds component [{}]. ",rest);
          return false;
        }
        angle += rv * (1.0/(60.0*60.0));
      } else {
        tmp = rest.substr(minutesAt+1);
        // Any digits after the minute marker ?
        if(tmp.contains('1') ||
           tmp.contains('2') ||
           tmp.contains('3') ||
           tmp.contains('4') ||
           tmp.contains('5') ||
           tmp.contains('6') ||
           tmp.contains('7') ||
           tmp.contains('8') ||
           tmp.contains('9')) {
          SPDLOG_ERROR("No seconds marker found, but digits after '. [{}]. ",rest);
          return false;
        }
      }
    }
    
    bool haveNorth = rest.contains('N') || rest.contains('n');
    bool haveSouth = rest.contains('S') || rest.contains('s');
    bool haveEast = rest.contains('E') || rest.contains('e');
    bool haveWest = rest.contains('W') || rest.contains('w');
    
    switch(af) {
    case AF_None:
      if(haveNorth || haveSouth || haveEast || haveWest) {
        SPDLOG_ERROR("Unexpected designator in [{}] ",value);
        return false;
      }
      break;
      
    case AF_NS:
      if(haveEast || haveWest || (haveNorth && haveSouth)) {
        SPDLOG_ERROR("Inconsistent designator in [{}] ",value);
        return false;
      }
      if(angle < 0) {
        SPDLOG_ERROR("Negative angle used in gps coordinate with N/S designator. ");
        return false;
      }
      
      if(haveSouth)
        angle *= -1;
      break;
      
    case AF_EW:
      if(haveNorth || haveSouth || (haveEast && haveWest)) {
        SPDLOG_ERROR("Inconsistent designator in [{}] ",value);
        return false;
      }
      if(angle < 0) {
        SPDLOG_ERROR("Negative angle used in gps coordinate with E/W designator. ");
        return false;
      }
      if(haveWest)
        angle *= -1;      
      break;
    }
    
    return true;
  }

  
  //! Convert GPS coordinate into global Cartesian coordinates.
  
  Point<double,3> GPSCoordinate::cartesian() const
  {
    RealT latRad = deg2rad(mAt[0]);
    RealT longRad = deg2rad(mAt[1]);
    
    RealT sinTheta = std::sin(latRad);
    RealT cosTheta = std::cos(latRad);
    RealT sinLambda = std::sin(longRad);
    RealT cosLambda = std::cos(longRad);
    
    RealT H = mAt[2];
    RealT v = m_a / std::sqrt(1-m_e2*sqr(sinTheta));
    
    Point<double,3> ret;
    RealT vPlusH = (v + H);
    ret[0] = vPlusH * cosTheta * cosLambda; // X
    ret[1] = vPlusH * cosTheta * sinLambda; // Y
    ret[2] = ((1-m_e2) * v + H) * sinTheta; // Z
    
    return ret;
  }
  
  //! Access local vertical direction
  
  Vector<double,3> GPSCoordinate::vertical() const {
    
    RealT latRad = deg2rad(mAt[0]);
    RealT longRad = deg2rad(mAt[1]);
    
    RealT sinTheta = std::sin(latRad);
    RealT cosTheta = std::cos(latRad);
    RealT sinLambda = std::sin(longRad);
    RealT cosLambda = std::cos(longRad);
    
    Vector<double,3> ret;
    ret[0] = cosTheta * cosLambda;
    ret[1] = cosTheta * sinLambda;
    ret[2] = sinTheta;
    
    return ret;
    
  }

  //! Interpolate between two GPS positions.
  //! Fraction is between 0.0 and 1.0, where 0 is p1, and 1 is p2.

  bool GPSCoordinate::bilinearInterpolate(RealT fraction,
                                           const GPSCoordinate &p1,
                                           const GPSCoordinate &p2,
                                           GPSCoordinate &position)
  {
    RealT mf = 1.0 - fraction;
    // FIXME: This isn't ideal.  We really want to find the shortest path over a sphere,
    // but it'll do for now.
    position = GPSCoordinate(p1.latitude() * mf + p2.latitude() * fraction,
                              p1.longitude() * mf + p2.longitude() * fraction,
                              p1.height() * mf + p2.height() * fraction,
                              p1.horizontalErrorBounds() * mf + p2.horizontalErrorBounds() * fraction,
                              p1.verticalErrorBounds() * mf + p2.verticalErrorBounds() * fraction
                              );
    return true;
  }


  //! Compute the differential 
  
  bool GPSCoordinate::differential(Vector<double,3> &diffLat,Vector<double,3> &diffLong,Vector<double,3> &diffHeight) const
  {
    RealT latRad = deg2rad(mAt[0]);
    RealT longRad = deg2rad(mAt[1]);
    
    RealT sinTheta = std::sin(latRad);
    RealT cosTheta = std::cos(latRad);
    RealT sinLambda = std::sin(longRad);
    RealT cosLambda = std::cos(longRad);
    
    RealT H = mAt[2];

    RealT q3 = std::sqrt(1-m_e2*sqr(sinTheta));
    
    {
      RealT q1 = m_a*m_e2*sqr(cosTheta)*sinTheta/std::pow((1-m_e2*sqr(sinTheta)),(3.0/2.0));
      RealT q2 = sinTheta*(H+m_a/q3);
      
      diffLat[0] = q1*cosLambda-q2*cosLambda;
      diffLat[1] = q1*sinLambda-q2*sinLambda;
      diffLat[2] = 
        cosTheta*(H+m_a*(1-m_e2)/q3)+
        m_a*(1-m_e2)*m_e2*cosTheta*sqr(sinTheta)/std::pow((1-m_e2*sqr(sinTheta)),(3.0/2.0));
    }
    
    {
      RealT q4 = cosTheta*(H+m_a/q3);
      
      diffLong[0] = -q4*sinLambda;
      diffLong[1] =  q4*cosLambda;
      diffLong[2] =  0;
    }
    
    
    {
      // Height.
      diffHeight[0] = cosTheta * cosLambda;
      diffHeight[1] = cosTheta * sinLambda;
      diffHeight[2] = sinTheta;
    }
    
    
    return true;
  }


}
