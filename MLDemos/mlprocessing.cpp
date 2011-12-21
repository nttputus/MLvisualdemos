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
#include "dynamical.h"
#include "clusterer.h"
#include "maximize.h"
#include "roc.h"
#include <QDebug>
#include <fstream>
#include <QPixmap>
#include <QBitmap>
#include <QSettings>
#include <QFileDialog>
#include <QProgressDialog>

using namespace std;

void MLDemos::Classify()
{
    if(!canvas || !canvas->data->GetCount()) return;
    drawTimer->Stop();
	drawTimer->Clear();
	mutex.lock();
    DEL(clusterer);
    DEL(regressor);
    DEL(dynamical);
    DEL(classifier);
	DEL(maximizer);
	int tab = optionsClassify->tabWidget->currentIndex();
    if(tab >= classifiers.size() || !classifiers[tab]) return;
    classifier = classifiers[tab]->GetClassifier();
    tabUsedForTraining = tab;
    float ratios [] = {.1f,.25f,1.f/3.f,.5f,2.f/3.f,.75f,.9f,1.f};
    int ratioIndex = optionsClassify->traintestRatioCombo->currentIndex();
    float trainRatio = ratios[ratioIndex];
    int positive = optionsClassify->positiveSpin->value();

    bool trained = Train(classifier, positive, trainRatio);
    if(trained)
    {
		classifiers[tab]->Draw(canvas, classifier);
		UpdateInfo();
		if(drawTimer && classifier->UsesDrawTimer())
        {
            drawTimer->classifier = &this->classifier;
            drawTimer->start(QThread::NormalPriority);
        }
	}
    else
    {
        mutex.unlock();
        Clear();
		mutex.lock();
		UpdateInfo();
	}
	mutex.unlock();
}


void MLDemos::ClassifyCross()
{
    if(!canvas || !canvas->data->GetCount()) return;
    drawTimer->Stop();
    QMutexLocker lock(&mutex);
    DEL(clusterer);
    DEL(regressor);
    DEL(dynamical);
    DEL(classifier);
	DEL(maximizer);
	int tab = optionsClassify->tabWidget->currentIndex();
    if(tab >= classifiers.size() || !classifiers[tab]) return;
    tabUsedForTraining = tab;

    float ratios [] = {.1f,.25f,1.f/3.f,.5f,2.f/3.f,.75f,.9f,1.f};
    int ratioIndex = optionsClassify->traintestRatioCombo->currentIndex();
    float trainRatio = ratios[ratioIndex];
    int positive = optionsClassify->positiveSpin->value();
    int foldCount = optionsClassify->foldCountSpin->value();

    vector<fvec> fmeasures;
    fmeasures.resize(2);
    bool trained = false;
    FOR(f,foldCount)
    {
        DEL(classifier);
        classifier = classifiers[tab]->GetClassifier();
        trained = Train(classifier, positive, trainRatio);
        if(!trained) break;
        if(classifier->rocdata.size()>0)
        {
            fmeasures[0].push_back(GetBestFMeasure(classifier->rocdata[0]));
        }
        if(classifier->rocdata.size()>1)
        {
            fmeasures[1].push_back(GetBestFMeasure(classifier->rocdata[1]));
        }
    }
    classifier->crossval = fmeasures;
    ShowCross();
    if(trained) classifiers[tab]->Draw(canvas, classifier);
    UpdateInfo();
}

void MLDemos::Regression()
{
    if(!canvas || !canvas->data->GetCount()) return;
    drawTimer->Stop();
	drawTimer->Clear();

    QMutexLocker lock(&mutex);
    DEL(clusterer);
    DEL(regressor);
    DEL(dynamical);
    DEL(classifier);
	DEL(maximizer);
	int tab = optionsRegress->tabWidget->currentIndex();
    if(tab >= regressors.size() || !regressors[tab]) return;
    regressor = regressors[tab]->GetRegressor();
    tabUsedForTraining = tab;

    float ratios [] = {.1f,.25f,1.f/3.f,.5f,2.f/3.f,.75f,.9f,1.f};
    int ratioIndex = optionsRegress->traintestRatioCombo->currentIndex();
    float trainRatio = ratios[ratioIndex];

    Train(regressor, trainRatio);
    regressors[tab]->Draw(canvas, regressor);
    UpdateInfo();
}

