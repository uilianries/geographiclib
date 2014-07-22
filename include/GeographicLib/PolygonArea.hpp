/**
 * \file PolygonArea.hpp
 * \brief Header for GeographicLib::PolygonArea class
 *
 * Copyright (c) Charles Karney (2010-2011) <charles@karney.com> and licensed
 * under the MIT/X11 License.  For more information, see
 * http://geographiclib.sourceforge.net/
 **********************************************************************/

#if !defined(GEOGRAPHICLIB_POLYGONAREA_HPP)
#define GEOGRAPHICLIB_POLYGONAREA_HPP 1

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicExact.hpp>
#include <GeographicLib/Accumulator.hpp>

namespace GeographicLib {

  /**
   * \brief Polygon areas
   *
   * This computes the area of a polygon whose edges are geodesics using the
   * method given in Section 6 of
   * - C. F. F. Karney,
   *   <a href="http://dx.doi.org/10.1007/s00190-012-0578-z">
   *   Algorithms for geodesics</a>,
   *   J. Geodesy <b>87</b>, 43--55 (2013);
   *   DOI: <a href="http://dx.doi.org/10.1007/s00190-012-0578-z">
   *   10.1007/s00190-012-0578-z</a>;
   *   addenda: <a href="http://geographiclib.sf.net/geod-addenda.html">
   *   geod-addenda.html</a>.
   *
   * This class lets you add vertices and edges one at a time to the polygon.
   * The sequence must start with a vertex and thereafter vertices and edges
   * can be added in any order.  Any vertex after the first creates a new edge
   * which is the ''shortest'' geodesic from the previous vertex.  In some
   * cases there may be two or many such shortest geodesics and the area is
   * then not uniquely defined.  In this case, either add an intermediate
   * vertex or add the edge ''as'' an edge (by defining its direction and
   * length).
   *
   * The area and perimeter are accumulated in two times the standard floating
   * point precision to guard against the loss of accuracy with many-sided
   * polygons.  At any point you can ask for the perimeter and area so far.
   * There's an option to treat the points as defining a polyline instead of a
   * polygon; in that case, only the perimeter is computed.
   *
   * This is a templated class to allow it to be used with either Geodesic and
   * GeodesicExact.  GeographicLib::PolygonArea and
   * GeographicLib::PolygonAreaExact are typedefs for these two cases.
   *
   * @tparam GeodType the geodesic class to use.
   *
   * Example of use:
   * \include example-PolygonArea.cpp
   *
   * <a href="Planimeter.1.html">Planimeter</a> is a command-line utility
   * providing access to the functionality of PolygonArea.
   **********************************************************************/

  template <class GeodType = Geodesic>
  class PolygonAreaT {
  private:
    typedef Math::real real;
    GeodType _earth;
    real _area0;                // Full ellipsoid area
    bool _polyline;             // Assume polyline (don't close and skip area)
    unsigned _mask;
    unsigned _num;
    int _crossings;
    Accumulator<> _areasum, _perimetersum;
    real _lat0, _lon0, _lat1, _lon1;
    static inline int transit(real lon1, real lon2) {
      // Return 1 or -1 if crossing prime meridian in east or west direction.
      // Otherwise return zero.
      // Compute lon12 the same way as Geodesic::Inverse.
      lon1 = Math::AngNormalize(lon1);
      lon2 = Math::AngNormalize(lon2);
      real lon12 = Math::AngDiff(lon1, lon2);
      int cross =
        lon1 < 0 && lon2 >= 0 && lon12 > 0 ? 1 :
        (lon2 < 0 && lon1 >= 0 && lon12 < 0 ? -1 : 0);
      return cross;
    }
  public:

    /**
     * Constructor for PolygonAreaT.
     *
     * @param[in] earth the Geodesic object to use for geodesic calculations.
     * @param[in] polyline if true that treat the points as defining a polyline
     *   instead of a polygon (default = false).
     **********************************************************************/
    PolygonAreaT(const GeodType& earth, bool polyline = false)
      : _earth(earth)
      , _area0(_earth.EllipsoidArea())
      , _polyline(polyline)
      , _mask(GeodType::LATITUDE | GeodType::LONGITUDE | GeodType::DISTANCE |
              (_polyline ? GeodType::NONE : GeodType::AREA))
    { Clear(); }

    /**
     * Clear PolygonAreaT, allowing a new polygon to be started.
     **********************************************************************/
    void Clear() {
      _num = 0;
      _crossings = 0;
      _areasum = 0;
      _perimetersum = 0;
      _lat0 = _lon0 = _lat1 = _lon1 = Math::NaN();
    }

    /**
     * Add a point to the polygon or polyline.
     *
     * @param[in] lat the latitude of the point (degrees).
     * @param[in] lon the longitude of the point (degrees).
     *
     * \e lat should be in the range [&minus;90&deg;, 90&deg;] and \e
     * lon should be in the range [&minus;540&deg;, 540&deg;).
     **********************************************************************/
    void AddPoint(real lat, real lon);

    /**
     * Add an edge to the polygon or polyline.
     *
     * @param[in] azi azimuth at current point (degrees).
     * @param[in] s distance from current point to next point (meters).
     *
     * \e azi should be in the range [&minus;540&deg;, 540&deg;).  This does
     * nothing if no points have been added yet.  Use PolygonAreaT::CurrentPoint
     * to determine the position of the new vertex.
     **********************************************************************/
    void AddEdge(real azi, real s);

