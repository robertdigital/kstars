/***************************************************************************
                          skycomposite.cpp  -  K Desktop Planetarium
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

#include "skycomposite.h"

#include "kstars.h"
#include "kstarsdata.h"
#include "skyobject.h"

SkyComposite::SkyComposite(SkyComponent *parent ) 
: SkyComponent( parent )
{
}

SkyComposite::~SkyComposite()
{
	while ( !components().isEmpty() )
		delete components().takeFirst();
}

void SkyComposite::addComponent(SkyComponent *component)
{
	components().append(component);
}

void SkyComposite::removeComponent(SkyComponent *component)
{
	int index = components().indexOf(component);
	if (index != -1)
	{
		// component is in list
		components().removeAt(index);
		delete component;
	}
}

void SkyComposite::draw(KStars *ks, QPainter& psky, double scale)
{
	foreach (SkyComponent *component, components())
		component->draw(ks, psky, scale);
}

void SkyComposite::drawExportable(KStars *ks, QPainter& psky, double scale)
{
	foreach (SkyComponent *component, components())
		component->drawExportable(ks, psky, scale);
}

void SkyComposite::init(KStarsData *data)
{
	foreach (SkyComponent *component, components())
		component->init(data);
}

void SkyComposite::update(KStarsData *data, KSNumbers *num )
{
	foreach (SkyComponent *component, components())
		component->update( data, num );
}

SkyObject* SkyComposite::findByName( const QString &name ) {
	SkyObject *o = 0;
	foreach ( SkyComponent *comp, components() ) {
		o = comp->findByName( name );
		if ( o ) return o;
	}
	return 0;
}
