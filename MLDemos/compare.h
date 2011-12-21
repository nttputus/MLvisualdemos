#ifndef _COMPARE_H_
#define _COMPARE_H_

#include <public.h>
#include <mymaths.h>
#include <drawUtils.h>
#include <QLabel>
#include "ui_compare.h"

class CompareAlgorithms : public QObject
{
	Q_OBJECT

	std::map< QString, std::vector< fvec > > results;
	std::map< QString, QStringList > algorithms;
	Ui::CompareDisplay *compareDisplay;
	QWidget *compareWidget;
	QLabel *display;
	QPixmap pixmap;

public:
	CompareAlgorithms(QWidget *parent=0);
	~CompareAlgorithms();
	void AddResults(fvec results, QString name, QString algorithm);
	void Show();
	void Clear();
	QPixmap &Display(){return pixmap;};

public slots:
	void Update();
};

#endif // _COMPARE_H_
