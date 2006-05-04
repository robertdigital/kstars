/*  Image Sequence
    Capture image sequence from an imaging device.
    
    Copyright (C) 2003 Jasem Mutlaq (mutlaqja@ikarustech.com)

    This application is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
 */

#include <QObject>
#include <QTimer>
#include <QStringList>

#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <klineedit.h>
#include <kprogressbar.h>
#include <knuminput.h>

#include "imagesequence.h"
#include "kstars.h"
#include "indidriver.h"
#include "indielement.h"
#include "indiproperty.h"
#include "indimenu.h"
#include "indidevice.h"
#include "indistd.h"
#include "devicemanager.h"
#include "Options.h"

#define RETRY_MAX	12
#define RETRY_PERIOD	5000

imagesequence::imagesequence(QWidget* parent): QDialog(parent)
{
	ksw = (KStars *) parent;
	INDIMenu *devMenu = ksw->getINDIMenu();
	
         setupUi(this);

	if (devMenu)
	{
		connect (devMenu, SIGNAL(newDevice()), this, SLOT(newCCD()));
		connect (devMenu, SIGNAL(newDevice()), this, SLOT(newFilter()));
	}
	
	seqTimer = new QTimer(this);
	
//	setModal(false);
	
	// Connect signals and slots
	connect(startB, SIGNAL(clicked()), this, SLOT(startSequence()));
	connect(stopB, SIGNAL(clicked()), this, SLOT(stopSequence()));
	connect(closeB, SIGNAL(clicked()), this, SLOT(close()));
	connect(seqTimer, SIGNAL(timeout()), this, SLOT(prepareCapture()));
	connect(CCDCombo, SIGNAL(activated(int)), this, SLOT(checkCCD(int)));
	connect(filterCombo, SIGNAL(activated(int)), this, SLOT(updateFilterCombo(int)));
	
	active = false;
	ISOStamp = false;
	seqExpose = 0;
	seqTotalCount = 0;
	seqCurrentCount = 0;
	seqDelay = 0;
	lastCCD = 0;
	lastFilter = 0;
	stdDevCCD = NULL;
	stdDevFilter = NULL;
}

imagesequence::~imagesequence()
{
}

bool imagesequence::updateStatus()
{
	bool result;
	
	result = setupCCDs();
	setupFilters();
	
	// If everything okay, let's show the dialog
	return result;
}

void imagesequence::newCCD()
{
	// Only update when it's visible
	if (isVisible())
		setupCCDs();
}

void imagesequence::newFilter()
{
	// Only update when it's visible
	if (isVisible())
		setupFilters();
}

bool imagesequence::setupCCDs()
{
	bool imgDeviceFound (false);
	INDI_P *imgProp;
	INDIMenu *devMenu = ksw->getINDIMenu();
	if (devMenu == NULL)
		return false;
	
	CCDCombo->clear();
	
	for (int i=0; i < devMenu->mgr.size(); i++)
	{
		for (int j=0; j < devMenu->mgr.at(i)->indi_dev.size(); j++)
		{
			imgProp = devMenu->mgr.at(i)->indi_dev.at(j)->findProp("CCD_EXPOSE_DURATION");
			if (!imgProp)
				continue;

			imgDeviceFound = true;
		 
			if (devMenu->mgr.at(i)->indi_dev.at(j)->label.isEmpty())
				devMenu->mgr.at(i)->indi_dev.at(j)->label = 
					devMenu->mgr.at(i)->indi_dev.at(j)->name;
				
			CCDCombo->addItem(devMenu->mgr.at(i)->indi_dev.at(j)->label);
			
		}
	}

	if (imgDeviceFound) {
		CCDCombo->setCurrentIndex(lastCCD);
		currentCCD = CCDCombo->currentText();
	}
	else return false;

	if (!verifyCCDIntegrity())
	{
		stopSequence();
		return false;
	}
	else
	{
		INDI_P *exposeProp;
		INDI_E *exposeElem;
		
		exposeProp = stdDevCCD->dp->findProp("CCD_EXPOSE_DURATION");
		if (!exposeProp)
		{
			KMessageBox::error(this, i18n("Device does not support CCD_EXPOSE_DURATION property."));
			return false;
		}
	
		exposeElem = exposeProp->findElement("EXPOSE_DURATION");
		if (!exposeElem)
		{
			KMessageBox::error(this, i18n("CCD_EXPOSE_DURATION property is missing DURATION element."));
			return false;
		}
		
		exposureIN->setValue(exposeElem->value);
	}
	
	return true;
}

