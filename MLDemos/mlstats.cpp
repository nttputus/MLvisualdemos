/*********************************************************************
MLDemos: A User-Friendly visualization toolkit for machine learning
Copyright (C) 2010  Basilio Noris
Contact: mldemos@b4silio.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free
Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*********************************************************************/
#include "mldemos.h"
#include "basicMath.h"
#include "classifier.h"
#include "regressor.h"
#include "clusterer.h"
#include <QDebug>
#include <fstream>
#include <QPixmap>
#include <QBitmap>
#include <QSettings>
#include <QFileDialog>
#include <vector>
#include <algorithm>

using namespace std;

void MLDemos::MouseOnRoc(QMouseEvent *event)
{
	int e;
	switch( event->button())
	{
	case Qt::LeftButton:
		e = CV_EVENT_LBUTTONUP;
		break;
	case Qt::RightButton:
		e = CV_EVENT_RBUTTONUP;
		break;
	}
	//roc_on_mouse(e, event->x(), event->y(), 0, 0);
	//rocWidget->ShowImage(GetRocImage());
	//statsDialog->repaint();
}

void MLDemos::ShowRoc()
{
	if(!classifier) return;
	SetROCInfo();
	actionShowStats->setChecked(true);
	showStats->tabWidget->setCurrentWidget(showStats->rocTab);
	ShowStatsDialog();
}

void MLDemos::ShowCross()
{
	if(!classifier && !regressor) return;
	SetCrossValidationInfo();
	actionShowStats->setChecked(true);
	showStats->tabWidget->setCurrentWidget(showStats->crossvalidTab);
	ShowStatsDialog();
}

void MLDemos::StatsChanged()
{
	int tab = showStats->tabWidget->currentIndex();
	switch(tab)
	{
	case 0:
		UpdateInfo();
		break;
	case 1:
		SetROCInfo();
		break;
	case 2:
		SetCrossValidationInfo();
		break;
	}
}

void PaintData(std::vector<float> data, QPixmap &pm)
{
	QPainter painter(&pm);
	painter.fillRect(pm.rect(), Qt::white);

	int w = pm.width();
	int h = pm.height();
	int cnt = data.size();
	int pad = 10;
	QPointF oldPoint;
	double minVal = FLT_MAX;
	double maxVal = -FLT_MAX;
	for(int i=0; i< data.size(); i++)
	{
		if(minVal > data[i]) minVal = data[i];
		if(maxVal < data[i]) maxVal = data[i];
	}
	if (minVal == maxVal)
	{
		minVal = 0;
	}

	painter.setBrush(Qt::NoBrush);
	painter.setPen(QPen(QColor(200,200,200), 0.5));
	int steps = 10;
	for(int i=0; i<=steps; i++)
	{
		painter.drawLine(QPoint(0, i/(float)steps*(h-2*pad) + pad), QPoint(w, i/(float)steps*(h-2*pad) + pad));
		painter.drawLine(QPoint(i/(float)steps*w, 0), QPoint(i/(float)steps*w, h));
	}
	painter.setRenderHint(QPainter::Antialiasing);

	painter.setPen(QPen(Qt::black, 1.5));
	for(int i=0; i< data.size(); i++)
	{
		float value = data[i];
		if (value != value) continue;
		float x = i/(float)cnt*w;
		float y = (1 - (value-minVal)/(maxVal - minVal)) * (float)(h-2*pad) + pad;
		QPointF point(x, y);
		if(i) painter.drawLine(oldPoint, point);
		//painter.drawEllipse(point, 3, 3);
		oldPoint = point;
	}
	painter.setPen(QPen(Qt::black, 0.5));
	painter.setBrush(QColor(255,255,255,200));
	painter.drawRect(QRect(w - 100 - 15,h - 55,110,45));
	painter.setPen(QPen(Qt::black, 1));
	painter.drawText(QPointF(w - 107, h-57+20), QString("start: %1").arg(data[0], 3));
	painter.drawText(QPointF(w - 107, h-57+40), QString("end: %1").arg(data[data.size()-1], 3));
	}

void MLDemos::SetROCInfo()
{
	QSize size(showStats->rocWidget->width(),showStats->rocWidget->height());
	if(classifier && bIsRocNew)
	{
		QPixmap rocImage = RocImage(classifier->rocdata, classifier->roclabels, size);
		bIsRocNew = false;
	//	rocImage.save("roc.png");
		rocWidget->ShowImage(rocImage);
	}
	if(maximizer)
	{
		vector<double> history = maximizer->HistoryValue();
		vector<float> data;data.resize(history.size());
		FOR(i, data.size()) data[i] = history[i];
		if(!data.size()) return;
		QPixmap pixmap(size);
		PaintData(data, pixmap);
		rocWidget->ShowImage(pixmap);
	}
}

