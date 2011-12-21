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
#include "classifierLinear.h"
#include "JnS/Matutil.h"
#include "JnS/JnS.h"
#include <QDebug>

using namespace std;

void ClassifierLinear::Train( std::vector< fvec > samples, ivec labels )
{
	if(!samples.size()) return;

	switch(linearType)
	{
	case 0:
		TrainPCA(samples, labels);
		break;
	case 1:
		TrainLDA(samples, labels, false);
		break;
	case 2:
		TrainLDA(samples, labels);
		break;
	case 3:
		TrainICA(samples, labels);
		break;
	}

	vector<fvec> projected; projected.resize(samples.size());
	FOR(i, samples.size())
	{
		projected[i] = Project(samples[i]);
	}

	int dim = samples[0].size();
	meanPos.resize(dim,0);
	meanNeg.resize(dim,0);
	int cntPos=0, cntNeg=0;
	FOR(i, samples.size())
	{
		if(labels[i]==1)
		{
			FOR(d,dim) meanPos[d] += projected[i][d];
			cntPos++;
		}
		else
		{
			FOR(d,dim) meanNeg[d] += projected[i][d];
			cntNeg++;
		}
	}
	FOR(d,dim)
	{
		if(cntPos) meanPos[d] /= cntPos;
		if(cntNeg) meanNeg[d] /= cntNeg;
	}
	bUsesDrawTimer = false;
	minResponse = FLT_MAX;
	maxResponse = -FLT_MAX;
	float minResp = FLT_MAX;
	float maxResp = -FLT_MAX;
	FOR(i, samples.size())
	{
		float response = Test(samples[i]);
		if(minResp > response) minResp = response;
		if(maxResp < response) maxResp = response;
	}
	if(minResp == maxResp)
	{
			maxResp = minResp + 0.5f;
			minResp -= 0.5f;
	}
	minResponse = minResp;
	maxResponse = maxResp;
}

float ClassifierLinear::Test(const fvec &sample )
{
	float response = 0;
	if(linearType < 3) // pca, lda, fisher
	{
		fVec point(sample[0] - meanAll[0], sample[1] - meanAll[1]);
		float estimate = W*point;
		response = -(estimate - threshold);
	}
	else
	{
		float distPos = 0, distNeg = 0;
		if(meanPos.size() == sample.size() && meanNeg.size() == sample.size())
		{
			fvec projected = Project(sample);

			FOR(d,sample.size())
			{
				distPos += fabs(projected[d] - meanPos[d]);
				distNeg += fabs(projected[d] - meanNeg[d]);
			}
		}
		response = (distNeg - distPos);
	}
	if(minResponse != FLT_MAX)
	{
		response = (response-minResponse)/fabs(maxResponse-minResponse); // 0-1 range
		response = (response-0.5f)*6.f;
	}
	return response;
}

char *ClassifierLinear::GetInfoString()
{
	char *text = new char[1024];
	sprintf(text, "");
	switch(linearType)
	{
	case 0:
		sprintf(text, "%sPCA\n", text);
		break;
	case 1:
		sprintf(text, "%sLDA\n", text);
		break;
	case 2:
		sprintf(text, "%sFisher LDA\n", text);
		break;
	case 3:
		sprintf(text, "%sICA\n", text);
		break;
	default:
		sprintf(text, "%sNaive Bayes\n", text);
		break;
	}
	if(linearType == 3) // ica
	{
		sprintf(text, "%sUnmixing matrix:\n\t%.3f %.3f\n\t%.3f %.3f", text, Transf[0], Transf[1], Transf[2], Transf[3]);
	}
	else if(linearType < 3)
	{
		sprintf(text, "%sProjection Direction:\n\t%.3f %.3f\n", text, W.x, W.y);
	}
	return text;
}

void Invert(double *sigma, double *invSigma)
{
	double det = sigma[0]*sigma[3] - sigma[1]*sigma[2];
	if(det == 0) return; // non inversible
	invSigma[0] = sigma[3] / det;
	invSigma[1] = -sigma[1] / det;
	invSigma[2] = -sigma[2] / det;
	invSigma[3] = sigma[0] / det;
}

fvec ClassifierLinear::InvProject(const fvec &sample)
{
	fvec newSample = sample;
	if(linearType == 3) // ica
	{
		if(!Transf) return newSample;
		double iTransf[4];
		Invert(Transf, iTransf);
		double *Data = new double[2];
		Data[0] = sample[0];
		Data[1] = sample[1];
		//                Data[0] = sample[0]-meanPos[0];
		//                Data[1] = sample[1]-meanPos[1];
		Transform (Data, iTransf, 2, 1);
		newSample[0] = Data[0]+meanPos[0];
		newSample[1] = Data[1]+meanPos[1];
	}
	return newSample;
}

