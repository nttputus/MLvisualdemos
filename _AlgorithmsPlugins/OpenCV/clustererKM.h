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
#ifndef _CLUSTERER_KM_H_
#define _CLUSTERER_KM_H_

#include <vector>
#include "clusterer.h"
#include "kmeans.h"

class ClustererKM : public Clusterer
{
private:
	float beta;
	u32 clusters;
	bool bSoft;
	bool bGmm;
	int power;

public:
	KMeansCluster *kmeans;

	ClustererKM();
	void Train(std::vector< fvec > samples);
	fvec Test( const fvec &sample);
	fvec Test( const fVec &sample);
	char *GetInfoString();

	void SetParams(u32 clusters, int method, float beta, int power);
};

#endif // _CLUSTERER_KM_H_