bool imagesequence::setupFilters()
{
	bool filterDeviceFound(false);
	INDI_P *filterProp;
	INDIMenu *devMenu = ksw->getINDIMenu();
	if (devMenu == NULL)
			return false;
	
	filterCombo->clear();
	filterPosCombo->clear();
	
	filterCombo->addItem(i18n("None"));
	
	// Second step is to check for filter wheel, it is only optional.
	for (int i=0; i < devMenu->mgr.size(); i++)
	{
		for (int j=0; j < devMenu->mgr.at(i)->indi_dev.size(); j++)
		{
  		filterProp = devMenu->mgr.at(i)->indi_dev.at(j)->findProp("FILTER_SLOT");
			if (!filterProp)
				continue;

			filterDeviceFound = true;
		 
			if (devMenu->mgr.at(i)->indi_dev.at(j)->label.isEmpty())
				devMenu->mgr.at(i)->indi_dev.at(j)->label = 
					devMenu->mgr.at(i)->indi_dev.at(j)->name;
				
			filterCombo->addItem(devMenu->mgr.at(i)->indi_dev.at(j)->label);
			
		}
	}

	// If we found device, populate filters combo with aliases assigned to filter numbers
	// In Configure INDI
	if (filterDeviceFound)
	{
		filterCombo->setCurrentIndex(lastFilter);
		currentFilter = filterCombo->currentText();
		updateFilterCombo(lastFilter);
		return true;
	}
	else
	return false;
}

void imagesequence::resetButtons()
{
	startB->setEnabled(true);
	stopB->setEnabled(false);
}

void imagesequence::startSequence()
{
	if (active)
		stopSequence();

	// Let's find out which device has been selected and that it's connected.
	if (!verifyCCDIntegrity())
		return;

	// Get expose paramater
	active = true;
	ISOStamp = ISOCheck->isChecked() ? true : false;
	seqExpose = exposureIN->value();
	seqTotalCount = countIN->value();
	seqCurrentCount = 0;
	seqDelay = delayIN->value() * 1000;		/* in ms */
	currentCCD = CCDCombo->currentText();
	lastCCD = CCDCombo->currentIndex();
	currentFilter = filterCombo->currentText();
	lastFilter = filterCombo->currentIndex();

	fullImgCountOUT->setText( QString::number(seqTotalCount));
	currentImgCountOUT->setText(QString::number(seqCurrentCount));

	// Ok, now let's connect signals and slots for this device
	connect(stdDevCCD, SIGNAL(FITSReceived(QString)), this, SLOT(newFITS(const QString&)));

	// set the progress info
	imgProgress->setEnabled(true);
	imgProgress->setMaximum(seqTotalCount);
	imgProgress->setValue(seqCurrentCount);

	stdDevCCD->batchMode    = true;
	stdDevCCD->ISOMode      = ISOStamp;
	// Set this LAST
	stdDevCCD->updateSequencePrefix(prefixIN->text());

	// Update button status
	startB->setEnabled(false);
	stopB->setEnabled(true);

	prepareCapture();
}

void imagesequence::stopSequence()
{
	retries              = 0;
	seqTotalCount        = 0;
	seqCurrentCount      = 0;
	active               = false;
	
	imgProgress->setEnabled(false);
	fullImgCountOUT->setText(QString());
	currentImgCountOUT->setText(QString());
	
	resetButtons();
	seqTimer->stop();
	
	if (stdDevCCD)
	{
		stdDevCCD->seqCount     = 0;
		stdDevCCD->batchMode    = false;
		stdDevCCD->ISOMode      = false;
		
		stdDevCCD->disconnect( SIGNAL(FITSReceived(QString)));
	}
}

void imagesequence::checkCCD(int ccdNum)
{
	INDI_D *idevice = NULL;
	QString targetCCD = CCDCombo->itemText(ccdNum);
	
	INDIMenu *imenu = ksw->getINDIMenu();
	if (!imenu)
	{
		KMessageBox::error(this, i18n("INDI Menu has not been initialized properly. Restart KStars."));
		return;
	}

	idevice = imenu->findDeviceByLabel(targetCCD);

	if (!idevice)
	{
		KMessageBox::error(this, i18n("INDI device %1 no longer exists.", targetCCD));
		CCDCombo->removeItem(ccdNum);
		lastCCD = CCDCombo->currentIndex();
		if (lastCCD != -1)
			checkCCD(lastCCD);
		return;
  }

	if (!idevice->isOn())
	{
		KMessageBox::error(this, i18n("%1 is disconnected. Establish a connection to the device using the INDI Control Panel.", targetCCD));

		CCDCombo->setCurrentIndex(lastCCD);
		return;
  }

	currentCCD = targetCCD;
}

