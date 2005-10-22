/***************************************************************************
              solarsystemlistcomponent.cpp  -  K Desktop Planetarium
                             -------------------
    begin                : 2005/22/09
    copyright            : (C) 2005 by Jason Harris
    email                : kstars@30doradus.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "solarsystemlistcomponent.h"

SolarSystemListComponent::SolarSystemListComponent( SolarSystemComposite *parent )
  : ListComponent( parent )
{
}

SolarSystemListComponent::~SolarSystemListComponent()
{
  //Object deletes handled by parent class (ListComponent)
}