void MLDemos::RegressionCross()
{
    if(!canvas || !canvas->data->GetCount()) return;
    drawTimer->Stop();
	drawTimer->Clear();
	QMutexLocker lock(&mutex);
    DEL(clusterer);
    DEL(regressor);
    DEL(dynamical);
    DEL(classifier);
	DEL(maximizer);
	int tab = optionsRegress->tabWidget->currentIndex();
    if(tab >= regressors.size() || !regressors[tab]) return;
    regressor = regressors[tab]->GetRegressor();
    tabUsedForTraining = tab;

    float ratios [] = {.1f,.25f,1.f/3.f,.5f,2.f/3.f,.75f,.9f,1.f};
    int ratioIndex = optionsRegress->traintestRatioCombo->currentIndex();
    float trainRatio = ratios[ratioIndex];
    int foldCount = optionsRegress->foldCountSpin->value();

    vector<fvec> errors;
    errors.resize(2);
    bool trained = false;
    FOR(f,foldCount)
    {
        DEL(regressor);
        regressor = regressors[tab]->GetRegressor();
        Train(regressor, trainRatio);
        if(regressor->trainErrors.size())
        {
            errors[0] = regressor->trainErrors;
        }
        if(regressor->testErrors.size())
        {
            errors[1] = regressor->testErrors;
        }
    }
    regressor->crossval = errors;
    ShowCross();

	Train(regressor, trainRatio);
    regressors[tab]->Draw(canvas, regressor);
	UpdateInfo();
}

void MLDemos::Dynamize()
{
    if(!canvas || !canvas->data->GetCount() || !canvas->data->GetSequences().size()) return;
    drawTimer->Stop();
	drawTimer->Clear();
    QMutexLocker lock(&mutex);
    DEL(clusterer);
    DEL(regressor);
    DEL(dynamical);
    DEL(classifier);
	DEL(maximizer);
	int tab = optionsDynamic->tabWidget->currentIndex();
    if(tab >= dynamicals.size() || !dynamicals[tab]) return;
    dynamical = dynamicals[tab]->GetDynamical();
    tabUsedForTraining = tab;

    Train(dynamical);
    dynamicals[tab]->Draw(canvas,dynamical);

	int w = canvas->width(), h = canvas->height();

	int resampleType = optionsDynamic->resampleCombo->currentIndex();
	int resampleCount = optionsDynamic->resampleSpin->value();
	int centerType = optionsDynamic->centerCombo->currentIndex();
	float dT = optionsDynamic->dtSpin->value();
	int zeroEnding = optionsDynamic->zeroCheck->isChecked();
	bool bColorMap = optionsDynamic->colorCheck->isChecked();

	vector< vector<fvec> > trajectories = canvas->data->GetTrajectories(resampleType, resampleCount, centerType, dT, zeroEnding);
	if(trajectories.size())
	{
		canvas->modelPixmap = QPixmap(w,h);
		QBitmap bitmap(w,h);
		bitmap.clear();
		canvas->modelPixmap.setMask(bitmap);
		canvas->modelPixmap.fill(Qt::transparent);
		QPainter painter(&canvas->modelPixmap);
		int dim = trajectories[0][0].size() / 2;
		fvec start(dim,0);
		FOR(i, trajectories.size())
		{
			FOR(d, dim) start[d] = trajectories[i][0][d];
			vector<fvec> result = dynamical->Test(start, 1000);
			fvec oldPt = result[0];
			int count = result.size();
			FOR(j, count-1)
			{
				fvec pt = result[j+1];
				painter.setPen(QPen(Qt::green, 2));
				painter.drawLine(canvas->toCanvasCoords(pt), canvas->toCanvasCoords(oldPt));
				//if(j<count-2) Canvas::drawSample(painter, canvas->toCanvasCoords(pt), 5, 2);
				oldPt = pt;
			}
			painter.setBrush(Qt::NoBrush);
			painter.setPen(Qt::green);
			painter.drawEllipse(canvas->toCanvasCoords(result[0]), 5, 5);
			painter.setPen(Qt::red);
			painter.drawEllipse(canvas->toCanvasCoords(result[count-1]), 5, 5);
		}
	}

	// the first index is "none", so we subtract 1
    int avoidIndex = optionsDynamic->obstacleCombo->currentIndex()-1;
    if(avoidIndex >=0 && avoidIndex < avoiders.size() && avoiders[avoidIndex])
    {
        DEL(dynamical->avoid);
		dynamical->avoid = avoiders[avoidIndex]->GetObstacleAvoidance();
    }
	UpdateInfo();
	if(dynamicals[tab]->UsesDrawTimer())
	{
		drawTimer->bColorMap = bColorMap;
		drawTimer->start(QThread::NormalPriority);
	}
}

