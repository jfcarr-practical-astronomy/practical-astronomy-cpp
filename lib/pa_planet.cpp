#include "pa_planet.h"
#include "pa_data.h"
#include "pa_macros.h"
#include "pa_types.h"
#include "pa_util.h"
#include <cmath>
#include <string>

using namespace pa_types;
using namespace pa_util;
using namespace pa_macros;

/**
 * Calculate approximate position of a planet.
 *
 * @return tuple <double planetRAHour, double planetRAMin, double planetRASec,
 * double planetDecDeg, double planetDecMin, double planetDecSec>
 */
std::tuple<double, double, double, double, double, double>
PAPlanet::ApproximatePositionOfPlanet(double lctHour, double lctMin,
                                      double lctSec, bool isDaylightSaving,
                                      int zoneCorrectionHours,
                                      double localDateDay, int localDateMonth,
                                      int localDateYear,
                                      std::string planetName) {
  int daylightSaving = (isDaylightSaving) ? 1 : 0;

  pa_data::PlanetData planetInfo = pa_data::planetLookup(planetName);

  double gdateDay = pa_macros::LocalCivilTimeGreenwichDay(
      lctHour, lctMin, lctSec, daylightSaving, zoneCorrectionHours,
      localDateDay, localDateMonth, localDateYear);
  int gdateMonth = pa_macros::LocalCivilTimeGreenwichMonth(
      lctHour, lctMin, lctSec, daylightSaving, zoneCorrectionHours,
      localDateDay, localDateMonth, localDateYear);
  int gdateYear = pa_macros::LocalCivilTimeGreenwichYear(
      lctHour, lctMin, lctSec, daylightSaving, zoneCorrectionHours,
      localDateDay, localDateMonth, localDateYear);

  double utHours = pa_macros::LocalCivilTimeToUniversalTime(
      lctHour, lctMin, lctSec, daylightSaving, zoneCorrectionHours,
      localDateDay, localDateMonth, localDateYear);
  double dDays = pa_macros::CivilDateToJulianDate(gdateDay + (utHours / 24),
                                                  gdateMonth, gdateYear) -
                 pa_macros::CivilDateToJulianDate(0, 1, 2010);
  double npDeg1 = 360 * dDays / (365.242191 * planetInfo.tp_PeriodOrbit);
  double npDeg2 = npDeg1 - 360 * std::floor(npDeg1 / 360);
  double mpDeg = npDeg2 + planetInfo.long_LongitudeEpoch -
                 planetInfo.peri_LongitudePerihelion;
  double lpDeg1 = npDeg2 +
                  (360 * planetInfo.ecc_EccentricityOrbit *
                   sin(pa_util::DegreesToRadians(mpDeg)) / M_PI) +
                  planetInfo.long_LongitudeEpoch;
  double lpDeg2 = lpDeg1 - 360 * std::floor(lpDeg1 / 360);
  double planetTrueAnomalyDeg = lpDeg2 - planetInfo.peri_LongitudePerihelion;
  double rAU = planetInfo.axis_AxisOrbit *
               (1 - pow(planetInfo.ecc_EccentricityOrbit, 2)) /
               (1 + planetInfo.ecc_EccentricityOrbit *
                        cos(pa_util::DegreesToRadians(planetTrueAnomalyDeg)));

  pa_data::PlanetData earthInfo = pa_data::planetLookup("Earth");

  double neDeg1 = 360 * dDays / (365.242191 * earthInfo.tp_PeriodOrbit);
  double neDeg2 = neDeg1 - 360 * std::floor(neDeg1 / 360);
  double meDeg = neDeg2 + earthInfo.long_LongitudeEpoch -
                 earthInfo.peri_LongitudePerihelion;
  double leDeg1 = neDeg2 + earthInfo.long_LongitudeEpoch +
                  360 * earthInfo.ecc_EccentricityOrbit *
                      sin(pa_util::DegreesToRadians(meDeg)) / M_PI;
  double leDeg2 = leDeg1 - 360 * std::floor(leDeg1 / 360);
  double earthTrueAnomalyDeg = leDeg2 - earthInfo.peri_LongitudePerihelion;
  double rAU2 = earthInfo.axis_AxisOrbit *
                (1 - pow(earthInfo.ecc_EccentricityOrbit, 2)) /
                (1 + earthInfo.ecc_EccentricityOrbit *
                         cos(pa_util::DegreesToRadians(earthTrueAnomalyDeg)));
  double lpNodeRad = pa_util::DegreesToRadians(
      lpDeg2 - planetInfo.node_LongitudeAscendingNode);
  double psiRad =
      asin(sin(lpNodeRad) *
           sin(pa_util::DegreesToRadians(planetInfo.incl_OrbitalInclination)));
  double y = sin(lpNodeRad) *
             cos(pa_util::DegreesToRadians(planetInfo.incl_OrbitalInclination));
  double x = cos(lpNodeRad);
  double ldDeg = pa_macros::WToDegrees(atan2(y, x)) +
                 planetInfo.node_LongitudeAscendingNode;
  double rdAU = rAU * cos(psiRad);
  double leLdRad = pa_util::DegreesToRadians(leDeg2 - ldDeg);
  double atan2Type1 = atan2(rdAU * sin(leLdRad), rAU2 - rdAU * cos(leLdRad));

  double atan2Type2 = atan2(rAU2 * sin(-leLdRad), rdAU - rAU2 * cos(leLdRad));
  double aRad = (rdAU < 1) ? atan2Type1 : atan2Type2;
  double lamdaDeg1 = (rdAU < 1) ? 180 + leDeg2 + pa_macros::WToDegrees(aRad)
                                : pa_macros::WToDegrees(aRad) + ldDeg;
  double lamdaDeg2 = lamdaDeg1 - 360 * std::floor(lamdaDeg1 / 360);
  double betaDeg = pa_macros::WToDegrees(atan(
      rdAU * tan(psiRad) * sin(pa_util::DegreesToRadians(lamdaDeg2 - ldDeg)) /
      (rAU2 * sin(-leLdRad))));
  double raHours =
      pa_macros::DecimalDegreesToDegreeHours(pa_macros::EclipticRightAscension(
          lamdaDeg2, 0, 0, betaDeg, 0, 0, gdateDay, gdateMonth, gdateYear));
  double decDeg = pa_macros::EclipticDeclination(
      lamdaDeg2, 0, 0, betaDeg, 0, 0, gdateDay, gdateMonth, gdateYear);

  int planetRAHour = pa_macros::DecimalHoursHour(raHours);
  int planetRAMin = pa_macros::DecimalHoursMinute(raHours);
  double planetRASec = pa_macros::DecimalHoursSecond(raHours);
  double planetDecDeg = pa_macros::DecimalDegreesDegrees(decDeg);
  double planetDecMin = pa_macros::DecimalDegreesMinutes(decDeg);
  double planetDecSec = pa_macros::DecimalDegreesSeconds(decDeg);

  return std::tuple<double, double, double, double, double, double>{
      planetRAHour, planetRAMin,  planetRASec,
      planetDecDeg, planetDecMin, planetDecSec};
}
