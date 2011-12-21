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
#include <QtGui>
#include <QWidget>
#include <QSize>
#include <QPixmap>
#include <QPainter>
#include <QPen>
#include <QImage>

#include "public.h"
#include "basicMath.h"
#include "canvas.h"
#include "drawUtils.h"

using namespace std;

//DatasetManager Canvas::data;
bool Canvas::bCrossesAsDots = true;

Canvas::Canvas(QWidget *parent)
	: QWidget(parent),
	  bDisplayMap(false),
	  bDisplayInfo(false),
	  bDisplaySingle(false),
	  bDisplaySamples(true),
	  bDisplayTrajectories(true),
	  bDisplayLearned(true),
	  bDisplayGrid(true),
	  crosshair(QPainterPath()),
	  bShowCrosshair(false),
	  bNewCrosshair(true),
	  trajectoryCenterType(0),
	  trajectoryResampleType(1),
	  trajectoryResampleCount(100),
	  liveTrajectory(vector<fvec>()),
	  centers(map<int,fvec>()),
	  drawnSamples(0),
	  drawnTrajectories(0),
	  mouseAnchor(QPoint(-1,-1)),
	  bDrawing(false),
	  zoom(1.f),
	  data(new DatasetManager()),
	  xIndex(0), yIndex(1), center(2,0)
{
	resize(640,480);
	setAcceptDrops(true);

	setMouseTracking(true);
	setCursor(Qt::CrossCursor);
	setBackgroundRole(QPalette::Base);
	setMouseTracking(true);

	QPalette p(palette());
	p.setColor(backgroundRole(), Qt::white);
	setPalette(p);
	show();
}

Canvas::~Canvas()
{
	if(data) DEL(data);
}

void Canvas::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasFormat("text/plain")) event->acceptProposedAction();
}

void Canvas::dropEvent(QDropEvent *event)
{
	if(event->mimeData()->text() == "Target")
	{
		QPointF position = event->pos();
		//qDebug() << "Dropping Target at coordinates: " << position;
		targets.push_back(toSampleCoords(position.x(), position.y()));
	}
	else if(event->mimeData()->text() == "Gaussian")
	{
		QPointF position = event->pos();
		double variance = event->mimeData()->colorData().toDouble();
		PaintGaussian(position, variance);
	}
	else if(event->mimeData()->text() == "Gradient")
	{
		QPointF position = event->pos();
		PaintGradient(position);
	}
	event->acceptProposedAction();
}

void Canvas::SetConfidenceMap(QImage image)
{
	confidencePixmap = QPixmap::fromImage(image);
	repaint();
}

void Canvas::SetModelImage(QImage image)
{
	modelPixmap = QPixmap::fromImage(image);
	repaint();
}

void Canvas::Paint(QPainter &painter, bool bSvg)
{
	painter.setBackgroundMode(Qt::OpaqueMode);
	painter.setBackground(Qt::white);

	painter.fillRect(geometry(),Qt::white);

	if(bDisplayMap)
	{
		if(!confidencePixmap.isNull()) painter.drawPixmap(geometry(), confidencePixmap);
	}
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::HighQualityAntialiasing);

	if(bDisplaySamples)
	{
		DrawRewards();
		if(!rewardPixmap.isNull())
		{
			painter.setBackgroundMode(Qt::TransparentMode);
			painter.drawPixmap(geometry(), rewardPixmap);
		}
		if(bSvg) DrawSamples(painter);
		else
		{
			DrawSamples();
			painter.setBackgroundMode(Qt::TransparentMode);
			painter.drawPixmap(geometry(), samplesPixmap);
		}
		DrawObstacles(painter);
		//painter.drawPixmap(geometry(), obstaclesPixmap);
	}
	if(bDisplayTrajectories)
	{
		if(bSvg)
		{
			DrawTrajectories(painter);
		}
		else
		{
			DrawTrajectories();
			painter.setBackgroundMode(Qt::TransparentMode);
			painter.drawPixmap(geometry(), trajectoriesPixmap);
		}
		if(targets.size()) DrawTargets(painter);
	}
	if(!bSvg && bDisplayLearned && !modelPixmap.isNull())
	{
		painter.setBackgroundMode(Qt::TransparentMode);
		painter.drawPixmap(geometry(), modelPixmap);
	}
	if(!bSvg && bDisplayInfo && !infoPixmap.isNull())
	{
		painter.setBackgroundMode(Qt::TransparentMode);
		painter.drawPixmap(geometry(), infoPixmap);
	}
	if(!bSvg && bShowCrosshair)
	{
		if(bNewCrosshair) emit DrawCrosshair();
		painter.setBackgroundMode(Qt::TransparentMode);
		painter.drawPath(crosshair.translated(mouse));
		if(liveTrajectory.size()) DrawLiveTrajectory(painter);
	}
	if(bDisplayGrid)
	{
		if(!bSvg)
		{
		}
		else
		{
			if(gridPixmap.isNull()) RedrawAxes();
			painter.setBackgroundMode(Qt::TransparentMode);
			painter.drawPixmap(geometry(), gridPixmap);
		}
	}

}