void MLDemos::Avoidance()
{
    if(!canvas || !dynamical) return;
    drawTimer->Stop();
    QMutexLocker lock(&mutex);
	// the first index is "none", so we subtract 1
	int index = optionsDynamic->obstacleCombo->currentIndex()-1;
    if(index >=0 && index >= avoiders.size() || !avoiders[index]) return;
    DEL(dynamical->avoid);
    dynamical->avoid = avoiders[index]->GetObstacleAvoidance();
	UpdateInfo();
	drawTimer->Clear();
    drawTimer->start(QThread::NormalPriority);
}

void MLDemos::Cluster()
{
    if(!canvas || !canvas->data->GetCount()) return;
    drawTimer->Stop();
    QMutexLocker lock(&mutex);
    DEL(clusterer);
    DEL(regressor);
    DEL(dynamical);
    DEL(classifier);
	DEL(maximizer);
	int tab = optionsCluster->tabWidget->currentIndex();
    if(tab >= clusterers.size() || !clusterers[tab]) return;
    clusterer = clusterers[tab]->GetClusterer();
    tabUsedForTraining = tab;
    Train(clusterer);
	drawTimer->Stop();
	drawTimer->Clear();
	clusterers[tab]->Draw(canvas,clusterer);

	UpdateInfo();
	drawTimer->clusterer= &this->clusterer;
	drawTimer->start(QThread::NormalPriority);
}

void MLDemos::ClusterIterate()
{
    if(!canvas || !canvas->data->GetCount()) return;
    drawTimer->Stop();
    int tab = optionsCluster->tabWidget->currentIndex();
    if(tab >= clusterers.size() || !clusterers[tab]) return;
    QMutexLocker lock(&mutex);
    if(!clusterer)
    {
        clusterer = clusterers[tab]->GetClusterer();
        tabUsedForTraining = tab;
    }
    else clusterers[tab]->SetParams(clusterer);
    clusterer->SetIterative(true);
    Train(clusterer);
    clusterers[tab]->Draw(canvas,clusterer);
	UpdateInfo();
}

void MLDemos::Maximize()
{
	if(!canvas) return;
	if(canvas->rewardPixmap.isNull()) return;
	QMutexLocker lock(&mutex);
	drawTimer->Stop();
	DEL(clusterer);
	DEL(regressor);
	DEL(dynamical);
	DEL(classifier);
	DEL(maximizer);
	int tab = optionsMaximize->tabWidget->currentIndex();
	if(tab >= maximizers.size() || !maximizers[tab]) return;
	maximizer = maximizers[tab]->GetMaximizer();
	maximizer->maxAge = optionsMaximize->iterationsSpin->value();
	maximizer->stopValue = optionsMaximize->stoppingSpin->value();
	tabUsedForTraining = tab;
	Train(maximizer);

	UpdateInfo();
	drawTimer->Stop();
	drawTimer->Clear();
	drawTimer->start(QThread::NormalPriority);
}

void MLDemos::MaximizeContinue()
{
	if(!canvas || !maximizer) return;
	QMutexLocker lock(&mutex);
	if(drawTimer)
	{
		drawTimer->Stop();
	}
	maximizer->SetConverged(!maximizer->hasConverged());

	UpdateInfo();
	if(drawTimer)
	{
		drawTimer->start(QThread::NormalPriority);
	}
}

