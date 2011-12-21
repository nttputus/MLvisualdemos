/*********************************************************************
MLDemos: A User-Friendly visualization toolkit for machine learning
Copyright (C) 2010  Basilio Noris
Contact: mldemos@b4silio.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public License,
version 3 as published by the Free Software Foundation.

This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free
Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*********************************************************************/
#include "interfaceProjections.h"
#include <QPixmap>
#include <QBitmap>
#include <QPainter>
#include <QDebug>

using namespace std;

ClassProjections::ClassProjections()
{
	params = new Ui::ParametersProjections();
	params->setupUi(widget = new QWidget());
	projectionWindow = NULL;
	canvas = NULL;
	classifier = NULL;
	classifierType = 0;
	bDataIsFromCanvas = false;
	connect(params->projectionButton, SIGNAL(clicked()), this, SLOT(ShowProjection()));
	connect(params->toCanvasButton, SIGNAL(clicked()), this, SLOT(SendToCanvas()));
}

void ClassProjections::SetParams(Classifier *classifier)
{
	if(!classifier) return;
	int type = params->linearTypeCombo->currentIndex();
	classifierType = type;
	if(type != 4) ((ClassifierLinear *)classifier)->SetParams(type);
	else
	{
		int kernelType = params->kernelTypeCombo->currentIndex();
		float kernelWidth = params->kernelWidthSpin->value();
		int kernelDegree = params->kernelDegSpin->value();
		((ClassifierKPCA *)classifier)->SetParams(kernelType, kernelDegree, kernelWidth);
	}
}

QString ClassProjections::GetAlgoString()
{
	int type = params->linearTypeCombo->currentIndex();
	switch(type)
	{
	case 0:
		return "PCA";
		break;
	case 1:
		return "LDA";
		break;
	case 2:
		return "Fisher-LDA";
		break;
	case 3:
		return "ICA";
		break;
	case 4:
		return "Kernel PCA";
		break;
	case 5:
		return "Naive Bayes";
		break;
	}
}

Classifier *ClassProjections::GetClassifier()
{
	int type = params->linearTypeCombo->currentIndex();
	if(type == 4)
	{
		classifier = new ClassifierKPCA();
	}
	else{
		classifier = new ClassifierLinear();
	}
	SetParams(classifier);
	return classifier;
}

void ClassProjections::DrawInfo(Canvas *canvas, QPainter &painter, Classifier *classifier)
{
	if(!canvas || !classifier) return;
	if(!this->canvas) this->canvas = canvas;
	painter.setRenderHint(QPainter::Antialiasing);
	ClassifierLinear *linear = (ClassifierLinear*)classifier;
	int xIndex = canvas->xIndex, yIndex = canvas->yIndex;
	fvec mean = linear->GetMean();
	fVec m(mean[xIndex],mean[yIndex]);
	if(linear->GetType()==3) // ICA
	{
	}
	else if(linear->GetType() < 3) // PCA, LDA, Fisher
	{
		fvec pt[5];
		QPointF point[4];
		FOR(i,5) pt[i].resize(2,0);
		pt[0][0]=1;pt[0][1]=0;
		pt[1][0]=-1;pt[1][1]=0;
		pt[2][0]=0;pt[2][1]=0;
		FOR(i, 3) pt[i] = linear->Project(pt[i]);
		point[0] = canvas->toCanvasCoords(pt[0]);
		point[1] = canvas->toCanvasCoords(pt[1]);
		painter.setPen(QPen(Qt::black, 2));
		painter.drawLine(point[0], point[1]);
	}
	if(projectionWindow && projectionWindow->isVisible()) ShowProjection();
}

void ClassProjections::DrawModel(Canvas *canvas, QPainter &painter, Classifier *classifier)
{
	if(!classifier || !canvas) return;
	if(!this->canvas) this->canvas = canvas;
	painter.setRenderHint(QPainter::Antialiasing, true);

	int posClass = 1;
	bool bUseMinMax = true;
	float resMin = FLT_MAX;
	float resMax = -FLT_MAX;
	if(bUseMinMax)
	{
		// TODO: get the min and max for all samples
		std::vector<fvec> samples = canvas->data->GetSamples();
		FOR(i, samples.size())
		{
			float val = classifier->Test(samples[i]);
			if(val > resMax) resMax = val;
			if(val < resMin) resMin = val;
		}
		if(resMin == resMax) resMin -= 3;
	}

	FOR(i, canvas->data->GetCount())
	{
		fvec sample = canvas->data->GetSample(i);
		int label = canvas->data->GetLabel(i);
		QPointF point = canvas->toCanvasCoords(canvas->data->GetSample(i));
		float response = classifier->Test(sample);
		if(response > 0)
		{
			if(label == posClass) Canvas::drawSample(painter, point, 9, 1);
			else Canvas::drawCross(painter, point, 6, 2);
		}
		else
		{
			if(label != posClass) Canvas::drawSample(painter, point, 9, 0);
			else Canvas::drawCross(painter, point, 6, 0);
		}
	}
}