void Canvas::paintEvent(QPaintEvent *event)
{
	if(bDrawing) return;
	bDrawing = true;
	QPainter painter(this);
	Paint(painter);
	bDrawing = false;
}

QPointF Canvas::toCanvasCoords(fvec sample)
{
	sample -= center;
	QPointF point(sample[xIndex]*(zoom*height()),sample[yIndex]*(zoom*height()));
	point += QPointF(width()/2, height()/2);
	return point;
}

QPointF Canvas::toCanvas(fVec sample)
{
	sample -= center;
	QPointF point(sample[xIndex]*(zoom*height()),sample[yIndex]*(zoom*height()));
	point += QPointF(width()/2, height()/2);
	return point;
}

QPointF Canvas::toCanvasCoords(float x, float y)
{
	x -= center[xIndex];
	y -= center[yIndex];
	QPointF point(x*(zoom*height()),y*(zoom*height()));
	point += QPointF(width()/2, height()/2);
	return point;
}
fVec Canvas::fromCanvas(QPointF point)
{
	fVec sample;
	point -= QPointF(width()/2.f,height()/2.f);
	sample[xIndex] = point.x()/(zoom*height());
	sample[yIndex] = point.y()/(zoom*height());
	sample += center;
	return sample;
}

fVec Canvas::fromCanvas(float x, float y)
{
	fVec sample;
	x -= width()/2.f;
	y -= height()/2.f;
	sample[xIndex] = x/(zoom*height());
	sample[yIndex] = y/(zoom*height());
	sample += center;
	return sample;
}

fvec Canvas::toSampleCoords(QPointF point)
{
	fvec sample;
	sample.resize(2);
	point -= QPointF(width()/2.f,height()/2.f);
	sample[xIndex] = point.x()/(zoom*height());
	sample[yIndex] = point.y()/(zoom*height());
	sample += center;
	return sample;
}

fvec Canvas::toSampleCoords(float x, float y)
{
	fvec sample;
	sample.resize(2);
	x -= width()/2.f;
	y -= height()/2.f;
	sample[xIndex] = x/(zoom*height());
	sample[yIndex] = y/(zoom*height());
	sample += center;
	return sample;
}

fvec Canvas::canvasTopLeft()
{
	return toSampleCoords(0,0);
}

fvec Canvas::canvasBottomRight()
{
	return toSampleCoords(width()-1,height()-1);
}

QRectF Canvas::canvasRect()
{
	fvec tl = canvasTopLeft();
	fvec br = canvasBottomRight();
	return QRectF(tl[xIndex], tl[yIndex], (br-tl)[xIndex], (br-tl)[yIndex]);
}

void Canvas::SetZoom(float zoom)
{
	if(this->zoom == zoom) return;
	this->zoom = zoom;
	gridPixmap = QPixmap();
	modelPixmap = QPixmap();
	confidencePixmap = QPixmap();
	//rewardPixmap = QPixmap();
	infoPixmap = QPixmap();
	ResetSamples();
	bNewCrosshair = true;
	//repaint();
}

void Canvas::SetCenter(fvec center)
{
	if(this->center == center) return;
	this->center = center;
	gridPixmap = QPixmap();
	modelPixmap = QPixmap();
	confidencePixmap = QPixmap();
	//rewardPixmap = QPixmap();
	infoPixmap = QPixmap();
	ResetSamples();
	bNewCrosshair = true;
	//repaint();
}

void Canvas::SetDim(int xIndex, int yIndex)
{
	bool bChanged = false;
	if(this->xIndex != xIndex)
	{
		bChanged = true;
		this->xIndex = xIndex;
	}
	if(this->yIndex != yIndex)
	{
		bChanged = true;
		this->yIndex = yIndex;
	}
	if(bChanged)
	{
		gridPixmap = QPixmap();
		modelPixmap = QPixmap();
		confidencePixmap = QPixmap();
		//rewardPixmap = QPixmap();
		infoPixmap = QPixmap();
		ResetSamples();
		bNewCrosshair = true;
		//repaint();
	}
}