bool MLDemos::Train(Classifier *classifier, int positive, float trainRatio)
{
    if(!classifier) return false;
    ivec labels = canvas->data->GetLabels();
    ivec newLabels;
    newLabels.resize(labels.size(), 1);
	bool bMulticlass = classifier->IsMultiClass();
	if(!bMulticlass)
	{
		if(positive == 0)
		{
			FOR(i, labels.size()) newLabels[i] = (!labels[i] || labels[i] == -1) ? 1 : -1;
		}
		else
		{
			FOR(i, labels.size()) newLabels[i] = (labels[i] == positive) ? 1 : -1;
		}
		bool bHasPositive = false, bHasNegative = false;
		FOR(i, newLabels.size())
		{
			if(bHasPositive && bHasNegative) break;
			bHasPositive |= newLabels[i] == 1;
			bHasNegative |= newLabels[i] == -1;
		}
		if((!bHasPositive || !bHasNegative) && !classifier->SingleClass()) return false;
	}
	else
	{
		newLabels = labels;
	}

    classifier->rocdata.clear();
    classifier->roclabels.clear();

    vector<fvec> samples = canvas->data->GetSamples();
    if(trainRatio == 1)
    {
        classifier->Train(samples, newLabels);
        // we generate the roc curve for this guy
        vector<f32pair> rocData;
        FOR(i, samples.size())
        {
			if(bMulticlass)
			{
				fvec res = classifier->TestMulti(samples[i]);
				int max = 0;
				for(int j=1; j<res.size(); j++) if(res[max] < res[j]) max = j;
				rocData.push_back(f32pair(max, newLabels[i]));
			}
			else
			{
				float resp = classifier->Test(samples[i]);
				rocData.push_back(f32pair(resp, newLabels[i]));
			}
        }
        classifier->rocdata.push_back(rocData);
        classifier->roclabels.push_back("training");
    }
    else
    {
        int trainCnt = (int)(samples.size()*trainRatio);
        u32 *perm = randPerm(samples.size());
        vector<fvec> trainSamples;
        ivec trainLabels;
        trainSamples.resize(trainCnt);
        trainLabels.resize(trainCnt);
        FOR(i, trainCnt)
        {
            trainSamples[i] = samples[perm[i]];
            trainLabels[i] = newLabels[perm[i]];
        }
        classifier->Train(trainSamples, trainLabels);

        // we generate the roc curve for this guy
        vector<f32pair> rocData;
        FOR(i, trainCnt)
        {
			if(bMulticlass)
			{
				fvec res = classifier->TestMulti(samples[perm[i]]);
				int max = 0;
				for(int j=1; j<res.size(); j++) if(res[max] < res[j]) max = j;
				rocData.push_back(f32pair(max, newLabels[perm[i]]));
			}
			else
			{
				float resp = classifier->Test(samples[perm[i]]);
				rocData.push_back(f32pair(resp, newLabels[perm[i]]));
			}
        }
        classifier->rocdata.push_back(rocData);
        classifier->roclabels.push_back("training");
        rocData.clear();
        for(int i=trainCnt; i<samples.size(); i++)
        {
			if(bMulticlass)
			{
				fvec res = classifier->TestMulti(samples[perm[i]]);
				int max = 0;
				for(int j=1; j<res.size(); j++) if(res[max] < res[j]) max = j;
				rocData.push_back(f32pair(max, newLabels[perm[i]]));
			}
			else
			{
				float resp = classifier->Test(samples[perm[i]]);
				rocData.push_back(f32pair(resp, newLabels[perm[i]]));
			}
        }
        classifier->rocdata.push_back(rocData);
        classifier->roclabels.push_back("test");
        KILL(perm);
    }
    bIsRocNew = true;
    bIsCrossNew = true;
    SetROCInfo();
    return true;
}