void ClassProjections::ShowProjection()
{
	//if(projectionWindow && projectionWindow->isVisible())
	//{
	//	projectionWindow->hide();
	//	return;
	//}
	if(!classifier || !canvas) return;
	// we project all the data into a new image
	int w = canvas->width()/2;
	if(classifierType == 3 || classifierType == 4) w = canvas->width();
	int h = canvas->height()/2;
	QPixmap projectionPixmap(w, h);
	projectionPixmap.fill();
	vector<fvec> samples = canvas->data->GetSamples();
	if(!samples.size()) return;
	ivec labels = canvas->data->GetLabels();

	QPainter painter(&projectionPixmap);
	painter.setRenderHint(QPainter::Antialiasing, true);

	if(classifierType < 3) // PCA, LDA, Fisher
	{
		fvec pt[5];
		QPointF point[4];
		FOR(i,5) pt[i].resize(2,0);
		pt[0][0]=1;pt[0][1]=0;
		pt[1][0]=-1;pt[1][1]=0;
		pt[2][0]=0;pt[2][1]=0;

		FOR(i, 3)
		{
			pt[i] = ((ClassifierLinear *)classifier)->Project(pt[i]);
		}

		point[0] = canvas->toCanvasCoords(pt[0])*0.5f;
		point[1] = canvas->toCanvasCoords(pt[1])*0.5f;
		painter.setPen(QPen(Qt::black, 2));
		painter.drawLine(point[0], point[1]);

		// draw lines
		FOR(i, samples.size())
		{
			fvec sample = samples[i];
			int label = labels[i];
			fvec newSample = ((ClassifierLinear *)classifier)->Project(sample);

			QPointF point = canvas->toCanvasCoords(newSample)/canvas->width()*w;
			QPointF original = canvas->toCanvasCoords(samples[i])/canvas->width()*w;
			painter.setPen(QPen(QColor(0,0,0,128), 0.2));
			painter.drawLine(original, point);
		}

		// draw samples
		FOR(i, samples.size())
		{
			fvec sample = samples[i];
			int label = labels[i];
			fvec newSample = ((ClassifierLinear *)classifier)->Project(sample);

			QPointF point = canvas->toCanvasCoords(newSample)/canvas->width()*w;
			QPointF original = canvas->toCanvasCoords(samples[i])/canvas->width()*w;
			Canvas::drawSample(painter, point, 6, label);
			Canvas::drawSample(painter, original, 6, label);
		}
	}
	if(classifierType == 3) // ICA
	{
		fvec meanAll; meanAll.resize(samples[0].size(), 0);
		FOR(i, samples.size())
		{
			meanAll += samples[i];
		}
		meanAll /= samples.size();

		vector<fvec> projected(samples.size());
		FOR(i, samples.size()) projected[i] = ((ClassifierLinear *)classifier)->Project(samples[i]);
		// draw lines
		FOR(i, samples.size())
		{
			fvec& sample = samples[i];
			int& label = labels[i];
			fvec& newSample = projected[i];

			QPointF point = canvas->toCanvasCoords(newSample)/canvas->width()*w*0.5 + QPointF(w*0.5, 0);
			QPointF original = canvas->toCanvasCoords(samples[i]-meanAll)/canvas->width()*w*0.5;
			painter.setPen(QPen(QColor(0,0,0,40), 0.2));
			painter.drawLine(original, point);
		}

		// draw samples
		FOR(i, samples.size())
		{
			fvec sample = samples[i];
			int label = labels[i];
			fvec newSample = projected[i];

			QPointF point = canvas->toCanvasCoords(newSample)/canvas->width()*w*0.5 + QPointF(w*0.5, 0);
			QPointF original = canvas->toCanvasCoords(samples[i]-meanAll)/canvas->width()*w*0.5;
			Canvas::drawSample(painter, point, 6, label);
			Canvas::drawSample(painter, original, 6, label);
		}

		painter.setPen(QPen(Qt::black, 2.f));
		painter.drawLine(QPointF(w*0.5f, 0), QPointF(w*0.5f, h));
	}
	if(classifierType == 4) // KPCA
	{
		ClassifierKPCA *kpca = (ClassifierKPCA*)classifier;

		vector<fvec> results = kpca->GetResults();
		ivec labels = kpca->GetLabels();

		vector<fvec> samples = kpca->GetSamples();

		fvec meanAll(samples[0].size(), 0);
		FOR(i, samples.size())
		{
			meanAll += samples[i];
		}
		meanAll /= samples.size();

		// draw lines
		FOR(i, samples.size())
		{
			fvec sample = samples[i];
			int label = labels[i];
			fvec newSample = results[i];

			QPointF point = canvas->toCanvasCoords(newSample)/canvas->width()*w*0.5 + QPointF(w/4, -h*0.5);
			QPointF original = canvas->toCanvasCoords(samples[i]-meanAll)/canvas->width()*w*0.5;
			painter.setPen(QPen(QColor(0,0,0,40), 0.2));
			painter.drawLine(original, point);
		}

		// draw samples
		FOR(i, samples.size())
		{
			fvec sample = samples[i];
			int label = labels[i];
			fvec newSample = results[i];

			QPointF point = canvas->toCanvasCoords(newSample)/canvas->width()*w*0.5 + QPointF(w/4, -h*0.5);
			QPointF original = canvas->toCanvasCoords(samples[i]-meanAll)/canvas->width()*w*0.5;
			Canvas::drawSample(painter, point, 6, label);
			Canvas::drawSample(painter, original, 6, label);
		}

		painter.setPen(QPen(Qt::black, 2.f));
		painter.drawLine(QPointF(w*0.5f, 0), QPointF(w*0.5f, h));
	}

	if(!projectionWindow) projectionWindow = new QLabel("Projections");
	projectionWindow->resize(projectionPixmap.size());
	projectionWindow->setPixmap(projectionPixmap);
	projectionWindow->show();
}