void Canvas::FitToData()
{
	if(!data->GetCount())
	{
		center[xIndex] = 0;
		center[yIndex] = 0;
		SetZoom(1);
		return;
	}
	// we go through all the data and find the boundaries
	float minX=FLT_MAX, minY=FLT_MAX, maxX=-FLT_MAX, maxY=-FLT_MAX;
	vector<fvec> samples = data->GetSamples();
	FOR(i, samples.size())
	{
		fvec sample = samples[i];
		if(minX > sample[0]) minX = sample[0];
		if(minY > sample[1]) minY = sample[1];
		if(maxX < sample[0]) maxX = sample[0];
		if(maxY < sample[1]) maxY = sample[1];
	}

	// we compute the new zoom factor
	float diffX = maxX-minX;
	float diffY = maxY-minY;

	center[xIndex] = minX + diffX/2;
	center[yIndex] = minY + diffY/2;

	diffX *= 1.04; // add a small margin
	diffY *= 1.04; // add a small margin

	float aspectRatio = width()/(float)height();
	diffX /= aspectRatio;

	SetZoom(min(1/diffY,1/diffX));
}

void Canvas::DrawAxes(QPainter &painter)
{
	int w = width();
	int h = height();
	// we find out how 'big' the space is
	QRectF bounding = canvasRect();
	// we round up the size to the closest decimal
	float scale = bounding.height();
	if(scale <= 0) return;
	float mult = 1;
	if(scale > 10)
	{
		while(scale / mult > 10 && mult != 0) mult *= 2.5f; // we want at most 10 lines to draw
	}
	else
	{
		while(scale / mult < 5 && mult != 0) mult /= 2.f; // we want at least 5 lines to draw
	}
	if(mult == 0) mult = 1;

	// we now have the measure of the ticks, we can draw this
	painter.setRenderHint(QPainter::TextAntialiasing);
	painter.setFont(QFont("Lucida Grande", 9));
	for(float x = (int)(bounding.x()/mult)*mult; x < bounding.x() + bounding.width(); x += mult)
	{
		float canvasX = toCanvasCoords(x,0).x();
		painter.setPen(QPen(Qt::black, 0.5, Qt::DotLine));
		painter.drawLine(canvasX, 0, canvasX, h);
		painter.setPen(QPen(Qt::black, 0.5));
		painter.drawText(canvasX, h-5, QString("%1").arg((int)(x/mult)*mult));
	}
	// we now have the measure of the ticks, we can draw this
	for(float y = (int)(bounding.y()/mult)*mult; y < bounding.y() + bounding.height(); y += mult)
	{
		float canvasY = toCanvasCoords(0,y).y();
		painter.setPen(QPen(Qt::black, 0.5, Qt::DotLine));
		painter.drawLine(0, canvasY, w, canvasY);
		painter.setPen(QPen(Qt::black, 0.5));
		painter.drawText(2, canvasY, QString("%1").arg((int)(y/mult)*mult));
	}
}

void Canvas::RedrawAxes()
{
	int w = width();
	int h = height();
	gridPixmap = QPixmap(w,h);
	QBitmap bitmap(w,h);
	bitmap.clear();
	gridPixmap.setMask(bitmap);
	gridPixmap.fill(Qt::transparent);

	QPainter painter(&gridPixmap);
	DrawAxes(painter);
}

void Canvas::DrawSamples(QPainter &painter)
{
	int radius = 10;
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setRenderHint(QPainter::HighQualityAntialiasing);
	for(int i=0; i<data->GetCount(); i++)
	{
		if(data->GetFlag(i) == _TRAJ) continue;
		int label = data->GetLabel(i);
		QPointF point = toCanvasCoords(data->GetSample(i));
		Canvas::drawSample(painter, point, (data->GetFlag(i)==_TRAJ)?5:radius, bDisplaySingle ? 0 : label);
	}
}