void MLDemos::Train(Regressor *regressor, float trainRatio)
{
    if(!regressor) return;
    vector<fvec> samples = canvas->data->GetSamples();
    ivec labels = canvas->data->GetLabels();
    fvec trainErrors, testErrors;
    if(trainRatio == 1.f)
    {
        regressor->Train(samples, labels);
        trainErrors.clear();
        FOR(i, samples.size())
        {
            fvec sample = samples[i];
            int dim = sample.size();
            fvec res = regressor->Test(sample);
            float error = fabs(res[0] - sample[dim-1]);
            trainErrors.push_back(error);
        }
        regressor->trainErrors = trainErrors;
        regressor->testErrors.clear();
    }
    else
    {
        int trainCnt = (int)(samples.size()*trainRatio);
        u32 *perm = randPerm(samples.size());
        vector<fvec> trainSamples;
        ivec trainLabels;
        trainSamples.resize(trainCnt);
        trainLabels.resize(trainCnt);
        FOR(i, trainCnt)
        {
            trainSamples[i] = samples[perm[i]];
            trainLabels[i] = labels[perm[i]];
        }
        regressor->Train(trainSamples, trainLabels);

        FOR(i, trainCnt)
        {
            fvec sample = samples[perm[i]];
            int dim = sample.size();
            fvec res = regressor->Test(sample);
            float error = fabs(res[0] - sample[dim-1]);
            trainErrors.push_back(error);
        }
        for(int i=trainCnt; i<samples.size(); i++)
        {
            fvec sample = samples[perm[i]];
            int dim = sample.size();
            fvec res = regressor->Test(sample);
            float error = fabs(res[0] - sample[dim-1]);
            testErrors.push_back(error);
        }
        regressor->trainErrors = trainErrors;
        regressor->testErrors = testErrors;
        KILL(perm);
    }
    bIsCrossNew = true;
}

// returns respectively the reconstruction error for the training points individually, per trajectory, and the error to target
fvec MLDemos::Train(Dynamical *dynamical)
{
	if(!dynamical) return fvec();
    vector<fvec> samples = canvas->data->GetSamples();
    vector<ipair> sequences = canvas->data->GetSequences();
    ivec labels = canvas->data->GetLabels();
	if(!samples.size() || !sequences.size()) return fvec();
    int dim = samples[0].size();
    int count = optionsDynamic->resampleSpin->value();
    int resampleType = optionsDynamic->resampleCombo->currentIndex();
    int centerType = optionsDynamic->centerCombo->currentIndex();
    bool zeroEnding = optionsDynamic->zeroCheck->isChecked();


	ivec trajLabels(sequences.size());
	FOR(i, sequences.size())
	{
		trajLabels[i] = canvas->data->GetLabel(sequences[i].first);
	}

	//float dT = 10.f; // time span between each data frame
	float dT = optionsDynamic->dtSpin->value();
	dynamical->dT = dT;
	//dT = 10.f;
	vector< vector<fvec> > trajectories = canvas->data->GetTrajectories(resampleType, count, centerType, dT, zeroEnding);
	interpolate(trajectories[0],count);

    dynamical->Train(trajectories, labels);
	return Test(dynamical, trajectories, labels);
}

void MLDemos::Train(Clusterer *clusterer)
{
    if(!clusterer) return;
    clusterer->Train(canvas->data->GetSamples());
}

void MLDemos::Train(Maximizer *maximizer)
{
	if(!maximizer) return;
	if(canvas->rewardPixmap.isNull()) return;
	QImage rewardImage = canvas->rewardPixmap.toImage();
	QRgb *pixels = (QRgb*) rewardImage.bits();
	int w = rewardImage.width();
	int h = rewardImage.height();
	float *data = new float[w*h];

	float maxData = 0;
	FOR(i, w*h)
	{
		data[i] = 1.f - qBlue(pixels[i])/255.f; // all data is in a 0-1 range
		maxData = max(maxData, data[i]);
		//data[i] = qRed(pixels[i])*(qAlpha(pixels[i]) / 255.f)/255.f; // all data is in a 0-1 range
	}
	if(maxData > 0)
	{
		FOR(i, w*h) data[i] /= maxData; // we ensure that the data is normalized
	}
	fvec startingPoint;
	if(canvas->targets.size())
	{
		startingPoint = canvas->targets[canvas->targets.size()-1];
		QPointF starting = canvas->toCanvasCoords(startingPoint);
		startingPoint[0] = starting.x()/w;
		startingPoint[1] = starting.y()/h;
	}
	maximizer->Train(data, fVec(w,h), startingPoint);
	maximizer->age = 0;
	delete [] data;
}