void imagesequence::newFITS(const QString &deviceLabel)
{
	// If the FITS is not for our device, simply ignore
	if (deviceLabel != currentCCD)
	return;

	seqCurrentCount++;
	imgProgress->setValue(seqCurrentCount);

	currentImgCountOUT->setText( QString::number(seqCurrentCount));

	// if we're done
	if (seqCurrentCount == seqTotalCount)
	{
		stdDevCCD->batchMode    = false;
		stdDevCCD->ISOMode      = false;
		retries              = 0;
		seqTotalCount        = 0;
		seqCurrentCount      = 0;
		active               = false;
		seqTimer->stop();

		if (stdDevCCD)
			stdDevCCD->disconnect( SIGNAL(FITSReceived(QString)));

		resetButtons();
	}
	else
		seqTimer->start(seqDelay);
}


bool imagesequence::verifyCCDIntegrity()
{
	QString targetCCD;
	INDI_D *idevice = NULL;
	INDI_P *exposeProp;
	INDI_E *exposeElem;
	stdDevCCD = NULL;

	INDIMenu *imenu = ksw->getINDIMenu();
	if (!imenu)
	{
		KMessageBox::error(this, i18n("INDI Menu has not been initialized properly. Restart KStars."));
		return false;
	}

	targetCCD = CCDCombo->currentText();

	if (targetCCD.isEmpty())
		return false;

	// #2 Check if the device exists
	idevice = imenu->findDeviceByLabel(targetCCD);

	if (!idevice)
	{
		KMessageBox::error(this, i18n("INDI device %1 no longer exists.", targetCCD));
		CCDCombo->removeItem(CCDCombo->currentIndex());
		lastCCD = CCDCombo->currentIndex();
		return false;
	}

	if (!idevice->isOn())
	{
		KMessageBox::error(this, i18n("%1 is disconnected. Establish a connection to the device using the INDI Control Panel.", currentCCD));
		return false;
	}

	stdDevCCD = idevice->stdDev;
	exposeProp = stdDevCCD->dp->findProp("CCD_EXPOSE_DURATION");
	if (!exposeProp)
	{
		KMessageBox::error(this, i18n("Device does not support CCD_EXPOSE_DURATION property."));
		return false;
	}

	exposeElem = exposeProp->findElement("EXPOSE_DURATION");
	if (!exposeElem)
	{
		KMessageBox::error(this, i18n("CCD_EXPOSE_DURATION property is missing DURATION element."));
		return false;
	}

	return true;
}

bool imagesequence::verifyFilterIntegrity()
{
	QString targetFilter;
	INDIMenu *devMenu = ksw->getINDIMenu();
	INDI_D *filterDevice (NULL);
	INDI_E *filterElem(NULL);
	if (devMenu == NULL)
			return false;

	targetFilter  = filterCombo->currentText();

	if (targetFilter.isEmpty() || targetFilter == i18n("None"))
	{
		filterPosCombo->clear();
		return false;
	}

	// #1 Check the device exists
	filterDevice = devMenu->findDeviceByLabel(targetFilter);
	if (filterDevice == NULL)
	{
		KMessageBox::error(this, i18n("INDI device %1 no longer exists.", targetFilter));
		filterCombo->removeItem(filterCombo->currentIndex());
		filterCombo->setCurrentIndex(0);
		currentFilter = filterCombo->currentText();
		filterPosCombo->clear();
		stdDevFilter = NULL;
		return false;
	}

	// #2 Make sure it's connected
	if (!filterDevice->isOn())
	{
		KMessageBox::error(this, i18n("%1 is disconnected. Establish a connection to the device using the INDI Control Panel.", targetFilter));
		filterCombo->setCurrentIndex(0);
		currentFilter = filterCombo->currentText();
		filterPosCombo->clear();
		stdDevFilter = NULL;
		return false;
	}

	// #3 Make sure it has FILTER_SLOT std property by searching for its SLOT element
	filterElem = filterDevice->findElem("SLOT");
	if (filterElem == NULL)
	{
	 	KMessageBox::error(this, i18n("Device does not support FILTER_SLOT property."));
		filterCombo->setCurrentIndex(0);
		currentFilter = filterCombo->currentText();
		filterPosCombo->clear();
		stdDevFilter = NULL;
		return false;
	}

	stdDevFilter  = filterDevice->stdDev;
	lastFilter    = filterCombo->currentIndex();
	currentFilter = targetFilter;

	// We're good
	return true;
}

void imagesequence::prepareCapture()
{
	INDI_P * tempProp(NULL);

	// Do we need to select filter First??
	if (currentFilter.isEmpty() || currentFilter == i18n("None"))
	captureImage();
	else
	{
		if (!verifyFilterIntegrity())
		{
			stopSequence();
			return;
		}

		if ( stdDevFilter && ((tempProp = stdDevFilter->dp->findProp("FILTER_SLOT")) != NULL))
		{
			connect (tempProp, SIGNAL(okState()), this, SLOT(captureImage()));
			selectFilter();
		}
		else
			kDebug() << "Error: std Filter device lost or missing FILTER_SLOT property" << endl;
	}
}
  