void Canvas::DrawSamples()
{
	int radius = 10;
	if(!data->GetCount())
	{
		int w = width();
		int h = height();
		samplesPixmap = QPixmap(w,h);
		QBitmap bitmap(w,h);
		bitmap.clear();
		samplesPixmap.setMask(bitmap);
		samplesPixmap.fill(Qt::transparent);
		drawnSamples = 0;
		return;
	}
	if(drawnSamples == data->GetCount()) return;
	if(drawnSamples > data->GetCount()) drawnSamples = 0;

	if(!drawnSamples || samplesPixmap.isNull())
	{
		int w = width();
		int h = height();
		samplesPixmap = QPixmap(w,h);
		QBitmap bitmap(w,h);
		bitmap.clear();
		samplesPixmap.setMask(bitmap);
		samplesPixmap.fill(Qt::transparent);
		drawnSamples = 0;
	}
	QPainter painter(&samplesPixmap);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setRenderHint(QPainter::HighQualityAntialiasing);
	for(int i=drawnSamples; i<data->GetCount(); i++)
	{
		if(data->GetFlag(i) == _TRAJ) continue;
		int label = data->GetLabel(i);
		QPointF point = toCanvasCoords(data->GetSample(i));
		Canvas::drawSample(painter, point, (data->GetFlag(i)==_TRAJ)?5:radius, bDisplaySingle ? 0 : label);
	}
	drawnSamples = data->GetCount();
}

void Canvas::DrawTargets(QPainter &painter)
{
	painter.setRenderHint(QPainter::Antialiasing, true);
	FOR(i, targets.size())
	{
		QPointF point = toCanvasCoords(targets[i]);
		QPointF delta1 = QPointF(1,1);
		QPointF delta2 = QPointF(1,-1);
		painter.setBrush(Qt::NoBrush);
		painter.setPen(QPen(Qt::black, 1.5));
		int r = 8, p = 2;
		painter.drawEllipse(point,r,r);
		painter.drawLine(point+delta1*r, point+delta1*r+delta1*p);
		painter.drawLine(point-delta1*r, point-delta1*r-delta1*p);
		painter.drawLine(point+delta2*r, point+delta2*r+delta2*p);
		painter.drawLine(point-delta2*r, point-delta2*r-delta2*p);
	}
}

QPainterPath Canvas::DrawObstacle(Obstacle o)
{
	QPointF point;
	float aX = o.axes[0];
	float aY = o.axes[1];
	float angle = o.angle;
	float pX = o.power[0];
	float pY = o.power[1];
	QPainterPath obstaclePath;
	QPointF firstPoint;
	// first we draw the obstacle
	for(float theta=-PIf; theta < PIf + 0.1; theta += 0.1f)
	{
		float X, Y;
		X = aX * cosf(theta);
		//Y = aY * sinf(theta);
		Y = aY * (theta>=0?1.f:-1.f) * powf((1-powf(cosf(theta),2.f*pX)),1./(2*pY));

		float RX = + X * cosf(angle) - Y * sinf(angle);
		float RY = + X * sinf(angle) + Y * cosf(angle);

		point = QPointF(RX*(zoom*height()),RY*(zoom*height()));
		if(theta==-PIf)
		{
			firstPoint = point;
			obstaclePath.moveTo(point);
			continue;
		}
		obstaclePath.lineTo(point);
	}
	obstaclePath.lineTo(firstPoint);
	return obstaclePath;
}

void Canvas::DrawObstacles(QPainter &painter)
{
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::HighQualityAntialiasing);
	// we draw the obstacles
	if(!data->GetObstacles().size()) return;
	QList<QPainterPath> paths;
	QList<QPainterPath> safeties;
	FOR(i, data->GetObstacles().size())
	{
		QPainterPath obstaclePath = DrawObstacle(data->GetObstacle(i));
		obstaclePath.translate(toCanvasCoords(data->GetObstacle(i).center));
		paths.push_back(obstaclePath);
		obstaclePath = DrawObstacle(data->GetObstacle(i));

		QMatrix scalingMatrix;
		QPointF t = toCanvasCoords(data->GetObstacle(i).center);
		scalingMatrix.scale(data->GetObstacle(i).repulsion[0], data->GetObstacle(i).repulsion[1]);
		obstaclePath = scalingMatrix.map(obstaclePath);
		obstaclePath.translate(toCanvasCoords(data->GetObstacle(i).center));
		safeties.push_back(obstaclePath);
	}
	FOR(i, paths.size())
	{
		painter.setBrush(Qt::white);
		painter.setPen(QPen(Qt::black, 1,Qt::SolidLine));
		painter.drawPath(paths[i]);
		painter.setBrush(Qt::NoBrush);
		painter.setPen(QPen(Qt::black, 1,Qt::DotLine));
		painter.drawPath(safeties[i]);
	}
}

void Canvas::DrawObstacles()
{
	int w = width();
	int h = height();
	obstaclesPixmap = QPixmap(w,h);
	QBitmap bitmap(w,h);
	bitmap.clear();
	obstaclesPixmap.setMask(bitmap);
	obstaclesPixmap.fill(Qt::transparent);

	QPainter painter(&obstaclesPixmap);
	DrawObstacles(painter);
}