void MLDemos::Test(Maximizer *maximizer)
{
	if(!maximizer) return;

	do
	{
		fvec sample = maximizer->Test(maximizer->Maximum());
		maximizer->age++;
	}
	while(maximizer->age < maximizer->maxAge && maximizer->MaximumValue() < maximizer->stopValue);
}

// returns respectively the reconstruction error for the training points individually, per trajectory, and the error to target
fvec MLDemos::Test(Dynamical *dynamical, vector< vector<fvec> > trajectories, ivec labels)
{
	if(!dynamical || !trajectories.size()) return fvec();
	int dim = trajectories[0][0].size()/2;
	//(int dim = dynamical->Dim();
	float dT = dynamical->dT;
	fvec sample; sample.resize(dim,0);
	fvec vTrue; vTrue.resize(dim, 0);
	fvec xMin, xMax;
	xMin.resize(dim, FLT_MAX);
	xMax.resize(dim, -FLT_MAX);

	// test each trajectory for errors
	int errorCnt=0;
	float errorOne = 0, errorAll = 0;
	FOR(i, trajectories.size())
	{
		vector<fvec> t = trajectories[i];
		float errorTraj = 0;
		FOR(j, t.size())
		{
			FOR(d, dim)
			{
				sample[d] = t[j][d];
				vTrue[d] = t[j][d+dim];
				if(xMin[d] > sample[d]) xMin[d] = sample[d];
				if(xMax[d] < sample[d]) xMax[d] = sample[d];
			}
			fvec v = dynamical->Test(sample);
			float error = 0;
			FOR(d, dim) error += (v[d] - vTrue[d])*(v[d] - vTrue[d]);
			errorTraj += error;
			errorCnt++;
		}
		errorOne += errorTraj;
		errorAll += errorTraj / t.size();
	}
	errorOne /= errorCnt;
	errorAll /= trajectories.size();
	fvec res;
	res.push_back(errorOne);

	vector<fvec> endpoints;

	float errorTarget = 0;
	// test each trajectory for target
	FOR(i, trajectories.size())
	{
		fvec pos = trajectories[i][0];
		fvec end = trajectories[i][trajectories[i].size()-1];
		FOR(d, dim)
		{
			pos.pop_back();
			end.pop_back();
		}
		if(!endpoints.size()) endpoints.push_back(end);
		else
		{
			bool bExists = false;
			FOR(j, endpoints.size())
			{
				if(endpoints[j] == end)
				{
					bExists = true;
					break;
				}
			}
			if(!bExists) endpoints.push_back(end);
		}
		int steps = 500;
		float eps = FLT_MIN;
		FOR(j, steps)
		{
			fvec v = dynamical->Test(pos);
			float speed = 0;
			FOR(d, dim) speed += v[d]*v[d];
			speed = sqrtf(speed);
			if(speed*dT < eps) break;
			pos += v*dT;
		}
		float error = 0;
		FOR(d, dim)
		{
			error += (pos[d] - end[d])*(pos[d] - end[d]);
		}
		error = sqrtf(error);
		errorTarget += error;
	}
	errorTarget /= trajectories.size();
	res.push_back(errorTarget);

	fvec xDiff = xMax - xMin;
	fvec pos; pos.resize(dim);
	errorTarget = 0;
	int testCount = 100;
	FOR(i, testCount)
	{
		FOR(d, dim)
		{
			pos[d] = ((drand48()*2 - 0.5)*xDiff[d] + xMin[d]);
		}

		int steps = 500;
		float eps = FLT_MIN;
		FOR(j, steps)
		{
			fvec v = dynamical->Test(pos);
			float speed = 0;
			FOR(d, dim) speed += v[d]*v[d];
			speed = sqrtf(speed);
			if(speed*dT < eps) break;
			pos += v*dT;
		}
		float minError = FLT_MAX;
		FOR(j, endpoints.size())
		{
			float error = 0;
			FOR(d, dim)
			{
				error += (pos[d] - endpoints[j][d])*(pos[d] - endpoints[j][d]);
			}
			error = sqrtf(error);
			if(minError > error) minError = error;
		}
		errorTarget += minError;
	}
	errorTarget /= testCount;
	res.push_back(errorTarget);

	return res;
}

