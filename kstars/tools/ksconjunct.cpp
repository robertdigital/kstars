/***************************************************************************
                          ksconjunct.cpp  -  K Desktop Planetarium
                             -------------------
    begin                : Sat Mar 22 2008
    copyright            : (C) 2008 by Akarsh Simha
    email                : kstar@bas.org.in
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ksconjunct.h"

#include <math.h>

#include "ksnumbers.h"
#include "skyobjects/ksplanetbase.h"
#include "skyobjects/ksplanet.h"
#include "skyobjects/ksasteroid.h"
#include "skyobjects/kscomet.h"
#include "kstarsdata.h"

KSConjunct::KSConjunct() {
    geoPlace = KStarsData::Instance()->geo();
}

void KSConjunct::setGeoLocation( GeoLocation *geo ) {
    if( geo != NULL )
        geoPlace = geo;
    else
        geoPlace = KStarsData::Instance()->geo();
}

QMap<long double, dms> KSConjunct::findClosestApproach(SkyObject& Object1, KSPlanetBase& Object2, long double startJD, long double stopJD, dms maxSeparation,bool _opposition) {

  QMap<long double, dms> Separations;
  QPair<long double, dms> extremum;
  dms Dist;
  dms prevDist;
  double step, step0;
  int Sign, prevSign;
  opposition=_opposition;
  //  kDebug() << "Entered KSConjunct::findClosestApproach() with startJD = " << (double)startJD;
  //  kDebug() << "Initial Positional Information: \n";
  //  kDebug() << Object1.name() << ": RA = " << Object1.ra() -> toHMSString() << "; Dec = " << Object1.dec() -> toDMSString() << "\n";
  //  kDebug() << Object2.name() << ": RA = " << Object2.ra() -> toHMSString() << "; Dec = " << Object2.dec() -> toDMSString() << "\n";
  prevSign = 0;
  
  step0 = (stopJD - startJD) / 4.0;
  if(Object1.name() == i18n( "Mars" ) || Object2.name() == i18n( "Mars" ))
    if (step0 > 10.0)
      step0 = 10.0;
  if(Object1.name() == i18n( "Venus" ) || Object1.name() == i18n( "Mercury" ) || Object2.name() == i18n( "Mercury" ) || Object2.name() == i18n( "Venus" )) 
    if (step0 > 5.0)
      step0 = 5.0;
  if(Object1.name() == "Moon" || Object2.name() == "Moon")
    if (step0 > 0.25)
      step0 = 0.25;

  step = step0;
  
  //	kDebug() << "Initial Separation between " << Object1.name() << " and " << Object2.name() << " = " << (prevDist.toDMSString());

  long double jd = startJD;
  prevDist = findDistance(jd, &Object1, &Object2);
  jd += step;
  while ( jd <= stopJD ) {
    int progress = int( 100.0*(jd - startJD)/(stopJD - startJD) );
    emit madeProgress( progress );
    
    Dist = findDistance(jd, &Object1, &Object2);
    Sign = sgn(Dist.Degrees() - prevDist.Degrees()); 
    //	kDebug() << "Dist = " << Dist.toDMSString() << "; prevDist = " << prevDist.toDMSString() << "; Difference = " << (Dist.Degrees() - prevDist.Degrees()) << "; Step = " << step;

    //How close are we to a conjunction, and how fast are we approaching one?
    double factor = fabs( (Dist.Degrees() - prevDist.Degrees()) / Dist.Degrees());
    if ( factor > 10.0 ) { //let's go faster!
        step = step0 * factor/10.0;
    } else { //slow down, we're getting close!
        step = step0;
    }
    
    if( Sign != prevSign && prevSign == -1) { //all right, we may have just passed a conjunction
        if ( step > step0 ) { //mini-loop to back up and make sure we're close enough
            //            kDebug() << "Entering slow loop: " << endl;
            jd -= step;
            step = step0;
            Sign = prevSign;
            while ( jd <= stopJD ) {
                Dist = findDistance(jd, &Object1, &Object2);
                Sign = sgn(Dist.Degrees() - prevDist.Degrees()); 
                //	kDebug() << "Dist=" << Dist.toDMSString() << "; prevDist=" << prevDist.toDMSString() << "; Diff=" << (Dist.Degrees() - prevDist.Degrees()) << "djd=" << (int)(jd - startJD);
                if ( Sign != prevSign ) break;
                
                prevDist = Dist;
                prevSign = Sign;
                jd += step;
            }
        }
        
        //	kDebug() << "Sign = " << Sign << " and " << "prevSign = " << prevSign << ": Entering findPrecise()\n";
        if(findPrecise(&extremum, &Object1, &Object2, jd, step, Sign))
            if(extremum.second.radians() < maxSeparation.radians())
                Separations.insert(extremum.first, extremum.second);
    }

    prevDist = Dist;
    prevSign = Sign;
    jd += step;
  }

  return Separations;
}


dms KSConjunct::findDistance(long double jd, SkyObject *Object1, KSPlanetBase *Object2)
{
  KStarsDateTime t(jd);
  KSNumbers num(jd);
  dms dist;

  KSPlanet *m_Earth = new KSPlanet( I18N_NOOP( "Earth" ), QString(), QColor( "white" ), 12756.28 /*diameter in km*/ );
  m_Earth -> findPosition( &num );
  dms LST(geoPlace->GSTtoLST(t.gst()));

  // FIXME: This should be virtual functions or whatever
  if( Object1->isSolarSystem() ) {
      switch( Object1->type() ) {
      case 2: {
          KSPlanet *Planet = (KSPlanet *)Object1;
          Planet->findPosition(&num, geoPlace->lat(), &LST, (KSPlanetBase *)m_Earth);
          break;
      }
      case 9: {
          KSComet *Comet = (KSComet *)Object1;
          Comet->findPosition(&num, geoPlace->lat(), &LST, (KSPlanetBase *)m_Earth);
          break;
      }
      case 10: {
          KSAsteroid *Asteroid = (KSAsteroid *)Object1;
          Asteroid->findPosition(&num, geoPlace->lat(), &LST, (KSPlanetBase *)m_Earth);
          break;
      }
      }
  }
  else
      Object1->updateCoords( &num );

  Object2->findPosition(&num, geoPlace->lat(), &LST, (KSPlanetBase *)m_Earth);
  if(opposition) {  Object2->setRA( Object2->ra()->Hours() + 12.0); Object2->setDec( -Object2->dec()->Degrees()); }
   dist.setRadians(Object1 -> angularDistanceTo(Object2).radians());
  if(opposition) {  Object2->setRA( Object2->ra()->Hours() - 12.0); Object2->setDec( -Object2->dec()->Degrees()); }
  
  return dist;
}