void Canvas::DrawRewards()
{
	return;
	int w = width();
	int h = height();
	rewardPixmap= QPixmap(w,h);
	QBitmap bitmap(w,h);
	bitmap.clear();
	rewardPixmap.setMask(bitmap);
	rewardPixmap.fill(Qt::transparent);

	if(!data->GetReward()->rewards) return;

	QPainter painter(&rewardPixmap);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::HighQualityAntialiasing);

	int radius = 10;
	int stepsW = w/radius;
	int stepsH = h/radius;
	//int radius = min(w,h) / steps;
	// we draw the rewards
	QColor color;
	fvec sample; sample.resize(2);
	FOR(i, stepsH)
	{
		float y = i/(float)stepsH*h;
		FOR(j, stepsW)
		{
			float x = j/(float)stepsW*w;
			float value = data->GetReward()->ValueAt(toSampleCoords(x,y));
			if(value > 0) color = QColor(255, 255 - (int)(max(0.f,min(1.f, value)) * 255), 255 - (int)(max(0.f,min(1.f, value)) * 255));
			else color = QColor(255 - (int)(max(0.f,min(1.f, -value)) * 255),255 - (int)(max(0.f,min(1.f, -value)) * 255),255);
			painter.setBrush(color);
			painter.setPen(Qt::black);
			painter.drawEllipse(QRectF(x-radius/2.,y-radius/2.,radius,radius));
		}
	}
}

void Canvas::DrawTrajectories(QPainter &painter)
{
	int w = width();
	int h = height();
	int count = data->GetCount();

	bool bDrawing = false;

	vector<ipair> sequences = data->GetSequences();
	int start=0, stop=0;
	if(data->GetFlag(count-1) == _TRAJ)
	{
		if(sequences.size()) stop = sequences[sequences.size()-1].second;
		if(stop < count-1) // there's an unfinished trajectory
		{
			stop++;
			for(start=count-1; start >= stop && data->GetFlag(start) == _TRAJ; start--);
			sequences.push_back(ipair(start+(sequences.size() ? 1 : 0),count-1));
			bDrawing = true;
		}
	}

	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::HighQualityAntialiasing);

	vector<fvec> samples = data->GetSamples();

	map<int,int> counts;
	centers.clear();
	if(trajectoryCenterType)
	{
		FOR(i, sequences.size())
		{
			int index = sequences[i].first;
			if(trajectoryCenterType==1) // end
			{
				index = sequences[i].second;
			}
			int label = data->GetLabel(index);
			if(!centers.count(label))
			{
				fvec center;
				center.resize(2,0);
				centers[label] = center;
				counts[label] = 0;
			}
			centers[label] += samples[index];
			counts[label]++;
		}

		for(map<int,int>::iterator p = counts.begin(); p!=counts.end(); ++p)
		{
			int label = p->first;
			centers[label] /= p->second;
		}
	}

	// do the interpolation
	vector< vector<fvec> > trajectories;
	vector<fvec> diffs;
	ivec trajLabels;
	FOR(i, sequences.size())
	{
		start = sequences[i].first;
		stop = sequences[i].second;
		int label = data->GetLabel(start);
		fvec diff;
		if(trajectoryCenterType && (i < sequences.size()-1 || !bDrawing))
		{
			diff = centers[label] - samples[trajectoryCenterType==1?stop:start];
		}
		else diff.resize(2,0);
		vector<fvec> trajectory;
		trajectory.resize(stop-start+1);
		int pos = 0;
		for (int j=start; j<=stop; j++)
		{
			trajectory[pos++] = samples[j] + diff;
		}
		switch (trajectoryResampleType)
		{
		case 0: // do nothing
			break;
		case 1: // uniform resampling
		{
			if(i < sequences.size()-1 || !bDrawing)
			{
				trajectory = interpolate(trajectory, trajectoryResampleCount);
			}
		}
			break;
		case 2: // spline resampling
		{
			if(i < sequences.size()-1 || !bDrawing)
			{
				trajectory = interpolate(trajectory, trajectoryResampleCount);
			}
		}
			break;
		}
		trajectories.push_back(trajectory);
		trajLabels.push_back(data->GetLabel(start));
	}

	// let's draw the trajectories
	FOR(i, trajectories.size())
	{
		fvec oldPt = trajectories[i][0];
		int count = trajectories[i].size();
		int label = trajLabels[i];
		FOR(j, count-1)
		{
			fvec pt = trajectories[i][j+1];
			painter.setPen(QPen(Qt::black, 0.5));
			painter.drawLine(toCanvasCoords(pt), toCanvasCoords(oldPt));
			if(j<count-2) Canvas::drawSample(painter, toCanvasCoords(pt), 5, bDisplaySingle ? 0 : label);
			oldPt = pt;
		}
		painter.setBrush(Qt::NoBrush);
		painter.setPen(Qt::green);
		painter.drawEllipse(toCanvasCoords(trajectories[i][0]), 5, 5);
		if(!bDrawing)
		{
			painter.setPen(Qt::red);
			painter.drawEllipse(toCanvasCoords(trajectories[i][count-1]), 5, 5);
		}
	}
}