void MLDemos::Compare()
{
	if(!canvas) return;
	if(!compareOptions.size()) return;

	QMutexLocker lock(&mutex);
	drawTimer->Stop();
	DEL(clusterer);
	DEL(regressor);
	DEL(dynamical);
	DEL(classifier);
	DEL(maximizer);
	// we start parsing the algorithm list
	int folds = optionsCompare->foldCountSpin->value();
	float ratios [] = {.1f,.25f,1.f/3.f,.5f,2.f/3.f,.75f,.9f,1.f};
	int ratioIndex = optionsCompare->traintestRatioCombo->currentIndex();
	float trainRatio = ratios[ratioIndex];
	int positive = optionsCompare->positiveSpin->value();

	compare->Clear();

	QProgressDialog progress("Comparing Algorithms", "cancel", 0, folds*compareOptions.size());
	progress.show();
	FOR(i, compareOptions.size())
	{
		QString string = compareOptions[i];
		QTextStream stream(&string);
		QString line = stream.readLine();
		QString paramString = stream.readAll();
		if(line.startsWith("Maximization"))
		{
			QStringList s = line.split(":");
			int tab = s[1].toInt();
			if(tab >= maximizers.size() || !maximizers[tab]) continue;
			QTextStream paramStream(&paramString);
			QString paramName;
			float paramValue;
			while(!paramStream.atEnd())
			{
				paramStream >> paramName;
				paramStream >> paramValue;
				maximizers[tab]->LoadParams(paramName, paramValue);
			}
			QString algoName = maximizers[tab]->GetAlgoString();
			fvec resultIt, resultVal, resultEval;
			FOR(f, folds)
			{
				maximizer = maximizers[tab]->GetMaximizer();
				if(!maximizer) continue;
				maximizer->maxAge = optionsMaximize->iterationsSpin->value();
				maximizer->stopValue = optionsMaximize->stoppingSpin->value();
				Train(maximizer);
				Test(maximizer);
				resultIt.push_back(maximizer->age);
				resultVal.push_back(maximizer->MaximumValue());
				resultEval.push_back(maximizer->Evaluations());
				progress.setValue(f + i*folds);
				DEL(maximizer);
				qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
				if(progress.wasCanceled())
				{
					compare->AddResults(resultEval, "Evaluations", algoName);
					compare->AddResults(resultVal, "Reward", algoName);
					compare->AddResults(resultIt, "Iterations", algoName);
					compare->Show();
					return;
				}
			}
			compare->AddResults(resultEval, "Evaluations", algoName);
			compare->AddResults(resultVal, "Reward", algoName);
			compare->AddResults(resultIt, "Iterations", algoName);
		}
		if(line.startsWith("Classification"))
		{
			QStringList s = line.split(":");
			int tab = s[1].toInt();
			if(tab >= classifiers.size() || !classifiers[tab]) continue;
			QTextStream paramStream(&paramString);
			QString paramName;
			float paramValue;
			while(!paramStream.atEnd())
			{
				paramStream >> paramName;
				paramStream >> paramValue;
				classifiers[tab]->LoadParams(paramName, paramValue);
			}
			QString algoName = classifiers[tab]->GetAlgoString();
			fvec resultTrain, resultTest;
			FOR(f, folds)
			{
				classifier = classifiers[tab]->GetClassifier();
				if(!classifier) continue;
				Train(classifier, positive, trainRatio);
				if(classifier->rocdata.size()>0)
				{
					resultTrain.push_back(GetBestFMeasure(classifier->rocdata[0]));
				}
				if(classifier->rocdata.size()>1)
				{
					resultTest.push_back(GetBestFMeasure(classifier->rocdata[1]));
				}
				DEL(classifier);

				progress.setValue(f + i*folds);
				qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
				if(progress.wasCanceled())
				{
					compare->AddResults(resultTrain, "f-Measure (Training)", algoName);
					compare->AddResults(resultTest, "f-Measure (Test)", algoName);
					compare->Show();
					return;
				}
			}
			compare->AddResults(resultTrain, "f-Measure (Training)", algoName);
			compare->AddResults(resultTest, "f-Measure (Test)", algoName);
		}
		if(line.startsWith("Regression"))
		{
			QStringList s = line.split(":");
			int tab = s[1].toInt();
			if(tab >= regressors.size() || !regressors[tab]) continue;
			QTextStream paramStream(&paramString);
			QString paramName;
			float paramValue;
			while(!paramStream.atEnd())
			{
				paramStream >> paramName;
				paramStream >> paramValue;
				regressors[tab]->LoadParams(paramName, paramValue);
			}
			QString algoName = regressors[tab]->GetAlgoString();
			fvec resultTrain, resultTest;
			FOR(f, folds)
			{
				regressor = regressors[tab]->GetRegressor();
				if(!regressor) continue;
				Train(regressor, trainRatio);
				if(regressor->trainErrors.size())
				{
					float error = 0.f;
					FOR(i, regressor->trainErrors.size()) error += regressor->trainErrors[i];
					error /= regressor->trainErrors.size();
					resultTrain.push_back(error);
				}
				if(regressor->testErrors.size())
				{
					float error = 0.f;
					FOR(i, regressor->testErrors.size()) error += regressor->testErrors[i];
					error /= regressor->testErrors.size();
					resultTest = regressor->testErrors;
				}
				DEL(regressor);

				progress.setValue(f + i*folds);
				qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
				if(progress.wasCanceled())
				{
					compare->AddResults(resultTrain, "Error (Training)", algoName);
					compare->AddResults(resultTest, "Error (Testing)", algoName);
					compare->Show();
					return;
				}
			}
			compare->AddResults(resultTrain, "Error (Training)", algoName);
			compare->AddResults(resultTest, "Error (Testing)", algoName);
		}
		if(line.startsWith("Dynamical"))
		{
			QStringList s = line.split(":");
			int tab = s[1].toInt();
			if(tab >= dynamicals.size() || !dynamicals[tab]) continue;
			QTextStream paramStream(&paramString);
			QString paramName;
			float paramValue;
			while(!paramStream.atEnd())
			{
				paramStream >> paramName;
				paramStream >> paramValue;
				dynamicals[tab]->LoadParams(paramName, paramValue);
			}
			QString algoName = dynamicals[tab]->GetAlgoString();
			fvec resultReconst, resultTargetTraj, resultTarget;
			FOR(f, folds)
			{
				dynamical = dynamicals[tab]->GetDynamical();
				if(!dynamical) continue;
				fvec results = Train(dynamical);
				if(results.size())
				{
					resultReconst.push_back(results[0]);
					resultTargetTraj.push_back(results[1]);
					resultTarget.push_back(results[2]);
				}
				DEL(dynamical);

				progress.setValue(f + i*folds);
				qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
				if(progress.wasCanceled())
				{
					compare->AddResults(resultReconst, "Reconstruction Error", algoName);
					compare->AddResults(resultTargetTraj, "Target Error (trajectories)", algoName);
					compare->AddResults(resultTarget, "Target Error (random points)", algoName);
					compare->Show();
					return;
				}
			}
			compare->AddResults(resultReconst, "Reconstruction Error", algoName);
			compare->AddResults(resultTargetTraj, "Target Error (trajectories)", algoName);
			compare->AddResults(resultTarget, "Target Error (random points)", algoName);
		}
		compare->Show();
	}
}

void MLDemos::ExportOutput()
{
	if(!classifier && !regressor && !clusterer && !dynamical && !maximizer) return;
    // get a file
}

void MLDemos::ExportAnimation()
{
    if(!canvas->data->GetSamples().size()) return;
}