fvec ClassifierLinear::Project(const fvec &sample)
{
	fvec newSample = sample;
	if(linearType == 3) // ica
	{
		if(!Transf) return newSample;
		double *Data = new double[2];
		Data[0] = sample[0]-meanAll[0];
		Data[1] = sample[1]-meanAll[1];
		Transform (Data, Transf, 2, 1);
		newSample[0] = Data[0];
		newSample[1] = Data[1];
		//		newSample[0] = Data[0]+meanPos[0];
		//		newSample[1] = Data[1]+meanPos[1];
	}
	else if(linearType < 3) // pca, lda, fisher
	{
		fVec mean(meanAll[0], meanAll[1]);
		fVec point(sample[0], sample[1]);
		float dot = W*(point-mean);
		fVec proj(dot*W.x, dot*W.y);
		//proj += cvVec2(.5f,.5f);
		proj += mean;
		newSample[0] = proj.x;
		newSample[1] = proj.y;
	}
	return newSample;
}

void ClassifierLinear::SetParams( u32 linearType )
{
	this->linearType = linearType;
	if(linearType == 1 || linearType == 2) bSingleClass = false;
	else bSingleClass = true;
}

void ClassifierLinear::TrainPCA(std::vector< fvec > samples, const ivec &labels)
{
	u32 dim = 2;

	meanAll.resize(dim,0);
	float **sigma = NULL;

	FOR(i, samples.size())
	{
		meanAll += samples[i];
	}
	meanAll /= samples.size();

	fvec mean;
	mean.resize(dim,0);

	// we want zero mean samples
	FOR(i, samples.size()) samples[i] -= meanAll;

	GetCovariance(samples, mean, &sigma);

	float invSigma1[2][2] = {{sigma[1][1],-sigma[0][1]},{-sigma[0][1],sigma[0][0]}};
	FOR(i,2)
	{
		FOR(j,2)
		{
			invSigma1[i][j] /= sigma[0][0]*sigma[1][1] - sigma[0][1]*sigma[1][0];
		}
	}

	float determinant = (invSigma1[1][1]+invSigma1[0][0])*(invSigma1[1][1]+invSigma1[0][0])-4.0f*(invSigma1[0][0]*invSigma1[1][1]-invSigma1[1][0]*invSigma1[0][1]);
	float eigenvalue1, eigenvalue2;
	if (determinant > 0){
		eigenvalue1 = (invSigma1[1][1] + invSigma1[0][0] + sqrtf(determinant)) / 2.0f;
		eigenvalue2 = (invSigma1[1][1] + invSigma1[0][0] - sqrtf(determinant)) / 2.0f;
	}else{
		printf("determinant is not positive during calculation of eigenvalues !!");
		return;
	}
	fVec e1, e2, tmp;

	if(invSigma1[0][0] - eigenvalue1 != 0)
		e1.x = -invSigma1[0][1]/(invSigma1[0][0]-eigenvalue1);
	else e1.x = 0;
	e1.y = 1.0;

	if(invSigma1[0][0] - eigenvalue2 != 0)
		e2.x = -invSigma1[0][1]/(invSigma1[0][0]-eigenvalue2);
	else e2.x = 0;
	e2.y = 1.0;

	// swap the eigens
	if (eigenvalue1 < eigenvalue2)
	{
		fVec tmp = e1;
		e1 = e2;
		e2 = tmp;
		float lambda = eigenvalue1;
		eigenvalue1 = eigenvalue2;
		eigenvalue2 = lambda;
	}

	e1=e1.normalize();
	e2=e2.normalize();

	W = e2;
	if(W.x < 0) W *= -1;

	W = W.normalize();

	KILL(sigma);

	float threshold = 0;
	bool inverted = false;
	// we do the actual classification
	u32 steps = 1000;
	u32 minError = samples.size();
	for( int c=0; c<steps; c++)
	{
		u32 error = 0;
		u32 negError = 0;
		float thresh = 1.f / steps * c;
		FOR(i, samples.size())
		{
			fVec point(samples[i][0], samples[i][1]);
			float estimate = W*point;
			if(labels[i])
			{
				if (estimate < thresh) error++;
				else negError++;
			}
			else
			{
				if (estimate >= thresh) error++;
				else negError++;
			}
		}
		if(minError > min(negError, error))
		{
			inverted = negError > error;
			minError = min(negError, error);
			threshold = thresh;
		}
	}

	float error = minError / (f32)samples.size();
}