void Canvas::DrawTrajectories()
{
	int w = width();
	int h = height();
	int count = data->GetCount();
	if(!count || (!data->GetSequences().size() && (data->GetFlag(count-1) != _TRAJ)))
	{
		trajectoriesPixmap = QPixmap(w,h);
		QBitmap bitmap(w,h);
		bitmap.clear();
		trajectoriesPixmap.setMask(bitmap);
		trajectoriesPixmap.fill(Qt::transparent);
		drawnTrajectories = 0;
	}

	bool bDrawing = false;

	vector<ipair> sequences = data->GetSequences();
	int start=0, stop=0;
	if(data->GetFlag(count-1) == _TRAJ)
	{
		if(sequences.size()) stop = sequences.back().second;
		if(stop < count-1) // there's an unfinished trajectory
		{
			stop++;
			for(start=count-1; start >= stop && data->GetFlag(start) == _TRAJ; start--);
			sequences.push_back(ipair(start+(sequences.size() ? 1 : 0),count-1));
			bDrawing = true;
		}
	}
	if(!bDrawing && drawnTrajectories == sequences.size()) return;
	if(drawnTrajectories > sequences.size()) drawnTrajectories = 0;

	if(!drawnTrajectories || trajectoriesPixmap.isNull())
	{
		trajectoriesPixmap = QPixmap(w,h);
		QBitmap bitmap(w,h);
		bitmap.clear();
		trajectoriesPixmap.setMask(bitmap);
		trajectoriesPixmap.fill(Qt::transparent);
		drawnTrajectories = 0;
	}

	QPainter painter(&trajectoriesPixmap);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::HighQualityAntialiasing);

	ivec trajLabels(sequences.size());
	FOR(i, sequences.size())
	{
		trajLabels[i] = data->GetLabel(sequences[i].first);
	}
	vector< vector<fvec> > trajectories = data->GetTrajectories(trajectoryResampleType, trajectoryResampleCount, trajectoryCenterType, 0.1, true);
	if(bDrawing)
	{
		vector<fvec> trajectory;
		for(int i=sequences.back().first; i<sequences.back().second; i++)
		{
			trajectory.push_back(data->GetSample(i));
		}
		if(trajectory.size()) trajectories.push_back(trajectory);
	}
	// let's draw the trajectories
	for(int i=drawnTrajectories; i<trajectories.size(); i++)
	{
		fvec oldPt = trajectories[i][0];
		int count = trajectories[i].size();
		int label = trajLabels[i];
		FOR(j, count-1)
		{
			fvec pt = trajectories[i][j+1];
			painter.setPen(QPen(Qt::black, 0.5));
			painter.drawLine(toCanvasCoords(pt), toCanvasCoords(oldPt));
			if(j<count-2) Canvas::drawSample(painter, toCanvasCoords(pt), 5, bDisplaySingle ? 0 : label);
			oldPt = pt;
		}
		painter.setBrush(Qt::NoBrush);
		painter.setPen(Qt::green);
		painter.drawEllipse(toCanvasCoords(trajectories[i][0]), 5, 5);
		if(!bDrawing)
		{
			painter.setPen(Qt::red);
			painter.drawEllipse(toCanvasCoords(trajectories[i][count-1]), 5, 5);
		}
	}
	drawnTrajectories = !bDrawing ? sequences.size() : sequences.size()-1;
}

