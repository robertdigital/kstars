/***************************************************************************
                          horizoncomponent.h  -  K Desktop Planetarium
                             -------------------
    begin                : 2005/07/08
    copyright            : (C) 2005 by Thomas Kabelmann
    email                : thomas.kabelmann@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef HORIZONCOMPONENT_H
#define HORIZONCOMPONENT_H

class QList;

class SkyComposite;
class KStarsData;
class SkyMap;
class KSNumbers;

/**
	*@class HorizonComponent
	*Represents the horizon on the sky map.
	
	*@author Thomas Kabelmann
	*@version 0.1
	*/
class HorizonComponent: public SkyComponent
{
	public:
  
		/**
		 *@short Constructor
		 *@p parent Pointer to the parent SkyComposite object
		 */
		HorizonComponent(SkyComposite *parent);

		/**
		 *@short Destructor
		 */
		virtual ~HorizonComponent();

		/**
		 *@short Draw the Horizon on the Sky map
		 *@p map Pointer to the SkyMap object
		 *@p psky Reference to the QPainter on which to paint
		 *@p scale the scaling factor for drawing (1.0 for screen draws)
		 */
		virtual void draw(KStars *ks, QPainter& psky, double scale);

		/**
		 *@short Initialize the Horizon
		 *@p data Pointer to the KStarsData object
		 */
		virtual void init(KStarsData *data);
	
		/**
		 *@short Update the coordinates of the Horizon
		 *@p data Pointer to the KStarsData object
		 *@p num Pointer to the KSNumbers object
		 *@p needNewCoords true if cordinates should be recomputed
		 */
		virtual void update(KStarsData*, KSNumbers*, bool needNewCoords);

	private:
		// the points of the horizon
		QList<SkyPoint*> Horizon;
};

#endif
