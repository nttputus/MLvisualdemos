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
#include "public.h"
#include "basicMath.h"
#include "classifierKNN.h"
#include <map>
using namespace std;

void ClassifierKNN::Train( std::vector< fvec > samples, ivec labels )
{
	if(!samples.size()) return;
	int dim = samples[0].size();
	DEL(kdTree);
	annClose();
	ANN::MetricType = (ANN_METRIC)metricType;
	ANN::MetricPower = metricP;
	this->samples = samples;
	this->labels = labels;

	dataPts = annAllocPts(samples.size(), dim);			// allocate data points
	FOR(i, samples.size())
	{
		FOR(j, dim) dataPts[i][j] = samples[i][j];
	}
	kdTree = new ANNkd_tree(dataPts, samples.size(), dim);

	FOR(i, labels.size())
	{
		counts[i] = 0;
	}
}

ClassifierKNN::~ClassifierKNN()
{
	annClose();
	DEL(kdTree);
}

fvec ClassifierKNN::TestMulti(const fvec &sample)
{
	if(!samples.size()) return fvec();
	fvec score;

	double eps = 0; // error bound
	ANNpoint queryPt; // query point
	queryPt = annAllocPt(sample.size()); // allocate query point
	ANNidxArray nnIdx = new ANNidx[k]; // allocate near neigh indices
	ANNdistArray dists = new ANNdist[k]; // allocate near neighbor dists
	FOR(i, sample.size()) queryPt[i] = sample[i];
	kdTree->annkSearch(queryPt, k, nnIdx, dists, eps);
	int cnt = 0;

	for(map<int,int>::iterator it = counts.begin(); it != counts.end(); it++)
	{
		it->second = 0;
	}

	FOR(i, k)
	{
		if(nnIdx[i] >= labels.size()) continue;
		int label = labels[nnIdx[i]];
		if(counts.count(label)) counts[label]++;
		cnt++;
	}
	delete [] nnIdx; // clean things up
	delete [] dists;

	FOR(i, 256)
	{
		if(counts.count(i)) score.push_back(counts[i]);
	}

	float sum = 0;
	FOR(i, score.size())
	{
		sum += score[i];
	}

	if(sum > 0)
	{
		FOR(i, score.size())
		{
			score[i] /= sum;
		}
	}

//	map<int,int>::iterator it = max_element(counts.begin(), counts.end()).second;
//	int label = it->first;
//	score = it->second / (float)cnt;

	return score;
}

float ClassifierKNN::Test( const fvec &sample )
{
	if(!samples.size()) return 0;
	float score = 0;
	double eps = 0; // error bound
	ANNpoint queryPt; // query point
	queryPt = annAllocPt(sample.size()); // allocate query point
	ANNidxArray nnIdx = new ANNidx[k]; // allocate near neigh indices
	ANNdistArray dists = new ANNdist[k]; // allocate near neighbor dists
	FOR(i, sample.size()) queryPt[i] = sample[i];
	kdTree->annkSearch(queryPt, k, nnIdx, dists, eps);
	int cnt = 0;
	//map<int,int> counts;
	FOR(i, k)
	{
		if(nnIdx[i] >= labels.size()) continue;
		int label = labels[nnIdx[i]];
		//if(counts.count(label)) counts[label]++;
		//else counts[label] = 0;
		score += labels[nnIdx[i]];
		cnt++;
	}
	score /= cnt;

	delete [] nnIdx; // clean things up
	delete [] dists;

	return score;
}

float ClassifierKNN::Test( const fVec &sample )
{
	if(!samples.size()) return 0;
	float score = 0;

	double eps = 0; // error bound
	ANNpoint queryPt; // query point
	queryPt = annAllocPt(sample.size()); // allocate query point
	ANNidxArray nnIdx = new ANNidx[k]; // allocate near neigh indices
	ANNdistArray dists = new ANNdist[k]; // allocate near neighbor dists
	FOR(i, sample.size()) queryPt[i] = sample._[i];
	kdTree->annkSearch(queryPt, k, nnIdx, dists, eps);
	int cnt = 0;
	FOR(i, k)
	{
		if(nnIdx[i] >= labels.size()) continue;
		score += labels[nnIdx[i]];
		cnt++;
	}
	score /= cnt;

	delete [] nnIdx; // clean things up
	delete [] dists;

	return score*2;
}

void ClassifierKNN::SetParams( u32 k, int metricType, u32 metricP )
{
	this->k = k;
	switch(metricType)
	{
	case 0:
		this->metricType = ANN_METRIC1;
		this->metricP = 1;
		break;
	case 1:
		this->metricType = ANN_METRIC2;
		this->metricP = 2;
		break;
	case 2:
		this->metricType = ANN_METRICP;
		this->metricP = metricP;
		break;
	case 3:
		this->metricType = ANN_METRIC0;
		this->metricP = 0;
		break;
	}
}

char *ClassifierKNN::GetInfoString()
{
	char *text = new char[1024];
	sprintf(text, "KNN\n");
	sprintf(text, "%sK: %d\n", text, k);
	sprintf(text, "%sMetric: ", text);
	switch(metricType)
	{
	case 0:
		sprintf(text, "%infinite norm\n", text);
		break;
	case 1:
		sprintf(text, "%s1-norm (Manhattan)\n", text);
		break;
	case 2:
		sprintf(text, "%s2-norm (Euclidean)\n", text);
		break;
	case 3:
		sprintf(text, "%s%d-norm\n", text, metricP);
		break;
	}
	return text;
}