void Canvas::DrawLiveTrajectory(QPainter &painter)
{
	if(!liveTrajectory.size() || !liveTrajectory[0].size()) return;
	int w = width();
	int h = height();
	fvec oldPt = liveTrajectory[0];
	int count = liveTrajectory.size();
	FOR(j, count-1)
	{
		fvec pt = liveTrajectory[j+1];
		if(!pt.size()) break;
		int label = 1;
		if(false && bDisplayMap)
		{
			painter.setPen(QPen(Qt::white, 3));
			painter.drawLine(toCanvasCoords(pt), toCanvasCoords(oldPt));
			painter.setPen(QPen(Qt::black, 1));
			painter.drawLine(toCanvasCoords(pt), toCanvasCoords(oldPt));
		}
		else
		{
			painter.setPen(QPen(Qt::magenta, 2));
			painter.drawLine(toCanvasCoords(pt), toCanvasCoords(oldPt));
		}
		//if(j<count-2) Canvas::drawSample(painter, QPoint(pt[0]*w, pt[1]*h), 5, label);
		oldPt = pt;
	}
	painter.setBrush(Qt::NoBrush);
	painter.setPen(Qt::green);
	painter.drawEllipse(toCanvasCoords(liveTrajectory[0]), 5, 5);
	painter.setPen(Qt::red);
	painter.drawEllipse(toCanvasCoords(liveTrajectory[count-1]), 5, 5);
}