void ClassProjections::SendToCanvas()
{
	if(!canvas) return;

	if(bDataIsFromCanvas)
	{
		FOR(i, data.size()) canvas->data->SetSample(i, data[i]);
		params->toCanvasButton->setText("Set Projection");
		bDataIsFromCanvas = false;
	}
	else
	{
		if(!classifier) return;
		if(!dynamic_cast<ClassifierLinear*>(classifier) && !dynamic_cast<ClassifierKPCA*>(classifier))
		{
			data.clear();
			return;
		}
		data = canvas->data->GetSamples();
		if(classifierType == 4)
		{
			vector<fvec> projected = ((ClassifierKPCA *)classifier)->GetResults();
			FOR(i, projected.size())
			{
				canvas->data->SetSample(i, projected[i]);
			}
		}
		else
		{
			FOR(i, data.size())
			{
				fvec projected = ((ClassifierLinear *)classifier)->Project(data[i]);
				canvas->data->SetSample(i, projected);
			}
		}
		bDataIsFromCanvas = true;
		params->toCanvasButton->setText("Set Original");
	}
	canvas->modelPixmap = QPixmap();
	canvas->confidencePixmap = QPixmap();
	canvas->infoPixmap = QPixmap();
	canvas->ResetSamples();
	canvas->repaint();
}

void ClassProjections::SaveOptions(QSettings &settings)
{
	settings.setValue("linearType", params->linearTypeCombo->currentIndex());
	settings.setValue("kernelDeg", params->kernelDegSpin->value());
	settings.setValue("kernelType", params->kernelTypeCombo->currentIndex());
	settings.setValue("kernelWidth", params->kernelWidthSpin->value());
}

bool ClassProjections::LoadOptions(QSettings &settings)
{
	if(settings.contains("linearType")) params->linearTypeCombo->setCurrentIndex(settings.value("linearType").toInt());
	if(settings.contains("kernelDeg")) params->kernelDegSpin->setValue(settings.value("kernelDeg").toFloat());
	if(settings.contains("kernelType")) params->kernelTypeCombo->setCurrentIndex(settings.value("kernelType").toInt());
	if(settings.contains("kernelWidth")) params->kernelWidthSpin->setValue(settings.value("kernelWidth").toFloat());
	return true;
}

void ClassProjections::SaveParams(QTextStream &file)
{
	file << "classificationOptions" << ":" << "linearType" << " " << params->linearTypeCombo->currentIndex() << "\n";
	file << "classificationOptions" << ":" << "kernelDeg" << " " << params->kernelDegSpin->value() << "\n";
	file << "classificationOptions" << ":" << "kernelType" << " " << params->kernelTypeCombo->currentIndex() << "\n";
	file << "classificationOptions" << ":" << "kernelWidth" << " " << params->kernelWidthSpin->value() << "\n";
}

bool ClassProjections::LoadParams(QString name, float value)
{
	if(name.endsWith("linearType")) params->linearTypeCombo->setCurrentIndex((int)value);
	if(name.endsWith("kernelDeg")) params->kernelDegSpin->setValue((int)value);
	if(name.endsWith("kernelType")) params->kernelTypeCombo->setCurrentIndex((int)value);
	if(name.endsWith("kernelWidth")) params->kernelWidthSpin->setValue(value);
	return true;
}