    /**
     * Return the results so far.
     *
     * @param[in] reverse if true then clockwise (instead of counter-clockwise)
     *   traversal counts as a positive area.
     * @param[in] sign if true then return a signed result for the area if
     *   the polygon is traversed in the "wrong" direction instead of returning
     *   the area for the rest of the earth.
     * @param[out] perimeter the perimeter of the polygon or length of the
     *   polyline (meters).
     * @param[out] area the area of the polygon (meters<sup>2</sup>); only set
     *   if \e polyline is false in the constructor.
     * @return the number of points.
     **********************************************************************/
    unsigned Compute(bool reverse, bool sign,
                     real& perimeter, real& area) const;

    /**
     * Return the results assuming a tentative final test point is added;
     * however, the data for the test point is not saved.  This lets you report
     * a running result for the perimeter and area as the user moves the mouse
     * cursor.  Ordinary floating point arithmetic is used to accumulate the
     * data for the test point; thus the area and perimeter returned are less
     * accurate than if PolygonAreaT::AddPoint and PolygonAreaT::Compute are
     * used.
     *
     * @param[in] lat the latitude of the test point (degrees).
     * @param[in] lon the longitude of the test point (degrees).
     * @param[in] reverse if true then clockwise (instead of counter-clockwise)
     *   traversal counts as a positive area.
     * @param[in] sign if true then return a signed result for the area if
     *   the polygon is traversed in the "wrong" direction instead of returning
     *   the area for the rest of the earth.
     * @param[out] perimeter the approximate perimeter of the polygon or length
     *   of the polyline (meters).
     * @param[out] area the approximate area of the polygon
     *   (meters<sup>2</sup>); only set if polyline is false in the
     *   constructor.
     * @return the number of points.
     *
     * \e lat should be in the range [&minus;90&deg;, 90&deg;] and \e
     * lon should be in the range [&minus;540&deg;, 540&deg;).
     **********************************************************************/
    unsigned TestPoint(real lat, real lon, bool reverse, bool sign,
                       real& perimeter, real& area) const;

    /**
     * Return the results assuming a tentative final test point is added via an
     * azimuth and distance; however, the data for the test point is not saved.
     * This lets you report a running result for the perimeter and area as the
     * user moves the mouse cursor.  Ordinary floating point arithmetic is used
     * to accumulate the data for the test point; thus the area and perimeter
     * returned are less accurate than if PolygonAreaT::AddEdge and
     * PolygonAreaT::Compute are used.
     *
     * @param[in] azi azimuth at current point (degrees).
     * @param[in] s distance from current point to final test point (meters).
     * @param[in] reverse if true then clockwise (instead of counter-clockwise)
     *   traversal counts as a positive area.
     * @param[in] sign if true then return a signed result for the area if
     *   the polygon is traversed in the "wrong" direction instead of returning
     *   the area for the rest of the earth.
     * @param[out] perimeter the approximate perimeter of the polygon or length
     *   of the polyline (meters).
     * @param[out] area the approximate area of the polygon
     *   (meters<sup>2</sup>); only set if polyline is false in the
     *   constructor.
     * @return the number of points.
     *
     * \e azi should be in the range [&minus;540&deg;, 540&deg;).
     **********************************************************************/
    unsigned TestEdge(real azi, real s, bool reverse, bool sign,
                      real& perimeter, real& area) const;

    /// \cond SKIP
    /**
     * <b>DEPRECATED</b>
     * The old name for PolygonAreaT::TestPoint.
     **********************************************************************/
    unsigned TestCompute(real lat, real lon, bool reverse, bool sign,
                         real& perimeter, real& area) const {
      return TestPoint(lat, lon, reverse, sign, perimeter, area);
    }
    /// \endcond

    /** \name Inspector functions
     **********************************************************************/
    ///@{
    /**
     * @return \e a the equatorial radius of the ellipsoid (meters).  This is
     *   the value inherited from the Geodesic object used in the constructor.
     **********************************************************************/

    Math::real MajorRadius() const { return _earth.MajorRadius(); }

    /**
     * @return \e f the flattening of the ellipsoid.  This is the value
     *   inherited from the Geodesic object used in the constructor.
     **********************************************************************/
    Math::real Flattening() const { return _earth.Flattening(); }

    /**
     * Report the previous vertex added to the polygon or polyline.
     *
     * @param[out] lat the latitude of the point (degrees).
     * @param[out] lon the longitude of the point (degrees).
     *
     * If no points have been added, then NaNs are returned.  Otherwise, \e lon
     * will be in the range [&minus;180&deg;, 180&deg;).
     **********************************************************************/
    void CurrentPoint(real& lat, real& lon) const
    { lat = _lat1; lon = _lon1; }
    ///@}
  };

  /**
   * @relates PolygonAreaT
   *
   * Polygon areas using Geodesic.  This should be used if the flattening is
   * small.
   **********************************************************************/
  typedef PolygonAreaT<Geodesic> PolygonArea;

  /**
   * @relates PolygonAreaT
   *
   * Polygon areas using GeodesicExact.  (But note that the implementation of
   * areas in GeodesicExact uses a high order series and this is only accurate
   * for modest flattenings.)
   **********************************************************************/
  typedef PolygonAreaT<GeodesicExact> PolygonAreaExact;

} // namespace GeographicLib

#endif  // GEOGRAPHICLIB_POLYGONAREA_HPP