void imagesequence::captureImage()
{
	INDI_P * exposeProp(NULL);
	INDI_E * exposeElem(NULL);
	INDI_P * tempProp(NULL);

	// Let's capture a new frame in acoord with the settings
	// We need to take into consideration the following conditions:
	// A. The device has been disconnected.
	// B. The device has been lost.
	// C. The property is still busy.
	// D. The property has been lost.

	if ( stdDevFilter && ((tempProp = stdDevFilter->dp->findProp("FILTER_SLOT")) != NULL))
		tempProp->disconnect( SIGNAL (okState()));

	if (!verifyCCDIntegrity())
	{
		stopSequence();
		return;
	}

	exposeProp = stdDevCCD->dp->findProp("CCD_EXPOSE_DURATION");
	exposeElem = exposeProp->findElement("EXPOSE_DURATION");

	// disable timer until it's called again by newFITS, or called for retries
	seqTimer->stop();

	// Make sure it's not busy, if it is then schedual.
	if (exposeProp->state == PS_BUSY)
	{
		retries++;

		if (retries > RETRY_MAX)
		{
			seqTimer->stop();
			KMessageBox::error(this, i18n("Device is busy and not responding."));
			stopSequence();
			retries = 0;
			return;
		}

		seqTimer->start(RETRY_PERIOD);
	}

	// Set duration if applicable. We check the property permission, min, and max values
	if (exposeProp->perm == PP_RW || exposeProp->perm == PP_WO)
	{
		if (seqExpose < exposeElem->min || seqExpose > exposeElem->max)
		{
			stopSequence();
			KMessageBox::error(this, i18n("Expose duration is invalid. %1 supports expose durations from %2 to %3 seconds only.", currentCCD, exposeElem->min, exposeElem->max));
			return;
		}

		// we're okay, set it
		exposeElem->targetValue = seqExpose;
		if (exposeElem->spin_w)
		{
			exposeElem->spin_w->setValue(seqExpose);
			exposeElem->spinChanged(seqExpose);
		}
		else
			exposeElem->write_w->setText( QString::number(seqExpose));

	}

	// We're done! Send it to the driver
	exposeProp->newText();

}

void imagesequence::updateFilterCombo(int filterNum)
{
	INDIMenu *devMenu = ksw->getINDIMenu();
	QStringList filterList;
	INDI_E *filterElem;
	int filterMax;

	if (!verifyFilterIntegrity())
		return;

	filterList = Options::filterAlias();

	filterElem = devMenu->findDeviceByLabel(filterCombo->itemText(filterNum))->findElem("SLOT");
        Q_ASSERT(filterElem);

	filterMax = (int) filterElem->max; 

	// Clear combo
	filterPosCombo->clear();

	if (filterList.empty())
		for (int i=0; i <= filterMax; i++)
			filterList << QString::number(i);

	// Fill filter combo
	if (filterList.size() <= filterMax)
	{
		filterPosCombo->addItems(filterList);
		for (int i = filterList.size() ; i <= filterMax ; i++)
			filterPosCombo->addItem(QString::number(i));
	}
	else 
	{
		// filterMax < filterList.size()
		for (int i = 0 ; i <= filterMax ; i++)
			filterPosCombo->addItem(filterList[i]);
	}

	filterPosCombo->setCurrentIndex(((int) filterElem->value));
}

void imagesequence::selectFilter()
{
	INDI_P * filterProp(NULL);
	INDI_E * filterElem(NULL);
	INDI_D * filterDev(NULL);
	INDIMenu *devMenu = ksw->getINDIMenu();

	// Let's select a new filter in acoord with the settings
	// We need to take into consideration the following conditions:
	// A. The device has been disconnected.
	// B. The device has been lost.
	// C. The property is still busy.
	// D. The property has been lost.
	
	// We have a filter, let's check if it's valid
	if (!verifyFilterIntegrity())
		return;

	filterDev = devMenu->findDeviceByLabel(currentFilter);
	filterProp = filterDev->findProp("FILTER_SLOT");
	filterElem = filterProp->findElement("SLOT");

	// Do we need to change the filter position??
	if (filterPosCombo->currentIndex() == filterElem->read_w->text().toInt())
	{
		captureImage();
		return;
	}

	if (filterProp->perm == PP_RW || filterProp->perm == PP_WO)
	{
		filterElem->targetValue = filterPosCombo->currentIndex();
		if (filterElem->spin_w)
		{
			filterElem->spin_w->setValue(filterElem->targetValue);
			filterElem->spinChanged(filterElem->targetValue);
		}
		else
			filterElem->write_w->setText(QString::number(filterElem->targetValue));

		// We're done! Send it to the driver
		filterProp->newText();
	}
}

#include "imagesequence.moc"

