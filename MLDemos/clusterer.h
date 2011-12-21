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
#ifndef _CLUSTERING_H_
#define _CLUSTERING_H_

#include <vector>
#include "mymaths.h"

extern "C" enum {CLUS_SVR, CLUS_GMM, CLUS_KM, CLUS_KKM, CLUS_NONE} clusteringType;

class Clusterer
{
protected:
	u32 dim;
	bool bIterative;

public:
	int type;

	Clusterer() : type(CLUS_NONE), dim(2), bIterative(false) {};
	~Clusterer(){};
	void Cluster(std::vector< fvec > allsamples) {Train(allsamples);};
	void SetIterative(bool iterative){bIterative = iterative;};

	virtual void Train(std::vector< fvec > samples){};
	virtual fvec Test( const fvec &sample){ return fvec(); };
	virtual fvec Test(const fVec &sample){ return Test((fvec)sample); };
	virtual char *GetInfoString(){return NULL;};
};

#endif // _CLUSTERING_H_