void Canvas::ResizeEvent()
{
	bNewCrosshair = true;
	if(!rewardPixmap.isNull())
	{
		QPixmap newReward(width(), height());
		newReward = rewardPixmap.scaled(newReward.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	}
	RedrawAxes();
}

void Canvas::mousePressEvent( QMouseEvent *event )
{
	int x = event->x();
	int y = event->y();

	fvec sample = toSampleCoords(x,y);

	int label = 0;
	if(event->button()==Qt::LeftButton) label = 1;
	if(event->button()==Qt::RightButton) label = 0;

	if(event->modifiers()==Qt::AltModifier)
	{
		mouseAnchor = event->pos();
		return;
	}

	emit Drawing(sample, label);
}

void Canvas::mouseReleaseEvent( QMouseEvent *event )
{
	int x = event->x();
	int y = event->y();

	fvec sample = toSampleCoords(x,y);

	int label = 0;
	if(event->button()==Qt::LeftButton) label = 1;
	if(event->button()==Qt::RightButton) label = 0;

	mouseAnchor = QPoint(-1,-1);
	if(x > 0 && x < width() && y>0 && y<height()) bShowCrosshair = true;

	//emit Drawing(sample, label);
	emit Released();
}

void Canvas::enterEvent(QEvent *event)
{
	bShowCrosshair = true;
	repaint();
}

void Canvas::focusOutEvent(QFocusEvent *event)
{
	bShowCrosshair = false;
	bNewCrosshair = true;
	repaint();
}

void Canvas::leaveEvent(QEvent *event)
{
	bShowCrosshair = false;
	bNewCrosshair = true;
	//mouseAnchor = QPoint(-1,-1);
	repaint();
}

void Canvas::wheelEvent(QWheelEvent *event)
{
	float d = 0;
	if (event->delta() > 100) d = 1;
	if (event->delta() < 100) d = -1;
	if(d!=0) emit Navigation(fVec(-1,d));
}

void Canvas::mouseMoveEvent( QMouseEvent *event )
{
	int x = event->x();
	int y = event->y();
	mouse = QPoint(x,y);
	fvec sample = toSampleCoords(x,y);

	// we navigate in our environment
	if(event->modifiers() == Qt::AltModifier && event->buttons() == Qt::LeftButton)
	{
		fVec d = (fromCanvas(mouseAnchor) - fromCanvas(event->pos()));
		if(d.x == 0 && d.y == 0) return;
		SetCenter(center + d);
		mouseAnchor = event->pos();
		bShowCrosshair = false;
		emit CanvasMoveEvent();
		return;
	}

	if(event->buttons() != Qt::LeftButton && event->buttons() != Qt::RightButton )
	{
		emit Navigation(sample);
		repaint();
	}
	else
	{
		int label = 0;
		if(event->buttons()==Qt::LeftButton) label = 1;
		if(event->buttons()==Qt::RightButton) label = 0;
		emit Drawing(sample, label);
	}
}

bool Canvas::SaveScreenshot( QString filename )
{
	QPixmap screenshot = GetScreenshot();
	return screenshot.save(filename);
}

QPixmap Canvas::GetScreenshot()
{
	QPixmap screenshot(width(), height());
	QPainter painter(&screenshot);
	bool tmp = bShowCrosshair;
	painter.setBackgroundMode(Qt::OpaqueMode);
	painter.setBackground(Qt::white);
	Paint(painter);
	bShowCrosshair = tmp;
	return screenshot;
}

bool Canvas::DeleteData( QPointF center, float radius )
{
	bool anythingDeleted = false;
	FOR(i, data->GetCount())
	{
		QPointF dataPoint = toCanvasCoords(data->GetSample(i));
		QPointF point = this->mapToParent(QPoint(dataPoint.x(), dataPoint.y()));
		point -= center;
		if(sqrt(point.x()*point.x() + point.y()*point.y()) < radius)
		{
			anythingDeleted = true;
			data->RemoveSample(i);
			i--;
		}
	}
	FOR(i, data->GetObstacles().size())
	{
		QPointF obstaclePoint= toCanvasCoords(data->GetObstacle(i).center);
		QPointF point = this->mapToParent(QPoint(obstaclePoint.x(), obstaclePoint.y()));
		point -= center;
		if(sqrt(point.x()*point.x() + point.y()*point.y()) < radius)
		{
			anythingDeleted = true;
			data->RemoveObstacle(i);
			i--;
		}
	}
	return anythingDeleted;
}

void Canvas::PaintReward(fvec sample, float radius, float shift)
{
	int w = width();
	int h = height();
	if(rewardPixmap.isNull())
	{
		rewardPixmap = QPixmap(w,h);
		QBitmap bitmap(w,h);
		bitmap.clear();
		rewardPixmap.setMask(bitmap);
		rewardPixmap.fill(Qt::transparent);
		rewardPixmap.fill(Qt::white);
	}
	QPainter painter(&rewardPixmap);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	QPointF center = toCanvasCoords(sample);
	QRadialGradient gradient( center, radius*.75);
	if(shift > 0)
	{
		gradient.setColorAt(0, QColor(255,0,0,shift*255));
		gradient.setColorAt(1, QColor(255,0,0,0));
	}
	else
	{
		gradient.setColorAt(0, QColor(255,255,255,-shift*255));
		gradient.setColorAt(1, QColor(255,255,255,0));
	}
	painter.setBrush(gradient);
	//if(shift > 0) painter.setBrush(QColor(255,0,0,shift*255));
	//else painter.setBrush(QColor(255,255,255,-shift*255));
	painter.setPen(Qt::NoPen);
	painter.drawEllipse(toCanvasCoords(sample), radius, radius);
}

void Canvas::PaintGaussian(QPointF position, double variance)
{
	int w = width();
	int h = height();
	if(rewardPixmap.isNull())
	{
		rewardPixmap = QPixmap(w,h);
		QBitmap bitmap(w,h);
		bitmap.clear();
		rewardPixmap.setMask(bitmap);
		rewardPixmap.fill(Qt::transparent);
		rewardPixmap.fill(Qt::white);
	}

	QImage image(w, h, QImage::Format_ARGB32);
	image.fill(qRgb(255,255,255));
	fVec pos(position.x()/(float)w, position.y()/(float)h);
	fVec point;
	float invSigma = 1./(variance*variance);
	float a = invSigma*sqrtf(2*PIf);
	float value;
	float minVal = 1e30, maxVal = -1e30;
	qDebug() << "gaussian dropped at position " << position;
	FOR(i, w)
	{
		point.x = i/(float)w;
		FOR(j, h)
		{
			point.y = j/(float)h;
			value = (pos - point).lengthSquared();
			value = expf(-0.5*value*invSigma);
			value = (1.f - value);
			if(value < minVal) minVal = value;
			if(value > maxVal) maxVal = value;
			int color = 255.f*value;
//			if(color > 255) color = 255;
//			if(color < -255) color = 0;
			//int color = min(255, max(0, (int)(255.f*(1.f - value))));
			image.setPixel(i,j,qRgba(255, color, color, 255));
		}
	}
	QPainter painter(&rewardPixmap);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setCompositionMode(QPainter::CompositionMode_Darken);
	painter.drawPixmap(0,0,w,h,QPixmap::fromImage(image));
}

void Canvas::PaintGradient(QPointF position)
{
	int w = width();
	int h = height();
	if(rewardPixmap.isNull())
	{
		rewardPixmap = QPixmap(w,h);
		QBitmap bitmap(w,h);
		bitmap.clear();
		rewardPixmap.setMask(bitmap);
		rewardPixmap.fill(Qt::transparent);
		rewardPixmap.fill(Qt::white);
	}
	QPainter painter(&rewardPixmap);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	QPointF center(w/2.f, h/2.f);
	QPointF opposite = center - (position-center);
	QLinearGradient gradient(opposite, position);
	gradient.setColorAt(0, QColor(255,255,255,255));
	gradient.setColorAt(1, QColor(255,0,0,255));
	painter.setBrush(gradient);
	painter.setPen(Qt::NoPen);
	painter.drawRect(rewardPixmap.rect());
}