void MLDemos::SetCrossValidationInfo()
{
	if(!bIsCrossNew) return;
	std::vector<fvec> fmeasures;
	if(classifier) fmeasures = classifier->crossval;
	else if(regressor) fmeasures = regressor->crossval;

	if(!fmeasures.size()) return;
	char txt[255];
	QString text;
	text += "Cross-Validation\n";
	float ratios [] = {.1f,.25f,1.f/3.f,.5f,2.f/3.f,.75f,.9f,1.f};
	int ratioIndex = classifier ? optionsClassify->traintestRatioCombo->currentIndex() : optionsRegress->traintestRatioCombo->currentIndex();
	float trainRatio = ratios[ratioIndex];
	if(classifier) sprintf(txt, "%d folds\n", optionsClassify->foldCountSpin->value());
	else sprintf(txt, "%d folds\n", optionsRegress->foldCountSpin->value());
	text += txt;
	sprintf(txt,"%d train, %d test samples", (int)(canvas->data->GetCount()*trainRatio), canvas->data->GetCount() - (int)(canvas->data->GetCount()*trainRatio));
	text += txt + QString("\n\n");
	text += classifier ? QString("Classification Performance:\n\n") : QString("Regression Error:\n\n");
	FOR(i, fmeasures.size())
	{
		fvec meanStd = MeanStd(fmeasures[i]);
		fvec quartiles = Quartiles(fmeasures[i]);
		text += !i ? "Training\n" : "Testing\n";
		sprintf(txt,"%.3f � %.3f", meanStd[0], meanStd[1]);
		text += txt + QString(" (mean�std)\n");
		sprintf(txt,"%.3f %.3f %.3f %.3f %.3f", quartiles[0], quartiles[1], quartiles[2], quartiles[3], quartiles[4]);
		text += txt + QString(" (quartiles)\n");
		text += "\n\n";
	}
	showStats->crossvalidText->setText(text);
	QPixmap boxplot = BoxPlot(fmeasures, QSize(showStats->crossvalidWidget->width(),showStats->crossvalidWidget->height()));
//	boxplot.save("boxplot.png");
	bIsCrossNew = false;
	crossvalidWidget->ShowImage(boxplot);
}


void MLDemos::UpdateInfo()
{
	// dataset information
	int count = canvas->data->GetCount();
	int pcount = 0, ncount = 0;
	ivec labels = canvas->data->GetLabels();
	int posClass = optionsClassify->positiveSpin->value();
	FOR(i, labels.size())
	{
		if(labels[i] == posClass) ++pcount;
		else ++ncount;
	}

	// min/max, mean/variance
	vector<fvec> samples = canvas->data->GetSamples();
	fvec sMin,sMax,sMean,sSigma;
	sMin.resize(2,FLT_MAX);
	sMax.resize(2,-FLT_MAX);
	sMean.resize(2,0);
	sSigma.resize(4,0);
	if(samples.size())
	{
		FOR(i,samples.size())
		{
			sMin[0] = min(sMin[0],samples[i][0]);
			sMin[1] = min(sMin[1],samples[i][1]);
			sMax[0] = max(sMax[0],samples[i][0]);
			sMax[1] = max(sMax[1],samples[i][1]);
			sMean += samples[i];
		}
		sMean /= samples.size();
		FOR(i, samples.size())
		{
			sSigma[0] += (samples[i][0]-sMean[0])*(samples[i][0]-sMean[0]);
			sSigma[1] += (samples[i][0]-sMean[0])*(samples[i][1]-sMean[1]);
			sSigma[3] += (samples[i][1]-sMean[1])*(samples[i][1]-sMean[1]);
		}
		sSigma[0] = sqrtf(sSigma[0]/samples.size());
		sSigma[1] = sqrtf(sSigma[1]/samples.size());
		if(sSigma[1] != sSigma[1]) sSigma[1] = 0;
		sSigma[2] = sSigma[1];
		sSigma[3] = sqrtf(sSigma[3]/samples.size());
	}
	else
	{
		sMin.clear();sMax.clear();
		sMin.resize(2,0);
		sMax.resize(2,0);
	}

	QString information;
	information += "Current Dataset:\n";
	char string[255];
	sprintf(string, "    %d Samples\n    %d Positives\n    %d Negatives\n\n", count, pcount, ncount);
	information += QString(string);
	information +=    "       Min - Max          Mean  ,    Var\n";
	sprintf(string,   "    %.3f    %.3f      %.3f   ,   %.3f  %.3f\n", sMin[0], sMax[0], sMean[0], sSigma[0], sSigma[1]);
	sprintf(string, "%s    %.3f    %.3f      %.3f   ,   %.3f  %.3f\n", string, sMin[1], sMax[1], sMean[1], sSigma[2], sSigma[3]);
	information += string;

	if(classifier) information += "\nClassifier: " + QString(classifier->GetInfoString());
	if(regressor)  information += "\nRegressor: "  + QString(regressor->GetInfoString());
	if(clusterer)  information += "\nClusterer: "  + QString(clusterer->GetInfoString());
	if(dynamical)  information += "\nDynamical: "  + QString(dynamical->GetInfoString());
	if(maximizer)  information += "\nMaximizer: "  + QString(maximizer->GetInfoString());

	showStats->infoText->setText(information);
}
