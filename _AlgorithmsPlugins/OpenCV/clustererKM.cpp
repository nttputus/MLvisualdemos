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
#include "basicOpenCV.h"
#include "clustererKM.h"

using namespace std;

ClustererKM::ClustererKM()
: beta(1), clusters(1), bSoft(false), bGmm(false), kmeans(0)
{
	type = CLUS_KM;
}

void ClustererKM::Train(std::vector< fvec > samples)
{
	if(!samples.size()) return;
	int dim = samples[0].size();
	if(!bIterative)
	{
		DEL(kmeans);
	}
	bool bInit = false;
	if(!kmeans)
	{
		bInit = true;
		kmeans = new KMeansCluster(clusters);
		kmeans->AddPoints(samples);
		kmeans->ResetClusters();
	}
	kmeans->SetSoft(bSoft);
	kmeans->SetGMM(bGmm);
	kmeans->SetBeta(beta);
	kmeans->SetPower(power);

	if(!bGmm && !bSoft) kmeans->Update();
	else
	{
		kmeans->Update(bInit);
	}

	if(!bIterative)
	{
		int iterations = 20;
		FOR(i, iterations) kmeans->Update();
	}
}

fvec ClustererKM::Test( const fvec &sample)
{
	fvec res;
	res.resize(clusters,0);
	if(!kmeans) return res;
	kmeans->Test(sample, res);
	float sum = 0;
	FOR(i, res.size()) sum += res[i];
	FOR(i, res.size()) res[i] /= sum;
	return res;
}

fvec ClustererKM::Test( const fVec &sample)
{
	fvec res;
	res.resize(clusters,0);
	if(!kmeans) return res;
	kmeans->Test(sample, res);
	float sum = 0;
	FOR(i, res.size()) sum += res[i];
	FOR(i, res.size()) res[i] /= sum;
	return res;
}

void ClustererKM::SetParams(u32 clusters, int method, float beta, int power)
{
	this->clusters = clusters;
	this->beta = beta;
	this->power = power;
	switch(method)
	{
	case 0:
		this->bSoft = false;
		this->bGmm = false;
		break;
	case 1:
		this->bSoft = true;
		this->bGmm = false;
		break;
	case 2:
		this->bSoft = false;
		this->bGmm = true;
		break;
	}
}


char *ClustererKM::GetInfoString()
{
	char *text = new char[1024];
	sprintf(text, "K-Means\n");
	sprintf(text, "%sClusters: %d\n", text, clusters);
	sprintf(text, "%sType:", text);
	if(!bSoft && !bGmm) sprintf(text, "%sK-Means\n", text);
	else if(bSoft) sprintf(text, "%sSoft K-Means (beta: %.3f)\n", text, beta);
	else sprintf(text, "%sGMM\n", text);

	sprintf(text, "%sMetric: ", text);
	switch(power)
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
	default:
		sprintf(text, "%s%.2f-norm\n", text, power);
		break;
	}
	return text;
}