bool KSConjunct::findPrecise(QPair<long double, dms> *out, SkyObject *Object1, KSPlanetBase *Object2, long double jd, double step, int prevSign) {
  dms prevDist;
  int Sign;
  dms Dist;

  if( out == NULL ) {
    kDebug() << "ERROR: Argument out to KSConjunct::findPrecise(...) was NULL!";
    return false;
  }

  prevDist = findDistance(jd, Object1, Object2);

  step = -step / 2.0;
  prevSign = -prevSign;

  while(true) {
    jd += step;
    Dist = findDistance(jd, Object1, Object2);
    //    kDebug() << "Dist=" << Dist.toDMSString() << "; prevDist=" << prevDist.toDMSString() << "; Diff=" << (Dist.Degrees() - prevDist.Degrees()) << "step=" << step;

    if(fabs(step) < 1.0 / (24.0*60.0) ) {
      out -> first = jd - step / 2.0;
      out -> second = findDistance(jd - step/2.0, Object1, Object2);
      if(out -> second.radians() < findDistance(jd - 5.0, Object1, Object2).radians())
        return true;
      else
        return false;
    }
    Sign = sgn(Dist.Degrees() - prevDist.Degrees());
    if(Sign != prevSign) {
      step = -step / 2.0;
      Sign = -Sign;
    }
    prevSign = Sign;
    prevDist = Dist;
  }
}

int KSConjunct::sgn(dms a) {

  // Auxiliary function used by the KSConjunct::findClosestApproach(...) 
  // method and the KSConjunct::findPrecise(...) method

  return ((a.radians() > 0)?1:((a.radians() < 0)?-1:0));

}
#include "ksconjunct.moc"