void ClassifierLinear::TrainLDA(std::vector< fvec > samples, const ivec &labels, bool bFisher)
{
	// we reduce the problem to a one vs many classification
	u32 dim = 2;

	meanAll.resize(dim,0);
	FOR(i, samples.size()) meanAll += samples[i];
	meanAll /= samples.size();
	FOR(i, samples.size()) samples[i] -= meanAll;

	vector<fvec> positives, negatives;
	FOR(i, samples.size())
	{
		if(labels[i]==1) positives.push_back(samples[i]);
		else negatives.push_back(samples[i]);
	}

	fvec mean1, mean2;
	mean1.resize(dim,0);
	mean2.resize(dim,0);
	float **sigma1 = NULL;
	float **sigma2 = NULL;

	FOR(i, positives.size())
	{
		FOR(j,dim)
		{
			mean1[j] += positives[i][j];
		}
	}
	mean1 /= (float)positives.size();

	FOR(i, negatives.size())
	{
		FOR(j,dim)
		{
			mean2[j] += negatives[i][j];
		}
	}
	mean2 /= (float)negatives.size();

	if(bFisher)
	{
		GetCovariance(positives, mean1, &sigma1);
		GetCovariance(negatives, mean2, &sigma2);
	}
	else
	{
		vector<fvec> posnegs;
		FOR(i, positives.size()) posnegs.push_back(positives[i] - mean1);
		FOR(i, negatives.size()) posnegs.push_back(negatives[i] - mean2);
		fvec zero;
		zero.resize(dim,0);
		GetCovariance(posnegs, zero, &sigma1);
		sigma2 = sigma1;
	}

	float sigma[2][2] = {{sigma1[0][0]+sigma2[0][0], sigma1[0][1]+sigma2[0][1]},{sigma1[1][0]+sigma2[1][0], sigma1[1][1]+sigma2[1][1]}};
	float invSigma[2][2] = {{sigma[1][1],-sigma[1][0]},{-sigma[0][1],sigma[0][0]}};
	FOR(i,2)
	{
		FOR(j,2)
		{
			invSigma[i][j] /= sigma[0][0]*sigma[1][1] - sigma[0][1]*sigma[1][0];
		}
	}

	float dM[2] = {mean2[0] - mean1[0],mean2[1] - mean1[1]};
	float w[2] = { invSigma[0][0]*dM[0] + invSigma[0][1]*dM[1] , invSigma[1][0]*dM[0] + invSigma[1][1]*dM[1] };

	float n = sqrtf(w[0]*w[0] + w[1]*w[1]);
	w[0] /= n; w[1] /= n;

	float c = w[0]*(mean1[0]+mean2[0])/2 + w[0]*(mean1[1]+mean2[1])/2;
	W = fVec(w[0], w[1]);
	W = W.normalize();

	if(sigma1 == sigma2)
	{
		KILL(sigma1);
	}
	else
	{
		KILL(sigma1);
		KILL(sigma2);
	}
}

void ClassifierLinear::GetCovariance(const vector<fvec> &samples, const fvec &mean, float ***covar)
{
	int dim = mean.size();
	float **cov = (*covar);
	if(!cov)
	{
		cov = new float*[dim];
		FOR(i, dim) cov[i] = new float[dim];
		(*covar) = cov;
	}
	FOR(i, dim)
	{
		FOR(j,dim)
		{
			cov[i][j] = 0;
		}
	}

	for (u32 i=0;i<samples.size();i++){
		float dX = samples[i][0] - mean[0];
		float dY = samples[i][1] - mean[1];
		cov[0][0] += dX*dX;
		cov[1][1] += dY*dY;
		cov[0][1] += dX*dY;
	}
	cov[0][0] /= (float)samples.size();
	cov[1][1] /= (float)samples.size();
	cov[0][1] /= (float)samples.size();
	cov[1][0] = cov[0][1];
}

void ClassifierLinear::TrainICA(std::vector< fvec > samples, const ivec &labels )
{
	u32 dim = 2;
	meanAll.resize(dim,0);
	float **sigma = NULL;
	FOR(i, samples.size())
	{
		meanAll += samples[i];
	}
	meanAll /= samples.size();

	const int nbsensors = samples[0].size();
	const int nbsamples = samples.size(); 

	if(!Transf)
	{
		if ((Transf = (double *) calloc(nbsensors*nbsensors, sizeof(double))) == NULL) OutOfMemory() ;
	}
	double *Data, *Mixing, *Global;
	if ((Data   = (double *) calloc(nbsensors*nbsamples, sizeof(double))) == NULL) OutOfMemory() ;
	if ((Mixing = (double *) calloc(nbsensors*nbsensors, sizeof(double))) == NULL) OutOfMemory() ;
	if ((Global = (double *) calloc(nbsensors*nbsensors, sizeof(double))) == NULL) OutOfMemory() ;

	FOR(i, samples.size())
	{
		Data[i*nbsensors + 0] = samples[i][0] - meanAll[0];
		Data[i*nbsensors + 1] = samples[i][1] - meanAll[1];

		//		Data[i*nbsensors + 0] = rand()/(float)RAND_MAX/5.;
		//		Data[i*nbsensors + 1] = 1 - rand()/(float)RAND_MAX/5.;
	}

	Identity(Mixing, nbsensors) ;   Mixing[0] = 2.0 ;
	Jade(Transf, Data, nbsensors, nbsamples ) ; 
	//Shibbs(Transf, Data, nbsensors, nbsamples ) ;

	FOR(i,nbsensors*nbsensors) Transf[i] /= 10;

	free(Data);
	//	free(Transf);  
	free(Mixing);
	free(Global);

	W = fVec(Transf[0], Transf[2]);
}
