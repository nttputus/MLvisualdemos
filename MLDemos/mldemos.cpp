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
#include <QDebug>
#include <QtGui>
#include <fstream>
#include <QPixmap>
#include <QUrl>
#include <QBitmap>
#include <QSettings>
#include <QFileDialog>
#include "basicMath.h"
#include "drawSVG.h"
#include <iostream>
#include <sstream>
#include "optimization_test_functions.h"


MLDemos::MLDemos(QString filename, QWidget *parent, Qt::WFlags flags)
    : QMainWindow(parent, flags),
      canvas(0),
      classifier(0),
      regressor(0),
      dynamical(0),
      clusterer(0),
      maximizer(0),
      bIsRocNew(true),
      bIsCrossNew(true),
      compareDisplay(0),
      compare(0),
      trajectory(ipair(-1,-1)),
      bNewObstacle(false),
      tabUsedForTraining(0)
{
    QApplication::setWindowIcon(QIcon(":/MLDemos/logo.png"));
    ui.setupUi(this);
    setAcceptDrops(true);

    connect(ui.actionExit, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(ShowAbout()));
    connect(ui.actionClearData, SIGNAL(triggered()), this, SLOT(ClearData()));
    connect(ui.actionClearModel, SIGNAL(triggered()), this, SLOT(Clear()));
    connect(ui.actionNew, SIGNAL(triggered()), this, SLOT(ClearData()));
    connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(SaveData()));
    connect(ui.actionLoad, SIGNAL(triggered()), this, SLOT(LoadData()));
    connect(ui.actionExportOutput, SIGNAL(triggered()), this, SLOT(ExportOutput()));
    connect(ui.actionExportAnimation, SIGNAL(triggered()), this, SLOT(ExportAnimation()));
    connect(ui.actionExport_SVG, SIGNAL(triggered()), this, SLOT(ExportSVG()));

    initDialogs();
    initToolBars();
    initPlugins();
    LoadLayoutOptions();
    SetTextFontSize();
    DisplayOptionChanged();
    UpdateInfo();
    FitToData();
    AlgoChanged();
    if(!classifiers.size()) algorithmOptions->tabWidget->setTabEnabled(0,false);
    if(!clusterers.size()) algorithmOptions->tabWidget->setTabEnabled(1,false);
    if(!regressors.size()) algorithmOptions->tabWidget->setTabEnabled(2,false);
    if(!dynamicals.size()) algorithmOptions->tabWidget->setTabEnabled(3,false);
    if(!maximizers.size()) algorithmOptions->tabWidget->setTabEnabled(4,false);

    algorithmWidget->setFixedSize(636,220);
    ui.canvasWidget->resize(width(), height());
    canvas->resize(ui.canvasWidget->width(), ui.canvasWidget->height());
    canvas->ResizeEvent();
    canvas->repaint();

    drawTime.start();
    if(filename != "") Load(filename);
}

void MLDemos::initToolBars()
{
    actionNew = new QAction(QIcon(":/MLDemos/icons/new.png"), tr("&New"), this);
    actionNew->setShortcut(QKeySequence(tr("Ctrl+N")));
    actionNew->setStatusTip(tr("Clear Everything"));

    actionSave = new QAction(QIcon(":/MLDemos/icons/save.png"), tr("&Save"), this);
    actionSave->setShortcut(QKeySequence(tr("Ctrl+S")));
    actionSave->setStatusTip(tr("Save Data"));

    actionLoad = new QAction(QIcon(":/MLDemos/icons/load.png"), tr("&Load"), this);
    actionLoad->setShortcut(QKeySequence(tr("Ctrl+L")));
    actionLoad->setStatusTip(tr("Load Data"));

    actionClassifiers = new QAction(QIcon(":/MLDemos/icons/classify.png"), tr("&Classification"), this);
    actionClassifiers->setShortcut(QKeySequence(tr("C")));
    actionClassifiers->setStatusTip(tr("Classify the data"));
    actionClassifiers->setCheckable(true);

    actionRegression = new QAction(QIcon(":/MLDemos/icons/regress.png"), tr("&Regression"), this);
    actionRegression->setShortcut(QKeySequence(tr("R")));
    actionRegression->setStatusTip(tr("Estimate Regression"));
    actionRegression->setCheckable(true);

    actionDynamical = new QAction(QIcon(":/MLDemos/icons/dynamical.png"), tr("&Dynamical"), this);
    actionDynamical->setShortcut(QKeySequence(tr("D")));
    actionDynamical->setStatusTip(tr("Estimate Dynamical System"));
    actionDynamical->setCheckable(true);

    actionClustering = new QAction(QIcon(":/MLDemos/icons/cluster.png"), tr("C&lustering"), this);
    actionClustering->setShortcut(QKeySequence(tr("L")));
    actionClustering->setStatusTip(tr("Cluster the data"));
    actionClustering->setCheckable(true);

    actionMaximizers = new QAction(QIcon(":/MLDemos/icons/maximize.png"), tr("&Maximize"), this);
    actionMaximizers->setShortcut(QKeySequence(tr("M")));
    actionMaximizers->setStatusTip(tr("Maximize Reward Function"));
    actionMaximizers->setCheckable(true);

    actionCompare = new QAction(QIcon(":/MLDemos/icons/compare.png"), tr("&Compare"), this);
    actionCompare->setShortcut(QKeySequence(tr("M")));
    actionCompare->setStatusTip(tr("Compare Algorithms"));
    actionCompare->setCheckable(true);

    actionDrawSamples = new QAction(QIcon(":/MLDemos/icons/draw.png"), tr("&Drawing"), this);
    actionDrawSamples->setShortcut(QKeySequence(tr("W")));
    actionDrawSamples->setStatusTip(tr("Show Sample Drawing Options"));
    actionDrawSamples->setCheckable(true);

    actionClearModel = new QAction(QIcon(":/MLDemos/icons/clearmodel.png"), tr("Clear Model"), this);
    actionClearModel->setShortcut(QKeySequence(tr("Shift+X")));
    actionClearModel->setStatusTip(tr("Clear current model"));

    actionClearData = new QAction(QIcon(":/MLDemos/icons/cleardata.png"), tr("Clear Data"), this);
    actionClearData->setShortcut(QKeySequence(tr("X")));
    actionClearData->setStatusTip(tr("Clear all data"));

    actionScreenshot = new QAction(QIcon(":/MLDemos/icons/screenshot.png"), tr("Save Screenshot"), this);
    actionScreenshot->setShortcut(QKeySequence(tr("Alt+S")));
    actionScreenshot->setStatusTip(tr("Save the current image to disk"));

    actionDisplayOptions = new QAction(QIcon(":/MLDemos/icons/display.png"), tr("Display &Options"), this);
    actionDisplayOptions->setShortcut(QKeySequence(tr("O")));
    actionDisplayOptions->setStatusTip(tr("Show Display Options"));
    actionDisplayOptions->setCheckable(true);

    actionShowStats = new QAction(QIcon(":/MLDemos/icons/stats.png"), tr("Info/Statistics"), this);
    actionShowStats->setShortcut(QKeySequence(tr("I")));
    actionShowStats->setStatusTip(tr("Display Algorithm Information and Data Statistics"));
    actionShowStats->setCheckable(true);

    connect(actionClassifiers, SIGNAL(triggered()), this, SLOT(ShowOptionClass()));
    connect(actionRegression, SIGNAL(triggered()), this, SLOT(ShowOptionRegress()));
    connect(actionDynamical, SIGNAL(triggered()), this, SLOT(ShowOptionDynamical()));
    connect(actionClustering, SIGNAL(triggered()), this, SLOT(ShowOptionCluster()));
    connect(actionMaximizers, SIGNAL(triggered()), this, SLOT(ShowOptionMaximize()));
    connect(actionCompare, SIGNAL(triggered()), this, SLOT(ShowOptionCompare()));
    connect(actionDrawSamples, SIGNAL(triggered()), this, SLOT(ShowSampleDrawing()));
    connect(actionDisplayOptions, SIGNAL(triggered()), this, SLOT(ShowOptionDisplay()));
    connect(actionClearData, SIGNAL(triggered()), this, SLOT(ClearData()));
    connect(actionClearModel, SIGNAL(triggered()), this, SLOT(Clear()));
    connect(actionScreenshot, SIGNAL(triggered()), this, SLOT(Screenshot()));
    connect(actionNew, SIGNAL(triggered()), this, SLOT(ClearData()));
    connect(actionSave, SIGNAL(triggered()), this, SLOT(SaveData()));
    connect(actionLoad, SIGNAL(triggered()), this, SLOT(LoadData()));
    connect(actionShowStats, SIGNAL(triggered()), this, SLOT(ShowStatsDialog()));

    /*
 connect(actionClearData, SIGNAL(triggered()), this, SLOT(ClearData()));
 connect(actionClearModel, SIGNAL(triggered()), this, SLOT(Clear()));
 connect(actionNew, SIGNAL(triggered()), this, SLOT(ClearData()));
 connect(actionSave, SIGNAL(triggered()), this, SLOT(SaveData()));
 connect(actionLoad, SIGNAL(triggered()), this, SLOT(LoadData()));
 */

    toolBar = addToolBar("Tools");
    toolBar->setObjectName("MainToolBar");
    toolBar->setMovable(false);
    toolBar->setFloatable(false);
    toolBar->setIconSize(QSize(32,32));
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    toolBar->addAction(actionNew);
    toolBar->addAction(actionLoad);
    toolBar->addAction(actionSave);
    toolBar->addSeparator();
    toolBar->addAction(actionClassifiers);
    toolBar->addAction(actionClustering);
    toolBar->addAction(actionRegression);
    toolBar->addAction(actionDynamical);
    toolBar->addAction(actionMaximizers);
    toolBar->addSeparator();
    toolBar->addAction(actionCompare);
    toolBar->addAction(actionClearModel);
    toolBar->addAction(actionClearData);
    toolBar->addSeparator();
    toolBar->addAction(actionDrawSamples);
    toolBar->addSeparator();
    toolBar->addAction(actionScreenshot);
    toolBar->addAction(actionDisplayOptions);
    toolBar->addAction(actionShowStats);
    toolBar->setVisible(true);

    connect(toolBar, SIGNAL(topLevelChanged(bool)), this, SLOT(ShowToolbar()));
    connect(ui.actionShow_Toolbar, SIGNAL(triggered()), this, SLOT(ShowToolbar()));
    connect(ui.actionSmall_Icons, SIGNAL(triggered()), this, SLOT(ShowToolbar()));

    QSize iconSize(24,24);
    drawToolbar->singleButton->setIcon(QIcon(":/MLDemos/icons/brush.png"));
    drawToolbar->singleButton->setIconSize(iconSize);
    drawToolbar->singleButton->setText("");
    drawToolbar->sprayButton->setIcon(QIcon(":/MLDemos/icons/airbrush.png"));
    drawToolbar->sprayButton->setIconSize(iconSize);
    drawToolbar->sprayButton->setText("");
    drawToolbar->eraseButton->setIcon(QIcon(":/MLDemos/icons/erase.png"));
    drawToolbar->eraseButton->setIconSize(iconSize);
    drawToolbar->eraseButton->setText("");
    drawToolbar->trajectoryButton->setIcon(QIcon(":/MLDemos/icons/trajectory.png"));
    drawToolbar->trajectoryButton->setIconSize(iconSize);
    drawToolbar->trajectoryButton->setText("");
    drawToolbar->lineButton->setIcon(QIcon(":/MLDemos/icons/line.png"));
    drawToolbar->lineButton->setIconSize(iconSize);
    drawToolbar->lineButton->setText("");
    drawToolbar->ellipseButton->setIcon(QIcon(":/MLDemos/icons/ellipse.png"));
    drawToolbar->ellipseButton->setIconSize(iconSize);
    drawToolbar->ellipseButton->setText("");
    drawToolbar->paintButton->setIcon(QIcon(":/MLDemos/icons/bigbrush.png"));
    drawToolbar->paintButton->setIconSize(iconSize);
    drawToolbar->paintButton->setText("");
    drawToolbar->obstacleButton->setIcon(QIcon(":/MLDemos/icons/obstacle.png"));
    drawToolbar->obstacleButton->setIconSize(iconSize);
    drawToolbar->obstacleButton->setText("");
}

void MLDemos::initDialogs()
{
    drawToolbar = new Ui::DrawingToolbar();
    drawToolbarContext1 = new Ui::DrawingToolbarContext1();
    drawToolbarContext2 = new Ui::DrawingToolbarContext2();
    drawToolbarContext3 = new Ui::DrawingToolbarContext3();
    drawToolbarContext4 = new Ui::DrawingToolbarContext4();

    drawToolbar->setupUi(drawToolbarWidget = new QWidget());
    drawToolbarContext1->setupUi(drawContext1Widget = new QWidget());
    drawToolbarContext2->setupUi(drawContext2Widget = new QWidget());
    drawToolbarContext3->setupUi(drawContext3Widget = new QWidget());
    drawToolbarContext4->setupUi(drawContext4Widget = new QWidget());

    connect(qApp, SIGNAL(focusChanged(QWidget *,QWidget *)),this,SLOT(FocusChanged(QWidget *,QWidget *)));

    drawToolbar->sprayButton->setContextMenuPolicy(Qt::CustomContextMenu);
    drawToolbar->ellipseButton->setContextMenuPolicy(Qt::CustomContextMenu);
    drawToolbar->lineButton->setContextMenuPolicy(Qt::CustomContextMenu);
    drawToolbar->eraseButton->setContextMenuPolicy(Qt::CustomContextMenu);
    drawToolbar->obstacleButton->setContextMenuPolicy(Qt::CustomContextMenu);
    drawToolbar->paintButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(drawToolbar->sprayButton, SIGNAL(customContextMenuRequested( const QPoint& )), this, SLOT(ShowContextMenuSpray(const QPoint &)));
    connect(drawToolbar->ellipseButton, SIGNAL(customContextMenuRequested( const QPoint& )), this, SLOT(ShowContextMenuEllipse(const QPoint &)));
    connect(drawToolbar->lineButton, SIGNAL(customContextMenuRequested( const QPoint& )), this, SLOT(ShowContextMenuLine(const QPoint &)));
    connect(drawToolbar->eraseButton, SIGNAL(customContextMenuRequested( const QPoint& )), this, SLOT(ShowContextMenuErase(const QPoint &)));
    connect(drawToolbar->obstacleButton, SIGNAL(customContextMenuRequested( const QPoint& )), this, SLOT(ShowContextMenuObstacle(const QPoint &)));
    connect(drawToolbar->paintButton, SIGNAL(customContextMenuRequested( const QPoint& )), this, SLOT(ShowContextMenuReward(const QPoint &)));

    displayOptions = new Ui::viewOptionDialog();
    aboutPanel = new Ui::aboutDialog();
    showStats = new Ui::statisticsDialog();

    displayOptions->setupUi(displayDialog = new QDialog());
    aboutPanel->setupUi(about = new QDialog());
    showStats->setupUi(statsDialog = new QDialog());
    rocWidget = new QNamedWindow("ROC Curve", false, showStats->rocWidget);
    crossvalidWidget = new QNamedWindow("Cross Validation", false, showStats->crossvalidWidget);
    infoWidget = new QNamedWindow("Info", false, showStats->informationWidget);

    connect(showStats->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(StatsChanged()));
    connect(rocWidget, SIGNAL(ResizeEvent(QResizeEvent *)), this, SLOT(StatsChanged()));
    connect(crossvalidWidget, SIGNAL(ResizeEvent(QResizeEvent *)), this, SLOT(StatsChanged()));
    connect(infoWidget, SIGNAL(ResizeEvent(QResizeEvent *)), this, SLOT(StatsChanged()));

    connect(drawToolbar->singleButton, SIGNAL(clicked()), this, SLOT(DrawSingle()));
    connect(drawToolbar->sprayButton, SIGNAL(clicked()), this, SLOT(DrawSpray()));
    connect(drawToolbar->lineButton, SIGNAL(clicked()), this, SLOT(DrawLine()));
    connect(drawToolbar->ellipseButton, SIGNAL(clicked()), this, SLOT(DrawEllipse()));
    connect(drawToolbar->eraseButton, SIGNAL(clicked()), this, SLOT(DrawErase()));
    connect(drawToolbar->trajectoryButton, SIGNAL(clicked()), this, SLOT(DrawTrajectory()));
    connect(drawToolbar->obstacleButton, SIGNAL(clicked()), this, SLOT(DrawObstacle()));
    connect(drawToolbar->paintButton, SIGNAL(clicked()), this, SLOT(DrawPaint()));

    connect(displayOptions->clipboardButton, SIGNAL(clicked()), this, SLOT(ToClipboard()));
    connect(displayOptions->mapCheck, SIGNAL(clicked()), this, SLOT(DisplayOptionChanged()));
    connect(displayOptions->modelCheck, SIGNAL(clicked()), this, SLOT(DisplayOptionChanged()));
    connect(displayOptions->infoCheck, SIGNAL(clicked()), this, SLOT(DisplayOptionChanged()));
    connect(displayOptions->samplesCheck, SIGNAL(clicked()), this, SLOT(DisplayOptionChanged()));
    connect(displayOptions->gridCheck, SIGNAL(clicked()), this, SLOT(DisplayOptionChanged()));
    connect(displayOptions->spinZoom, SIGNAL(valueChanged(double)), this, SLOT(DisplayOptionChanged()));
    connect(displayOptions->zoomFitButton, SIGNAL(clicked()), this, SLOT(FitToData()));
	connect(displayOptions->xDimIndex, SIGNAL(valueChanged(int)), this, SLOT(DisplayOptionChanged()));
	connect(displayOptions->yDimIndex, SIGNAL(valueChanged(int)), this, SLOT(DisplayOptionChanged()));
	//	connect(displayOptions->trajectoriesCheck, SIGNAL(clicked()), this, SLOT(DisplayOptionChanged()));
    //	connect(displayOptions->singleclassCheck, SIGNAL(clicked()), this, SLOT(DisplayOptionChanged()));


    algorithmOptions = new Ui::algorithmOptions();
    optionsClassify = new Ui::optionsClassifyWidget();
    optionsCluster = new Ui::optionsClusterWidget();
    optionsRegress = new Ui::optionsRegressWidget();
    optionsDynamic = new Ui::optionsDynamicWidget();
    optionsMaximize = new Ui::optionsMaximizeWidget();
    optionsCompare = new Ui::optionsCompare();

    algorithmWidget = new QWidget();
    algorithmOptions->setupUi(algorithmWidget);

    algorithmWidget->setWindowFlags(Qt::Tool); // disappears when unfocused on the mac
    //algorithmWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    displayDialog->setWindowFlags(Qt::Tool); // disappears when unfocused on the mac
    //drawToolbarWidget->setWindowFlags(Qt::Tool);
    drawToolbarWidget->setWindowFlags(Qt::CustomizeWindowHint | Qt::Tool | Qt::WindowTitleHint);
    drawContext1Widget->setWindowFlags(Qt::FramelessWindowHint);
    drawContext2Widget->setWindowFlags(Qt::FramelessWindowHint);
    drawContext3Widget->setWindowFlags(Qt::FramelessWindowHint);
    drawContext4Widget->setWindowFlags(Qt::FramelessWindowHint);
    drawToolbarWidget->setFixedSize(drawToolbarWidget->size());

    classifyWidget = new QWidget(algorithmOptions->tabClass);
    clusterWidget = new QWidget(algorithmOptions->tabClust);
    regressWidget = new QWidget(algorithmOptions->tabRegr);
    dynamicWidget = new QWidget(algorithmOptions->tabDyn);
    maximizeWidget = new QWidget(algorithmOptions->tabMax);
    optionsClassify->setupUi(classifyWidget);
    optionsCluster->setupUi(clusterWidget);
    optionsRegress->setupUi(regressWidget);
    optionsDynamic->setupUi(dynamicWidget);
    optionsMaximize->setupUi(maximizeWidget);
    compareWidget = new QWidget();
    optionsCompare->setupUi(compareWidget);

    connect(displayDialog, SIGNAL(rejected()), this, SLOT(HideOptionDisplay()));
    connect(statsDialog, SIGNAL(rejected()), this, SLOT(HideStatsDialog()));

    connect(optionsClassify->classifyButton, SIGNAL(clicked()), this, SLOT(Classify()));
    connect(optionsClassify->clearButton, SIGNAL(clicked()), this, SLOT(Clear()));
    connect(optionsClassify->rocButton, SIGNAL(clicked()), this, SLOT(ShowRoc()));
    connect(optionsClassify->crossValidButton, SIGNAL(clicked()), this, SLOT(ClassifyCross()));
    connect(optionsClassify->compareButton, SIGNAL(clicked()), this, SLOT(CompareAdd()));

    connect(optionsRegress->regressionButton, SIGNAL(clicked()), this, SLOT(Regression()));
    connect(optionsRegress->crossValidButton, SIGNAL(clicked()), this, SLOT(RegressionCross()));
    connect(optionsRegress->clearButton, SIGNAL(clicked()), this, SLOT(Clear()));
    //connect(optionsRegress->svmTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(ChangeActiveOptions()));
    connect(optionsRegress->compareButton, SIGNAL(clicked()), this, SLOT(CompareAdd()));

    connect(optionsCluster->clusterButton, SIGNAL(clicked()), this, SLOT(Cluster()));
    connect(optionsCluster->iterationButton, SIGNAL(clicked()), this, SLOT(ClusterIterate()));
    connect(optionsCluster->clearButton, SIGNAL(clicked()), this, SLOT(Clear()));

    connect(optionsDynamic->regressionButton, SIGNAL(clicked()), this, SLOT(Dynamize()));
    connect(optionsDynamic->clearButton, SIGNAL(clicked()), this, SLOT(Clear()));
    connect(optionsDynamic->centerCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(ChangeActiveOptions()));
    connect(optionsDynamic->resampleCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(ChangeActiveOptions()));
    connect(optionsDynamic->resampleSpin, SIGNAL(valueChanged(int)), this, SLOT(ChangeActiveOptions()));

	connect(optionsDynamic->dtSpin, SIGNAL(valueChanged(double)), this, SLOT(ChangeActiveOptions()));
	connect(optionsDynamic->obstacleCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(AvoidOptionChanged()));
	connect(optionsDynamic->compareButton, SIGNAL(clicked()), this, SLOT(CompareAdd()));
	connect(optionsDynamic->colorCheck, SIGNAL(clicked()), this, SLOT(ColorMapChanged()));

	connect(optionsMaximize->maximizeButton, SIGNAL(clicked()), this, SLOT(Maximize()));
	connect(optionsMaximize->pauseButton, SIGNAL(clicked()), this, SLOT(MaximizeContinue()));
	connect(optionsMaximize->clearButton, SIGNAL(clicked()), this, SLOT(Clear()));
	connect(optionsMaximize->targetButton, SIGNAL(pressed()), this, SLOT(TargetButton()));
	connect(optionsMaximize->gaussianButton, SIGNAL(pressed()), this, SLOT(GaussianButton()));
	connect(optionsMaximize->gradientButton, SIGNAL(pressed()), this, SLOT(GradientButton()));
	connect(optionsMaximize->benchmarkButton, SIGNAL(clicked()), this, SLOT(BenchmarkButton()));
	connect(optionsMaximize->compareButton, SIGNAL(clicked()), this, SLOT(CompareAdd()));

	connect(optionsCompare->compareButton, SIGNAL(clicked()), this, SLOT(Compare()));
	connect(optionsCompare->screenshotButton, SIGNAL(clicked()), this, SLOT(CompareScreenshot()));
	connect(optionsCompare->clearButton, SIGNAL(clicked()), this, SLOT(CompareClear()));
	connect(optionsCompare->removeButton, SIGNAL(clicked()), this, SLOT(CompareRemove()));

    optionsClassify->tabWidget->clear();
    optionsCluster->tabWidget->clear();
    optionsRegress->tabWidget->clear();
    optionsDynamic->tabWidget->clear();
    optionsMaximize->tabWidget->clear();
    optionsClassify->tabWidget->setUsesScrollButtons(true);
    optionsCluster->tabWidget->setUsesScrollButtons(true);
    optionsRegress->tabWidget->setUsesScrollButtons(true);
    optionsDynamic->tabWidget->setUsesScrollButtons(true);
    optionsMaximize->tabWidget->setUsesScrollButtons(true);

    QHBoxLayout *layout = new QHBoxLayout(optionsCompare->resultWidget);
    compare = new CompareAlgorithms(optionsCompare->resultWidget);

    connect(algorithmOptions->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(AlgoChanged()));
    connect(optionsClassify->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(AlgoChanged()));
    connect(optionsCluster->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(AlgoChanged()));
    connect(optionsRegress->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(AlgoChanged()));
    connect(optionsDynamic->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(AlgoChanged()));
    connect(optionsMaximize->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(AlgoChanged()));

    //canvas = new Canvas(ui.centralWidget);
    canvas = new Canvas(ui.canvasWidget);
    connect(canvas, SIGNAL(Drawing(fvec,int)), this, SLOT(Drawing(fvec,int)));
    connect(canvas, SIGNAL(DrawCrosshair()), this, SLOT(DrawCrosshair()));
    connect(canvas, SIGNAL(Navigation(fvec)), this, SLOT(Navigation(fvec)));
    connect(canvas, SIGNAL(Released()), this, SLOT(DrawingStopped()));
    connect(canvas, SIGNAL(CanvasMoveEvent()), this, SLOT(CanvasMoveEvent()));
    //connect(canvas, SIGNAL(ZoomChanged()), this, SLOT(ZoomChanged()));
    drawTimer = new DrawTimer(canvas, &mutex);
    drawTimer->classifier = &classifier;
    drawTimer->regressor = &regressor;
    drawTimer->dynamical = &dynamical;
    drawTimer->clusterer = &clusterer;
    drawTimer->maximizer = &maximizer;
    connect(drawTimer, SIGNAL(MapReady(QImage)), canvas, SLOT(SetConfidenceMap(QImage)));
    connect(drawTimer, SIGNAL(ModelReady(QImage)), canvas, SLOT(SetModelImage(QImage)));
    connect(drawTimer, SIGNAL(CurveReady()), this, SLOT(SetROCInfo()));
}

void MLDemos::initPlugins()
{
    qDebug() << "Importing plugins";
    QDir pluginsDir = QDir(qApp->applicationDirPath());
    QStringList pluginFileNames;
    QDir alternativeDir = pluginsDir;

#if defined(Q_OS_WIN)
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release") pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    if (pluginsDir.dirName() == "MacOS") {
        if(!pluginsDir.cd("plugins"))
        {
            qDebug() << "looking for alternative directory";
            pluginsDir.cdUp();
            pluginsDir.cdUp();
            alternativeDir = pluginsDir;
            alternativeDir.cd("plugins");
        }
        pluginsDir.cdUp();
    }
#endif
    bool bFoundPlugins = false;
#if defined(DEBUG)
    qDebug() << "looking for debug plugins";
    bFoundPlugins = pluginsDir.cd("pluginsDebug");
#else
    qDebug() << "looking for release plugins";
    bFoundPlugins = pluginsDir.cd("plugins");
#endif
    if(!bFoundPlugins)
    {
        qDebug() << "plugins not found on: " << pluginsDir.absolutePath();
        qDebug() << "using alternative directory: " << alternativeDir.absolutePath();
        pluginsDir = alternativeDir;
    }
    foreach (QString fileName, pluginsDir.entryList(QDir::Files))
    {
        QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = loader.instance();
        if (plugin)
        {
            qDebug() << fileName;
            // check type of plugin
            CollectionInterface *iCollection = qobject_cast<CollectionInterface *>(plugin);
            if(iCollection)
            {
                std::vector<ClassifierInterface*> classifierList = iCollection->GetClassifiers();
                std::vector<ClustererInterface*> clustererList = iCollection->GetClusterers();
                std::vector<RegressorInterface*> regressorList = iCollection->GetRegressors();
                std::vector<DynamicalInterface*> dynamicalList = iCollection->GetDynamicals();
                std::vector<MaximizeInterface*> maximizerList = iCollection->GetMaximizers();
                FOR(i, classifierList.size()) AddPlugin(classifierList[i], SLOT(ChangeActiveOptions));
                FOR(i, clustererList.size()) AddPlugin(clustererList[i], SLOT(ChangeActiveOptions));
                FOR(i, regressorList.size()) AddPlugin(regressorList[i], SLOT(ChangeActiveOptions));
                FOR(i, dynamicalList.size()) AddPlugin(dynamicalList[i], SLOT(ChangeActiveOptions));
                FOR(i, maximizerList.size()) AddPlugin(maximizerList[i], SLOT(ChangeActiveOptions));
                continue;
            }
            ClassifierInterface *iClassifier = qobject_cast<ClassifierInterface *>(plugin);
            if (iClassifier)
            {
                AddPlugin(iClassifier, SLOT(ChangeActiveOptions()));
                continue;
            }
            ClustererInterface *iClusterer = qobject_cast<ClustererInterface *>(plugin);
            if (iClusterer)
            {
                AddPlugin(iClusterer, SLOT(ChangeActiveOptions()));
                continue;
            }
            RegressorInterface *iRegressor = qobject_cast<RegressorInterface *>(plugin);
            if (iRegressor)
            {
                AddPlugin(iRegressor, SLOT(ChangeActiveOptions()));
                continue;
            }
            DynamicalInterface *iDynamical = qobject_cast<DynamicalInterface *>(plugin);
            if (iDynamical)
            {
                AddPlugin(iDynamical, SLOT(ChangeActiveOptions()));
                continue;
            }
            MaximizeInterface *iMaximize = qobject_cast<MaximizeInterface *>(plugin);
            if (iMaximize)
            {
                AddPlugin(iMaximize, SLOT(ChangeActiveOptions()));
                continue;
            }
            InputOutputInterface *iIO = qobject_cast<InputOutputInterface *>(plugin);
            if (iIO)
            {
                AddPlugin(iIO);
                continue;
            }
            AvoidanceInterface *iAvoid = qobject_cast<AvoidanceInterface *>(plugin);
            if (iAvoid)
            {
                AddPlugin(iAvoid, SLOT(ChangeActiveOptions()));
                continue;
            }
        }
    }
}

void MLDemos::SetTextFontSize()
{
#if defined(Q_OS_MAC)
    return; // default fontsizes are for mac already ;)
#endif
    QFont font("Lucida Sans Unicode", 7);
    QList<QWidget*> children = algorithmWidget->findChildren<QWidget*>();
    FOR(i, children.size())
    {
        if(children[i]) children[i]->setFont(font);
    }
    optionsMaximize->gaussianButton->setFont(QFont("Lucida Sans Unicode", 18));
    optionsMaximize->gradientButton->setFont(QFont("Lucida Sans Unicode", 18));
    optionsMaximize->targetButton->setFont(QFont("Lucida Sans Unicode", 18));
}

void MLDemos::ShowContextMenuSpray(const QPoint &point)
{
    QPoint pt = QPoint(30, 0);
    drawContext1Widget->move(drawToolbar->sprayButton->mapToGlobal(pt));
    drawContext1Widget->show();
    drawContext1Widget->setFocus();
}
void MLDemos::ShowContextMenuLine(const QPoint &point)
{
    QPoint pt = QPoint(30, 0);
    drawContext2Widget->move(drawToolbar->lineButton->mapToGlobal(pt));
    drawContext2Widget->show();
    drawContext2Widget->setFocus();
}
void MLDemos::ShowContextMenuEllipse(const QPoint &point)
{
    QPoint pt = QPoint(30, 0);
    drawContext2Widget->move(drawToolbar->ellipseButton->mapToGlobal(pt));
    drawContext2Widget->show();
    drawContext2Widget->setFocus();
}
void MLDemos::ShowContextMenuErase(const QPoint &point)
{
    QPoint pt = QPoint(30, 0);
    drawContext1Widget->move(drawToolbar->eraseButton->mapToGlobal(pt));
    drawContext1Widget->show();
    drawContext1Widget->setFocus();
}
void MLDemos::ShowContextMenuObstacle(const QPoint &point)
{
    QPoint pt = QPoint(30, 0);
    drawContext3Widget->move(drawToolbar->obstacleButton->mapToGlobal(pt));
    drawContext3Widget->show();
    drawContext3Widget->setFocus();
}
void MLDemos::ShowContextMenuReward(const QPoint &point)
{
    QPoint pt = QPoint(30, 0);
    drawContext4Widget->move(drawToolbar->paintButton->mapToGlobal(pt));
    drawContext4Widget->show();
    drawContext4Widget->setFocus();
}

bool IsChildOf(QObject *child, QObject *parent)
{
    if(!parent || !child) return false;
    if(child == parent) return true;
    QList<QObject*> list = parent->children();
    if(list.isEmpty()) return false;
    QList<QObject*>::iterator i;
    for (i = list.begin(); i<list.end(); ++i)
    {
        if(IsChildOf(child, *i)) return true;
    }
    return false;
}

void MLDemos::FocusChanged(QWidget *old, QWidget *now)
{
    if(drawContext1Widget->isVisible())
    {
        if(!IsChildOf(now, drawContext1Widget)) HideContextMenus();
    }
    if(drawContext2Widget->isVisible())
    {
        if(!IsChildOf(now, drawContext2Widget)) HideContextMenus();
    }
    if(drawContext3Widget->isVisible())
    {
        if(!IsChildOf(now, drawContext3Widget)) HideContextMenus();
    }
    if(drawContext4Widget->isVisible())
    {
        if(!IsChildOf(now, drawContext4Widget)) HideContextMenus();
    }
}

void MLDemos::HideContextMenus()
{
    drawContext1Widget->hide();
    drawContext2Widget->hide();
    drawContext3Widget->hide();
	drawContext4Widget->hide();
}

void MLDemos::AddPlugin(InputOutputInterface *iIO)
{
    inputoutputs.push_back(iIO);
    bInputRunning.push_back(false);
    connect(this, SIGNAL(SendResults(std::vector<fvec>)), iIO->object(), iIO->FetchResultsSlot());
    connect(iIO->object(), iIO->SetDataSignal(), this, SLOT(SetData(std::vector<fvec>, ivec, std::vector<ipair>)));
    connect(iIO->object(), iIO->QueryClassifierSignal(), this, SLOT(QueryClassifier(std::vector<fvec>)));
    connect(iIO->object(), iIO->QueryRegressorSignal(), this, SLOT(QueryRegressor(std::vector<fvec>)));
    connect(iIO->object(), iIO->QueryDynamicalSignal(), this, SLOT(QueryDynamical(std::vector<fvec>)));
    connect(iIO->object(), iIO->QueryClustererSignal(), this, SLOT(QueryClusterer(std::vector<fvec>)));
    connect(iIO->object(), iIO->QueryMaximizerSignal(), this, SLOT(QueryMaximizer(std::vector<fvec>)));
    connect(iIO->object(), iIO->DoneSignal(), this, SLOT(DisactivateIO(QObject *)));
    QString name = iIO->GetName();
    QAction *pluginAction = ui.menuInput_Output->addAction(name);
    pluginAction->setCheckable(true);
    pluginAction->setChecked(false);
    connect(pluginAction,SIGNAL(toggled(bool)), this, SLOT(ActivateIO()));
}

void MLDemos::AddPlugin(ClassifierInterface *iClassifier, const char *method)
{
    if(!iClassifier) return;
    // we add the interface so we can use it to produce classifiers
    classifiers.push_back(iClassifier);
    // we add the classifier parameters to the gui
    optionsClassify->tabWidget->addTab(iClassifier->GetParameterWidget(), iClassifier->GetName());
}

void MLDemos::AddPlugin(ClustererInterface *iCluster, const char *method)
{
    if(!iCluster) return;
    clusterers.push_back(iCluster);
    optionsCluster->tabWidget->addTab(iCluster->GetParameterWidget(), iCluster->GetName());
}

void MLDemos::AddPlugin(RegressorInterface *iRegress, const char *method)
{
    if(!iRegress) return;
    regressors.push_back(iRegress);
    optionsRegress->tabWidget->addTab(iRegress->GetParameterWidget(), iRegress->GetName());
}

void MLDemos::AddPlugin(DynamicalInterface *iDynamical, const char *method)
{
    if(!iDynamical) return;
    dynamicals.push_back(iDynamical);
    optionsDynamic->tabWidget->addTab(iDynamical->GetParameterWidget(), iDynamical->GetName());
}

void MLDemos::AddPlugin(AvoidanceInterface *iAvoid, const char *method)
{
    if(!iAvoid) return;
    avoiders.push_back(iAvoid);
    optionsDynamic->obstacleCombo->addItem(iAvoid->GetName());
}

void MLDemos::AddPlugin(MaximizeInterface *iMaximizer, const char *method)
{
    if(!iMaximizer) return;
    maximizers.push_back(iMaximizer);
    optionsMaximize->tabWidget->addTab(iMaximizer->GetParameterWidget(), iMaximizer->GetName());
}

MLDemos::~MLDemos()
{
    Clear();
    FOR(i, inputoutputs.size())
    {
        if(inputoutputs[i] && bInputRunning[i]) inputoutputs[i]->Stop();
    }
    SaveLayoutOptions();
    delete optionsClassify;
    delete optionsRegress;
    delete optionsCluster;
    delete optionsDynamic;
    delete optionsMaximize;
    delete drawToolbar;
    delete drawToolbarContext1;
    delete drawToolbarContext2;
    delete displayOptions;

    canvas->hide();
    delete canvas;
}

void MLDemos::closeEvent(QCloseEvent *event)
{
    if (true)
    {
        mutex.lock();
        DEL(regressor);
        DEL(classifier);
        mutex.unlock();
        qApp->quit();
    } else {
        event->ignore();
    }
}

void MLDemos::resizeEvent( QResizeEvent *event )
{
    if(canvas)
    {
        canvas->resize(ui.canvasWidget->width(), ui.canvasWidget->height());
        canvas->ResizeEvent();
    }
    CanvasMoveEvent();
}

void MLDemos::AlgoChanged()
{
    ChangeInfoFile();
    actionClassifiers->setChecked(algorithmOptions->tabClass->isVisible());
    actionClustering->setChecked(algorithmOptions->tabClust->isVisible());
    actionRegression->setChecked(algorithmOptions->tabRegr->isVisible());
    actionDynamical->setChecked(algorithmOptions->tabDyn->isVisible());
    actionMaximizers->setChecked(algorithmOptions->tabMax->isVisible());
    if(actionMaximizers->isChecked())
    {
        drawToolbar->paintButton->setChecked(true);
        DrawPaint();
    }
    if(actionDynamical->isChecked())
    {
        drawToolbar->trajectoryButton->setChecked(true);
        DrawTrajectory();
    }
    if(actionRegression->isChecked() || actionClassifiers->isChecked())
    {
        drawToolbar->sprayButton->setChecked(true);
        DrawSpray();
    }
}

void MLDemos::CompareAdd()
{
    if(algorithmOptions->tabClass->isVisible())
    {
        int tab = optionsClassify->tabWidget->currentIndex();
        QString name = classifiers[tab]->GetAlgoString();
        QString parameterData;
        QTextStream stream(&parameterData, QIODevice::WriteOnly);
        stream << "Classification" << ":" << tab << "\n";
        classifiers[tab]->SaveParams(stream);
        optionsCompare->algoList->addItem(name);
        compareOptions.push_back(parameterData);
    }
    if(algorithmOptions->tabRegr->isVisible())
    {
        int tab = optionsRegress->tabWidget->currentIndex();
        QString name = regressors[tab]->GetAlgoString();
        QString parameterData;
        QTextStream stream(&parameterData, QIODevice::WriteOnly);
        stream << "Regression" << ":" << tab << "\n";
        regressors[tab]->SaveParams(stream);
        optionsCompare->algoList->addItem(name);
        compareOptions.push_back(parameterData);
    }
    if(algorithmOptions->tabDyn->isVisible())
    {
        int tab = optionsDynamic->tabWidget->currentIndex();
        QString name = dynamicals[tab]->GetAlgoString();
        QString parameterData;
        QTextStream stream(&parameterData, QIODevice::WriteOnly);
        stream << "Dynamical" << ":" << tab << "\n";
        dynamicals[tab]->SaveParams(stream);
        optionsCompare->algoList->addItem(name);
        compareOptions.push_back(parameterData);
    }
    if(algorithmOptions->tabMax->isVisible())
    {
        int tab = optionsMaximize->tabWidget->currentIndex();
        QString name = maximizers[tab]->GetAlgoString();
        QString parameterData;
        QTextStream stream(&parameterData, QIODevice::WriteOnly);
        stream << "Maximization" << ":" << tab << "\n";
        maximizers[tab]->SaveParams(stream);
        optionsCompare->algoList->addItem(name);
        compareOptions.push_back(parameterData);
    }
    actionCompare->setChecked(true);
    compareWidget->show();
}

void MLDemos::CompareClear()
{
    optionsCompare->algoList->clear();
    compareOptions.clear();
    if(compareDisplay) compareDisplay->hide();
}

void MLDemos::CompareRemove()
{
    int offset = 0;
    FOR(i, optionsCompare->algoList->count())
    {
        if(optionsCompare->algoList->item(i)->isSelected())
        {
            compareOptions.removeAt(i-offset);
            offset++;
        }
    }

    QList<QListWidgetItem *> selected = optionsCompare->algoList->selectedItems();
    FOR(i, selected.size()) delete selected[i];
    if(optionsCompare->algoList->count()) optionsCompare->algoList->item(0)->setSelected(true);
}

void MLDemos::ShowOptionClass()
{
    if(actionClassifiers->isChecked())
    {
        algorithmOptions->tabWidget->setCurrentWidget(algorithmOptions->tabClass);
        algorithmWidget->show();
    }
    else algorithmWidget->hide();
    actionClustering->setChecked(false);
    actionRegression->setChecked(false);
    actionDynamical->setChecked(false);
    actionMaximizers->setChecked(false);
}

void MLDemos::ShowOptionRegress()
{
    if(actionRegression->isChecked())
    {
        algorithmOptions->tabWidget->setCurrentWidget(algorithmOptions->tabRegr);
        algorithmWidget->show();
    }
    else algorithmWidget->hide();
    actionClustering->setChecked(false);
    actionClassifiers->setChecked(false);
    actionDynamical->setChecked(false);
    actionMaximizers->setChecked(false);
}

void MLDemos::ShowOptionDynamical()
{
    if(actionDynamical->isChecked())
    {
        algorithmOptions->tabWidget->setCurrentWidget(algorithmOptions->tabDyn);
        algorithmWidget->show();
        drawToolbar->trajectoryButton->setChecked(true);
        DrawTrajectory();
    }
    else algorithmWidget->hide();
    actionClustering->setChecked(false);
    actionClassifiers->setChecked(false);
    actionRegression->setChecked(false);
    actionMaximizers->setChecked(false);
}

void MLDemos::ShowOptionCluster()
{
    if(actionClustering->isChecked())
    {
        algorithmOptions->tabWidget->setCurrentWidget(algorithmOptions->tabClust);
        algorithmWidget->show();
    }
    else algorithmWidget->hide();
    actionClassifiers->setChecked(false);
    actionRegression->setChecked(false);
    actionDynamical->setChecked(false);
    actionMaximizers->setChecked(false);
}

void MLDemos::ShowOptionMaximize()
{
    if(actionMaximizers->isChecked())
    {
        algorithmOptions->tabWidget->setCurrentWidget(algorithmOptions->tabMax);
        algorithmWidget->show();
        drawToolbar->paintButton->setChecked(true);
        DrawPaint();
    }
    else algorithmWidget->hide();
    actionClustering->setChecked(false);
    actionClassifiers->setChecked(false);
    actionRegression->setChecked(false);
    actionDynamical->setChecked(false);
}

void MLDemos::ShowOptionCompare()
{
    if(actionCompare->isChecked())
    {
        compareWidget->show();
    }
    else
    {
        compareWidget->hide();
    }
}

void MLDemos::ShowSampleDrawing()
{
    if(actionDrawSamples->isChecked())
    {
        drawToolbarWidget->show();
    }
    else
    {
        drawToolbarWidget->hide();
    }
}

void MLDemos::ShowOptionDisplay()
{
    if(actionDisplayOptions->isChecked()) displayDialog->show();
    else displayDialog->hide();
}

void MLDemos::ShowToolbar()
{
    if(ui.actionSmall_Icons->isChecked())
    {
        toolBar->setIconSize(QSize(32,32));
        toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    else
    {
        toolBar->setIconSize(QSize(64,64));
        toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    }
    if(ui.actionShow_Toolbar->isChecked()) toolBar->show();
    else toolBar->hide();
}

void MLDemos::ShowStatsDialog()
{
    if(actionShowStats->isChecked()) statsDialog->show();
    else statsDialog->hide();
}

void MLDemos::ShowAbout()
{
    about->show();
}

void MLDemos::HideOptionClass()
{
    if(algorithmOptions->tabClass->isVisible()) algorithmWidget->hide();
    actionClassifiers->setChecked(false);
}

void MLDemos::HideOptionRegress()
{
    if(algorithmOptions->tabRegr->isVisible()) algorithmWidget->hide();
    actionRegression->setChecked(false);
}

void MLDemos::HideOptionDynamical()
{
    if(algorithmOptions->tabDyn->isVisible()) algorithmWidget->hide();
    actionDynamical->setChecked(false);
}

void MLDemos::HideOptionCluster()
{
    if(algorithmOptions->tabClust->isVisible()) algorithmWidget->hide();
    actionClustering->setChecked(false);
}

void MLDemos::HideOptionMaximize()
{
    if(algorithmOptions->tabMax->isVisible()) algorithmWidget->hide();
    actionMaximizers->setChecked(false);
}

void MLDemos::HideSampleDrawing()
{
    drawToolbarWidget->hide();
    actionDrawSamples->setChecked(false);
}

void MLDemos::HideOptionDisplay()
{
    displayDialog->hide();
    actionDisplayOptions->setChecked(false);
}

void MLDemos::HideToolbar()
{
    toolBar->hide();
    ui.actionShow_Toolbar->setChecked(false);
}

void MLDemos::HideStatsDialog()
{
    statsDialog->hide();
    actionShowStats->setChecked(false);
}


void MLDemos::Clear()
{
    drawTimer->Stop();
    drawTimer->Clear();
    QMutexLocker lock(&mutex);
    qApp->processEvents();
    DEL(classifier);
    DEL(regressor);
    DEL(dynamical);
    DEL(clusterer);
    DEL(maximizer);
    canvas->confidencePixmap = QPixmap();
    canvas->modelPixmap = QPixmap();
    canvas->infoPixmap = QPixmap();
    canvas->liveTrajectory.clear();
    canvas->repaint();
    UpdateInfo();
}

void MLDemos::ResetPositiveClass()
{
    int labMin = INT_MAX, labMax = INT_MIN;
    if(!canvas->data->GetCount())
    {
        labMin = 0;
        labMax = 1;
    }
    else
    {
        ivec labels = canvas->data->GetLabels();
        FOR(i, labels.size())
        {
            if(labels[i] > labMax) labMax = labels[i];
            if(labels[i] < labMin) labMin = labels[i];
        }
    }
    optionsClassify->positiveSpin->setRange(labMin,labMax);
    if(optionsClassify->positiveSpin->value() < labMin)
        optionsClassify->positiveSpin->setValue(labMin);
    else if(optionsClassify->positiveSpin->value() > labMax)
        optionsClassify->positiveSpin->setValue(labMax);
	int dimCount = max(2,canvas->data->GetDimCount());
	displayOptions->xDimIndex->setRange(0,dimCount-1);
	displayOptions->yDimIndex->setRange(0,dimCount-1);
	canvas->SetDim(displayOptions->xDimIndex->value(), displayOptions->yDimIndex->value());
}

void MLDemos::ChangeActiveOptions()
{
    DisplayOptionChanged();
}

void MLDemos::ClearData()
{
    if(canvas)
    {
        canvas->data->Clear();
        canvas->targets.clear();
        canvas->rewardPixmap = QPixmap();
    }
    Clear();
    ResetPositiveClass();
    UpdateInfo();
}

void MLDemos::DrawSingle()
{
    if(drawToolbar->singleButton->isChecked())
    {
        drawToolbar->sprayButton->setChecked(false);
        drawToolbar->eraseButton->setChecked(false);
        drawToolbar->ellipseButton->setChecked(false);
        drawToolbar->lineButton->setChecked(false);
        drawToolbar->trajectoryButton->setChecked(false);
        drawToolbar->obstacleButton->setChecked(false);
        drawToolbar->paintButton->setChecked(false);
    }
}

void MLDemos::DrawSpray()
{
    if(drawToolbar->sprayButton->isChecked())
    {
        drawToolbar->singleButton->setChecked(false);
        drawToolbar->eraseButton->setChecked(false);
        drawToolbar->ellipseButton->setChecked(false);
        drawToolbar->lineButton->setChecked(false);
        drawToolbar->trajectoryButton->setChecked(false);
        drawToolbar->obstacleButton->setChecked(false);
        drawToolbar->paintButton->setChecked(false);
    }
}

void MLDemos::DrawErase()
{
    if(drawToolbar->eraseButton->isChecked())
    {
        drawToolbar->singleButton->setChecked(false);
        drawToolbar->sprayButton->setChecked(false);
        drawToolbar->ellipseButton->setChecked(false);
        drawToolbar->lineButton->setChecked(false);
        drawToolbar->trajectoryButton->setChecked(false);
        drawToolbar->obstacleButton->setChecked(false);
        drawToolbar->paintButton->setChecked(false);
    }
}

void MLDemos::DrawLine()
{
    if(drawToolbar->lineButton->isChecked())
    {
        drawToolbar->singleButton->setChecked(false);
        drawToolbar->sprayButton->setChecked(false);
        drawToolbar->ellipseButton->setChecked(false);
        drawToolbar->eraseButton->setChecked(false);
        drawToolbar->trajectoryButton->setChecked(false);
        drawToolbar->obstacleButton->setChecked(false);
        drawToolbar->paintButton->setChecked(false);
    }
}

void MLDemos::DrawEllipse()
{
    if(drawToolbar->ellipseButton->isChecked())
    {
        drawToolbar->singleButton->setChecked(false);
        drawToolbar->sprayButton->setChecked(false);
        drawToolbar->eraseButton->setChecked(false);
        drawToolbar->lineButton->setChecked(false);
        drawToolbar->trajectoryButton->setChecked(false);
        drawToolbar->obstacleButton->setChecked(false);
        drawToolbar->paintButton->setChecked(false);
    }
}

void MLDemos::DrawTrajectory()
{
    if(drawToolbar->trajectoryButton->isChecked())
    {
        drawToolbar->singleButton->setChecked(false);
        drawToolbar->sprayButton->setChecked(false);
        drawToolbar->eraseButton->setChecked(false);
        drawToolbar->lineButton->setChecked(false);
        drawToolbar->ellipseButton->setChecked(false);
        drawToolbar->obstacleButton->setChecked(false);
        drawToolbar->paintButton->setChecked(false);
    }
}

void MLDemos::DrawObstacle()
{
    if(drawToolbar->obstacleButton->isChecked())
    {
        drawToolbar->eraseButton->setChecked(false);
        drawToolbar->singleButton->setChecked(false);
        drawToolbar->sprayButton->setChecked(false);
        drawToolbar->ellipseButton->setChecked(false);
        drawToolbar->lineButton->setChecked(false);
        drawToolbar->trajectoryButton->setChecked(false);
        drawToolbar->paintButton->setChecked(false);
    }
}

void MLDemos::DrawPaint()
{
    if(drawToolbar->paintButton->isChecked())
    {
        drawToolbar->eraseButton->setChecked(false);
        drawToolbar->singleButton->setChecked(false);
        drawToolbar->sprayButton->setChecked(false);
        drawToolbar->ellipseButton->setChecked(false);
        drawToolbar->lineButton->setChecked(false);
        drawToolbar->trajectoryButton->setChecked(false);
        drawToolbar->obstacleButton->setChecked(false);
    }
}

void MLDemos::AvoidOptionChanged()
{
    if(dynamical)
    {
        int avoidIndex = optionsDynamic->obstacleCombo->currentIndex();
        mutex.lock();
        if(dynamical->avoid) delete dynamical->avoid;
        if(!avoidIndex) dynamical->avoid = 0;
        else dynamical->avoid = avoiders[avoidIndex-1]->GetObstacleAvoidance();
        mutex.unlock();
        drawTimer->Stop();
        drawTimer->Clear();
        drawTimer->start(QThread::NormalPriority);
    }
}

void MLDemos::ColorMapChanged()
{
	if(dynamical)
	{
		drawTimer->Stop();
		drawTimer->Clear();
		drawTimer->bColorMap = optionsDynamic->colorCheck->isChecked();
		drawTimer->start(QThread::NormalPriority);
	}
}

void MLDemos::DisplayOptionChanged()
{
    if(!canvas) return;
    canvas->bDisplayInfo = displayOptions->infoCheck->isChecked();
    canvas->bDisplayLearned = displayOptions->modelCheck->isChecked();
    canvas->bDisplayMap = displayOptions->mapCheck->isChecked();
    canvas->bDisplaySamples = displayOptions->samplesCheck->isChecked();
    canvas->bDisplayTrajectories = displayOptions->samplesCheck->isChecked();
    canvas->bDisplayGrid = displayOptions->gridCheck->isChecked();
	{
		int xIndex = displayOptions->xDimIndex->value();
		int yIndex = displayOptions->yDimIndex->value();
		if(xIndex == yIndex)
		{
			yIndex = xIndex+1;
		}
		canvas->SetDim(xIndex, yIndex);
	}
    float zoom = displayOptions->spinZoom->value();
    if(zoom >= 0.f) zoom += 1.f;
    else zoom = 1.f / (fabs(zoom)+1.f);
    if(zoom != canvas->GetZoom())
    {
        drawTimer->Stop();
        drawTimer->Clear();
        canvas->SetZoom(zoom);
        QMutexLocker lock(&mutex);
        if(classifier)
        {
            classifiers[tabUsedForTraining]->Draw(canvas, classifier);
            if(classifier->UsesDrawTimer())
            {
                drawTimer->start(QThread::NormalPriority);
            }
        }
        else if(regressor)
        {
            regressors[tabUsedForTraining]->Draw(canvas, regressor);
            //drawTimer->start(QThread::NormalPriority);
        }
        else if(clusterer)
        {
            clusterers[tabUsedForTraining]->Draw(canvas, clusterer);
            drawTimer->start(QThread::NormalPriority);
        }
        else if(dynamical)
        {
            dynamicals[tabUsedForTraining]->Draw(canvas, dynamical);
            if(dynamicals[tabUsedForTraining]->UsesDrawTimer()) drawTimer->start(QThread::NormalPriority);
        }
        else if(maximizer)
        {
            drawTimer->start(QThread::NormalPriority);
        }
        canvas->repaint();
    }
    //	canvas->bDisplayTrajectories = displayOptions->trajectoriesCheck->isChecked();
    if(optionsDynamic)
    {
        canvas->trajectoryCenterType = optionsDynamic->centerCombo->currentIndex();
        canvas->trajectoryResampleType = optionsDynamic->resampleCombo->currentIndex();
        canvas->trajectoryResampleCount = optionsDynamic->resampleSpin->value();
    }
    canvas->ResetSamples();
    canvas->repaint();
}

void MLDemos::ChangeInfoFile()
{
    QString infoFile;
    if(algorithmOptions->tabClass->isVisible())
    {
        int tab = optionsClassify->tabWidget->currentIndex();
        if(tab < 0 || tab >= (int)classifiers.size() || !classifiers[tab]) return;
        infoFile = classifiers[tab]->GetInfoFile();
    }
    if(algorithmOptions->tabClust->isVisible())
    {
        int tab = optionsCluster->tabWidget->currentIndex();
        if(tab < 0 || tab >= (int)clusterers.size() || !clusterers[tab]) return;
        infoFile = clusterers[tab]->GetInfoFile();
    }
    if(algorithmOptions->tabRegr->isVisible())
    {
        int tab = optionsRegress->tabWidget->currentIndex();
        if(tab < 0 || tab >= (int)regressors.size() || !regressors[tab]) return;
        infoFile = regressors[tab]->GetInfoFile();
    }
    if(algorithmOptions->tabDyn->isVisible())
    {
        int tab = optionsDynamic->tabWidget->currentIndex();
        if(tab < 0 || tab >= (int)dynamicals.size() || !dynamicals[tab]) return;
        infoFile = dynamicals[tab]->GetInfoFile();
    }
    if(algorithmOptions->tabMax->isVisible())
    {
        int tab = optionsMaximize->tabWidget->currentIndex();
        if(tab < 0 || tab >= (int)maximizers.size() || !maximizers[tab]) return;
        infoFile = maximizers[tab]->GetInfoFile();
    }
    if(infoFile == "") infoFile = "mldemos.html"; // we want the main information page

    QDir helpDir = QDir(qApp->applicationDirPath());
    QDir alternativeDir = helpDir;
#if defined(Q_OS_WIN)
    if (helpDir.dirName().toLower() == "debug" || helpDir.dirName().toLower() == "release") helpDir.cdUp();
#elif defined(Q_OS_MAC)
    if (helpDir.dirName() == "MacOS") {
        if(!helpDir.cd("help"))
        {
            helpDir.cdUp();
            helpDir.cdUp();
            helpDir.cdUp();
            alternativeDir = helpDir;
        }
        else helpDir.cdUp();
    }
#endif
    if(!helpDir.cd("help"))
    {
        //qDebug() << "using alternative directory: " << alternativeDir.absolutePath();
        helpDir = alternativeDir;
        if(!helpDir.cd("help")) return;
    }
    //qDebug() << "using help directory: " << helpDir.absolutePath();

    QString filePath(helpDir.absolutePath() + "/" + infoFile);
    //qDebug() << "loading info from: " << filePath;
    showStats->algoText->clear();
    showStats->algoText->setSource(QUrl::fromLocalFile(filePath));
}

void MLDemos::DrawCrosshair()
{
    int drawType = 0;
    if(drawToolbar->singleButton->isChecked()) drawType = 1;
    if(drawToolbar->sprayButton->isChecked()) drawType = 2;
    if(drawToolbar->eraseButton->isChecked()) drawType = 3;
    if(drawToolbar->ellipseButton->isChecked()) drawType = 4;
    if(drawToolbar->lineButton->isChecked()) drawType = 5;
    if(drawToolbar->trajectoryButton->isChecked()) drawType = 6;
    if(drawToolbar->obstacleButton->isChecked()) drawType = 7;
    if(drawToolbar->paintButton->isChecked()) drawType = 8;

    if(!drawType || drawType == 1 || drawType == 6)
    {
        canvas->crosshair = QPainterPath();
        canvas->bNewCrosshair = false;
        return;
    }
    int type = drawToolbarContext1->randCombo->currentIndex();
    float aX = drawToolbarContext2->spinSigmaX->value();
    float aY = drawToolbarContext2->spinSigmaY->value();
    float angle = -drawToolbarContext2->spinAngle->value()/180.f*PIf;
    float s = drawToolbarContext1->spinSize->value();
    int size = (int)(s*canvas->height());
    int sizeX = (int)(aX*canvas->height());
    int Size = canvas->height();

    QPainterPath cursor;

    float sin_angle = sinf(angle);
    float cos_angle = cosf(angle);

    switch(drawType)
    {
    case 5: // line
    {
        QPointF pStart, pStop;
        float x = cos_angle*aX;
        float y = sin_angle*aX;
        pStart = QPointF(- x*Size, - y*Size);
        pStop = QPointF(+ x*Size, + y*Size);
        cursor.moveTo(pStart);
        cursor.lineTo(pStop);
        canvas->crosshair = cursor;
        canvas->bNewCrosshair = false;
        return;
    }
        break;
    case 2: // spray
    case 3: // erase
    {
        cursor.addEllipse(QPoint(0,0),size/2,size/2);
        canvas->crosshair = cursor;
        canvas->bNewCrosshair = false;
        return;
    }
        break;
    case 7: // obstacles
    {
        Obstacle o;
        o.angle = drawToolbarContext3->spinAngle->value() / 180.f * PIf;
        o.axes.resize(2);
        o.axes[0] = drawToolbarContext3->spinSigmaX->value();
        o.axes[1] = drawToolbarContext3->spinSigmaY->value();
        o.power[0] = drawToolbarContext3->spinPowerX->value();
        o.power[1] = drawToolbarContext3->spinPowerY->value();
        o.repulsion[0] = drawToolbarContext3->spinRepulsionX->value();
        o.repulsion[1] = drawToolbarContext3->spinRepulsionY->value();
        o.center = fVec(0,0);
        canvas->crosshair = canvas->DrawObstacle(o);
        canvas->bNewCrosshair = false;
        return;
    }
        break;
    case 8: // paint
    {
        float radius = drawToolbarContext4->spinRadius->value();
        QPainterPath cursor;
        cursor.addEllipse(QPoint(0,0),radius,radius);
        canvas->crosshair = cursor;
        canvas->bNewCrosshair = false;
        return;
    }
        break;
    }

    QPointF oldPoint, point;
    for(float theta=0; theta < 2*PIf + 0.1; theta += 0.1f)
    {
        float X, Y;
        if(drawType == 2 || drawType == 3)
        {
            X = sqrtf(aX)/2 * cosf(theta);
            Y = sqrtf(aY)/2 * sinf(theta);
        }
        else
        {
            X = aX * cosf(theta);
            Y = aY * sinf(theta);
        }

        float RX = + X * cos_angle + Y * sin_angle;
        float RY = - X * sin_angle + Y * cos_angle;

        point = QPointF(RX*Size,RY*Size);
        if(theta==0)
        {
            cursor.moveTo(point);
            continue;
        }
        cursor.lineTo(point);
        oldPoint = point;
    }
    canvas->crosshair = cursor;
    canvas->bNewCrosshair = false;
}

void MLDemos::Drawing( fvec sample, int label)
{
    int drawType = 0; // none
    if(drawToolbar->singleButton->isChecked()) drawType = 1;
    if(drawToolbar->sprayButton->isChecked()) drawType = 2;
    if(drawToolbar->eraseButton->isChecked()) drawType = 3;
    if(drawToolbar->ellipseButton->isChecked()) drawType = 4;
    if(drawToolbar->lineButton->isChecked()) drawType = 5;
    if(drawToolbar->trajectoryButton->isChecked()) drawType = 6;
    if(drawToolbar->obstacleButton->isChecked()) drawType = 7;
    if(drawToolbar->paintButton->isChecked()) drawType = 8;
    if(!drawType) return;

    int speed = 6;

    if(label) label = drawToolbar->classSpin->value();

    switch(drawType)
    {
    case 1: // single samples
    {
        // we don't want to draw too often
        if(drawTime.elapsed() < 50/speed) return; // msec elapsed since last drawing
        canvas->data->AddSample(sample, label);
    }
        break;
    case 2: // spray samples
    {
        // we don't want to draw too often
        if(drawTime.elapsed() < 200/speed) return; // msec elapsed since last drawing
        int type = drawToolbarContext1->randCombo->currentIndex();
        float s = drawToolbarContext1->spinSize->value();
        float size = s*canvas->height();
        int count = drawToolbarContext1->spinCount->value();

        QPointF sampleCoords = canvas->toCanvasCoords(sample);
        // we generate the new data
        float variance = sqrtf(size*size/9.f*0.5f);
        fvec newSample; newSample.resize(2,0);
        FOR(i, count)
        {
            if(type == 0) // uniform
            {
                newSample[0] = (rand()/(float)RAND_MAX - 0.5f)*size + sampleCoords.x();
                newSample[1] = (rand()/(float)RAND_MAX - 0.5f)*size + sampleCoords.y();
            }
            else // normal
            {
                newSample[0] = RandN((float)sampleCoords.x(), variance);
                newSample[1] = RandN((float)sampleCoords.y(), variance);
            }
            fvec canvasSample = canvas->toSampleCoords(newSample[0],newSample[1]);
            //canvasSample.push_back(label ? RandN(1.f,0.5f) : RandN(-1.f,0.5f));
            //canvasSample.push_back(label ? RandN(-.5f,0.5f) : RandN(.5f,0.5f));
            canvas->data->AddSample(canvasSample, label);
        }
    }
        break;
    case 3: // erase
    {
        float s = drawToolbarContext1->spinSize->value();
        float size = s*canvas->height();
        QPointF center = canvas->toCanvasCoords(sample);
        bool anythingDeleted = canvas->DeleteData(center, size/2);
        if(anythingDeleted)
        {
            drawTimer->Stop();
            drawTimer->Clear();
            QMutexLocker lock(&mutex);
            if(dynamical && dynamical->avoid) dynamical->avoid->SetObstacles(canvas->data->GetObstacles());
            drawTimer->start(QThread::NormalPriority);
            canvas->ResetSamples();
        }
    }
        break;
    case 4: // ellipse
    {
        if(drawTime.elapsed() < 200/speed) return; // msec elapsed since last drawing
        float aX = drawToolbarContext2->spinSigmaX->value();
        float aY = drawToolbarContext2->spinSigmaY->value();
        float angle = -drawToolbarContext2->spinAngle->value()/180.f*PIf;
        int count = drawToolbarContext1->spinCount->value()+1;
        float sin_angle = sinf(angle);
        float cos_angle = cosf(angle);

        QPointF oldPoint, point;
        float startTheta = rand()/(float)RAND_MAX*2*PIf;
        for(float theta=0; theta < 2*PIf; theta += 2.f*PIf/count)
        {
            float X = aX * cosf(theta+startTheta);
            float Y = aY * sinf(theta+startTheta);

            float RX = + X * cos_angle + Y * sin_angle;
            float RY = - X * sin_angle + Y * cos_angle;

            fvec newSample;
            newSample.resize(2,0);
            newSample[0] = sample[0] + RX;
            newSample[1] = sample[1] + RY;
            if(theta==0)
            {
                oldPoint = point;
                continue;
            }
            canvas->data->AddSample(newSample, label);

            oldPoint = point;
        }
    }
        break;
    case 5: // line
    {
        if(drawTime.elapsed() < 200/speed) return; // msec elapsed since last drawing
        float aX = drawToolbarContext2->spinSigmaX->value();
        float angle = -drawToolbarContext2->spinAngle->value()/180.f*PIf;
        int count = drawToolbarContext1->spinCount->value();
        float sin_angle = sinf(angle);
        float cos_angle = cosf(angle);

        QPointF pStart, pStop;
        float x = cos_angle*aX;
        float y = sin_angle*aX;
        pStart = QPointF(sample[0] - x, sample[1] - y);
        pStop = QPointF(sample[0] + x, sample[1] + y);
        QPointF oldPoint = pStart;
        float start = (rand() / (float)RAND_MAX - 0.5) * (1/(float)count);
        FOR(i,count)
        {
            QPointF point = (pStop - pStart)*((i+1)/(float)count + start) + pStart;
            fvec newSample;
            newSample.resize(2);
            newSample[0] = point.x();
            newSample[1] = point.y();
            canvas->data->AddSample(newSample, label);
            oldPoint = point;
        }
    }
        break;
    case 6: // trajectory
    {
        if(trajectory.first == -1) // we're starting a trajectory
        {
            trajectory.first = canvas->data->GetCount();
        }
        // we don't want to draw too often
        //if(drawTime.elapsed() < 50/speed) return; // msec elapsed since last drawing
        canvas->data->AddSample(sample, label, _TRAJ);
        trajectory.second = canvas->data->GetCount()-1;
    }
        break;
    case 7: // obstacle
    {
        bNewObstacle = true;
        obstacle = Obstacle();
        obstacle.angle = drawToolbarContext3->spinAngle->value() / 180.f * PIf;
        obstacle.power[0] = drawToolbarContext3->spinPowerX->value();
        obstacle.power[1] = drawToolbarContext3->spinPowerY->value();
        obstacle.center = sample;
        obstacle.axes[0] = drawToolbarContext3->spinSigmaX->value();
        obstacle.axes[1] = drawToolbarContext3->spinSigmaY->value();
        obstacle.repulsion[0] = drawToolbarContext3->spinRepulsionX->value();
        obstacle.repulsion[1] = drawToolbarContext3->spinRepulsionX->value();
    }
        break;
    case 8: // paint rewards
    {
        float radius = drawToolbarContext4->spinRadius->value();
        float alpha = drawToolbarContext4->spinAlpha->value();
        canvas->PaintReward(sample, radius, label ? alpha : -alpha);
        /*
  // if we need to initialize the reward map
  if(!canvas->data->GetReward()->rewards)
  {
   ivec size;
   size.resize(2, 64);
   int length = size[0]*size[1];
   float *values = new float[length];
   FOR(i, length) values[i] = rand()/(float)RAND_MAX - 0.5f;
   canvas->data->GetReward()->SetReward(values, size, canvas->canvasTopLeft(), canvas->canvasBottomRight());
   delete [] values;
  }
  canvas->data->GetReward()->ShiftValueAt(sample, 0.2, label ? 0.33 : -0.33);
  //qDebug() << canvas->data->GetReward()->ValueAt(sample);
  */
    }
        break;
    }
    canvas->repaint();
    drawTime.restart();
    ResetPositiveClass();
    UpdateInfo();
}

void MLDemos::DrawingStopped()
{
    if(trajectory.first != -1)
    {
        // the last point is a duplicate, we take it out
        canvas->data->AddSequence(trajectory);
        canvas->drawnTrajectories = 0;
        trajectory.first = -1;
        canvas->repaint();
    }
    if(bNewObstacle)
    {
        bNewObstacle = false;
        canvas->data->AddObstacle(obstacle);
        canvas->repaint();
        if(dynamical && dynamical->avoid)
        {
            drawTimer->Stop();
            drawTimer->Clear();
            drawTimer->start(QThread::NormalPriority);
        }
    }
}

void MLDemos::FitToData()
{
    canvas->FitToData();
    float zoom = canvas->GetZoom();
    if(zoom >= 1) zoom -=1;
    else zoom = 1/(-zoom) - 1;
    if(zoom == displayOptions->spinZoom->value()) return; // nothing to be done!
    displayOptions->spinZoom->blockSignals(true);
    displayOptions->spinZoom->setValue(zoom);
    displayOptions->spinZoom->blockSignals(false);
    drawTimer->Stop();
    drawTimer->Clear();
    QMutexLocker lock(&mutex);
    if(classifier)
    {
        classifiers[tabUsedForTraining]->Draw(canvas, classifier);
        if(classifier->UsesDrawTimer())
        {
            drawTimer->start(QThread::NormalPriority);
        }
    }
    else if(regressor)
    {
        regressors[tabUsedForTraining]->Draw(canvas, regressor);
        //drawTimer->start(QThread::NormalPriority);
    }
    else if(clusterer)
    {
        clusterers[tabUsedForTraining]->Draw(canvas, clusterer);
        drawTimer->start(QThread::NormalPriority);
    }
    else if(dynamical)
    {
        dynamicals[tabUsedForTraining]->Draw(canvas, dynamical);
        if(dynamicals[tabUsedForTraining]->UsesDrawTimer()) drawTimer->start(QThread::NormalPriority);
    }
    canvas->repaint();
}

void MLDemos::CanvasMoveEvent()
{
    drawTimer->Stop();
    drawTimer->Clear();
    QMutexLocker lock(&mutex);
    if(classifier)
    {
        classifiers[tabUsedForTraining]->Draw(canvas, classifier);
        if(classifier->UsesDrawTimer())
        {
            drawTimer->start(QThread::NormalPriority);
        }
    }
    else if(regressor)
    {
        regressors[tabUsedForTraining]->Draw(canvas, regressor);
        //drawTimer->start(QThread::NormalPriority);
    }
    else if(clusterer)
    {
        clusterers[tabUsedForTraining]->Draw(canvas, clusterer);
        drawTimer->start(QThread::NormalPriority);
    }
    else if(dynamical)
    {
        dynamicals[tabUsedForTraining]->Draw(canvas, dynamical);
        if(dynamicals[tabUsedForTraining]->UsesDrawTimer()) drawTimer->start(QThread::NormalPriority);
    }
    canvas->repaint();
}

void MLDemos::ZoomChanged(float d)
{
    displayOptions->spinZoom->setValue(displayOptions->spinZoom->value()+d/4);
}

void MLDemos::Navigation( fvec sample )
{
    if(sample[0]==-1)
    {
        ZoomChanged(sample[1]);
        return;
    }
    QString information;
    char string[255];
    int count = canvas->data->GetCount();
    int pcount = 0, ncount = 0;
    ivec labels = canvas->data->GetLabels();
    FOR(i, labels.size())
    {
        if(labels[i] == 0) ++pcount;
        else ++ncount;
    }
    sprintf(string, "samples: %d (o:%.3d|x:%.3d)", count, pcount, ncount);
    information += QString(string);
    sprintf(string, " | x: %.3f y: %.3f", sample[0], sample[1]);
    information += QString(string);
    mutex.tryLock(500);
    if(classifier)
    {
		float score;
		if(classifier->IsMultiClass())
		{
			fvec res = classifier->TestMulti(sample);
			int max = 0;
			FOR(i, res.size()) if(res[max] < res[i]) max = i;
			score = max;
		}
		else
		{
			score = classifier->Test(sample);
		}
        drawTimer->bPaused = false;
        sprintf(string, " | value: %.4f", score);
        information += QString(string);
    }
    else if(dynamical)
    {
        // we build the trajectory(by hand)

        int count = 1000;
        std::vector<fvec> trajectory;
        fvec position = sample;
        if(dynamical->avoid) dynamical->avoid->SetObstacles(canvas->data->GetObstacles());
        FOR(i, count)
        {
            trajectory.push_back(position);
            fvec velocity = dynamical->Test(position);
            if(dynamical->avoid)
            {
                fvec newVelocity = dynamical->avoid->Avoid(position, velocity);
                velocity = newVelocity;
            }
            position += velocity*dynamical->dT;
            if(velocity == 0) break;
        }
        canvas->liveTrajectory = trajectory;
        canvas->repaint();
    }
    mutex.unlock();
    ui.statusBar->showMessage(information);
}

void MLDemos::TargetButton()
{
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

    mimeData->setText("Target");
    drag->setMimeData(mimeData);
    QPixmap pixmap(33,33);
    pixmap.fill();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::black, 1.5));
    painter.setBrush(Qt::NoBrush);

    int pad = 4, radius = 8;
    painter.drawEllipse(QPoint(16,16), radius, radius);
    painter.setBrush(Qt::black);
    painter.drawLine(QPoint(16,16) - QPoint(radius,radius), QPoint(16,16) - QPoint(radius+pad,radius+pad));
    painter.drawLine(QPoint(16,16) + QPoint(radius,radius), QPoint(16,16) + QPoint(radius+pad,radius+pad));
    painter.drawLine(QPoint(16,16) - QPoint(radius,-radius), QPoint(16,16) - QPoint(radius+pad,-radius-pad));
    painter.drawLine(QPoint(16,16) + QPoint(radius,-radius), QPoint(16,16) + QPoint(radius+pad,-radius-pad));
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));

    // maximization only allows one target, so we take the others out
    if(algorithmOptions->tabMax->isVisible())
    {
        canvas->targets.clear();
        canvas->repaint();
    }
    Qt::DropAction dropAction = drag->exec();
}

void MLDemos::GaussianButton()
{
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

    mimeData->setText("Gaussian");
    mimeData->setColorData(QVariant(optionsMaximize->varianceSpin->value()));
    drag->setMimeData(mimeData);
    QPixmap pixmap(33,33);
    pixmap.fill();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::black, 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPoint(16,16), 12,12);
    painter.setBrush(Qt::black);
    painter.drawEllipse(QPoint(16,16), 1,1);
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));
    Qt::DropAction dropAction = drag->exec();
}

void MLDemos::GradientButton()
{
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

    mimeData->setText("Gradient");
    drag->setMimeData(mimeData);
    QPixmap pixmap(33,33);
    pixmap.fill();
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::black, 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(QPoint(4,16), QPoint(29,4));
    painter.drawLine(QPoint(4,16), QPoint(29,29));
    painter.drawLine(QPoint(29,4), QPoint(29,29));
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));
    Qt::DropAction dropAction = drag->exec();
}

void MLDemos::BenchmarkButton()
{
    int w = canvas->width(), h = canvas->height();
    int type = optionsMaximize->benchmarkCombo->currentIndex();
    QImage image(w, h, QImage::Format_ARGB32);
    image.fill(qRgb(255,255,255));

    int dim = 2;
    float minSpace = 0.f;
    float maxSpace = 1.f;
    float minVal = FLT_MAX;
    float maxVal = -FLT_MAX;
    switch(type)
    {
    case 0: // griewangk
        minSpace = -60.f;
        maxSpace = 60.f;
        minVal = 0;
        maxVal = 2;
        break;
    case 1: // rastragin
        minSpace = -5.12f;
        maxSpace = 5.12f;
        minVal = 0;
        maxVal = 82;
        break;
    case 2: // schwefel
        minSpace = -500.f;
        maxSpace = 500.f;
        minVal = -838;
        maxVal = 838;
        break;
    case 3: // ackley
        minSpace = -2.f;
        maxSpace = 2.f;
        minVal = 0;
        maxVal = 2.3504;
    case 4: // michalewicz
        minSpace = -2;
        maxSpace = 2;
        minVal = -1.03159;
        maxVal = 5.74;
        //		minVal = -1.03159;
        //		maxVal = 55.74;
    }

    bool bSetValues = minVal == FLT_MAX;
    Eigen::VectorXd x(2);
    fVec point;
    float value = 0;
    FOR(i, w)
    {
        x[0] = i/(float)w*(maxSpace - minSpace) + minSpace;
        FOR(j, h)
        {
            x[1] = j/(float)h*(maxSpace - minSpace) + minSpace;

            switch(type)
            {
            case 0:
                value = griewangk(x)(0);
                break;
            case 1:
                value = rastragin(x)(0);
                break;
            case 2:
                value = schwefel(x)(0);
                break;
            case 3:
                value = ackley(x)(0);
                break;
            case 4:
                value = sixhump(x)(0);
                break;
            }
            if(bSetValues)
            {
                if(value < minVal) minVal = value;
                if(value > maxVal) maxVal = value;
            }
            else
            {
                value = (value-minVal)/(maxVal-minVal);
            }

            int color = 255.f*max(0.f,min(1.f,value));
            image.setPixel(i,j,qRgba(255, color, color, 255));
        }
    }
    if(bSetValues) qDebug() << "minmax: " << minVal << " " << maxVal;

    canvas->rewardPixmap = QPixmap::fromImage(image);
    canvas->repaint();
}

void MLDemos::SaveData()
{
    if(!canvas) return;
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Data"), "", tr("ML Files (*.ml)"));
    if(filename.isEmpty()) return;
    if(!filename.endsWith(".ml")) filename += ".ml";
    Save(filename);
}
void MLDemos :: Save(QString filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        ui.statusBar->showMessage("WARNING: Unable to save file");
        return;
    }
    file.close();
    canvas->data->Save(filename.toAscii());
    if(!canvas->rewardPixmap.isNull()) canvas->rewardPixmap.toImage().save(filename + "-reward.png");
    SaveParams(filename);
    ui.statusBar->showMessage("Data saved successfully");
}

void MLDemos::LoadData()
{
    if(!canvas) return;
    QString filename = QFileDialog::getOpenFileName(this, tr("Load Data"), "", tr("ML Files (*.ml)"));
    if(filename.isEmpty()) return;
    if(!filename.endsWith(".ml")) filename += ".ml";
    Load(filename);
}

void MLDemos::Load(QString filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        ui.statusBar->showMessage("WARNING: Unable to open file");
        return;
    }
    file.close();
    ClearData();
    canvas->data->Load(filename.toAscii());
    LoadParams(filename);
    QImage reward(filename + "-reward.png");
    if(!reward.isNull()) canvas->rewardPixmap = QPixmap::fromImage(reward);
    ui.statusBar->showMessage("Data loaded successfully");
    ResetPositiveClass();
    UpdateInfo();
    canvas->repaint();
}

void MLDemos::dragEnterEvent(QDragEnterEvent *event)
{
    QList<QUrl> dragUrl;
    if(event->mimeData()->hasUrls())
    {
        QList<QUrl> urls = event->mimeData()->urls();
        QStringList dataType;
        dataType << ".ml";
        for(int i=0; i<urls.size(); i++)
        {
            QString filename = urls[i].path();
            for(int j=0; j < dataType.size(); j++)
            {
                if(filename.toLower().endsWith(dataType[j]))
                {
                    dragUrl.push_back(urls[i]);
                    break;
                }
            }
        }
        if(dragUrl.size())
        {
            event->acceptProposedAction();
        }
    }
}

void MLDemos::dropEvent(QDropEvent *event)
{
    if(!event->mimeData()->hasUrls()) return;
    FOR(i, event->mimeData()->urls().length())
    {
        QString filename = event->mimeData()->urls()[i].toLocalFile();
        if(filename.toLower().endsWith(".ml"))
        {
            ClearData();
            canvas->data->Load(filename.toAscii());
            LoadParams(filename);
            ui.statusBar->showMessage("Data loaded successfully");
            ResetPositiveClass();
            UpdateInfo();
            canvas->repaint();
        }
    }
    event->acceptProposedAction();
}

void MLDemos::ExportSVG()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Vector Image"), "", tr("Images (*.svg)"));
    if(filename.isEmpty()) return;
    if(!filename.endsWith(".svg")) filename += ".svg";

    DrawSVG svg(canvas, &mutex);
    svg.classifier = classifier;
    svg.regressor = regressor;
    svg.clusterer = clusterer;
    svg.dynamical = dynamical;
    svg.maximizer = maximizer;
    if(classifier) svg.drawClass = classifiers[tabUsedForTraining];
    if(regressor) svg.drawRegr = regressors[tabUsedForTraining];
    if(dynamical) svg.drawDyn = dynamicals[tabUsedForTraining];
    if(clusterer) svg.drawClust = clusterers[tabUsedForTraining];
    svg.Write(filename);
    ui.statusBar->showMessage("Vector Image saved successfully");
}

void MLDemos::Screenshot()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Screenshot"), "", tr("Images (*.png *.jpg)"));
    if(filename.isEmpty()) return;
    if(!filename.endsWith(".jpg") && !filename.endsWith(".png")) filename += ".png";
    if(!canvas->SaveScreenshot(filename)) ui.statusBar->showMessage("WARNING: Unable to save image");
    else ui.statusBar->showMessage("Image saved successfully");
}

void MLDemos::CompareScreenshot()
{
    if(!canvas || !compare) return;
    QClipboard *clipboard = QApplication::clipboard();
    QPixmap screenshot = compare->Display().copy();
    clipboard->setImage(screenshot.toImage());
    //clipboard->setPixmap(screenshot);
}

void MLDemos::ToClipboard()
{
    QPixmap screenshot = canvas->GetScreenshot();
    if(screenshot.isNull())
    {
        ui.statusBar->showMessage("WARNING: Nothing to copy to clipboard");
        return;
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setImage(screenshot.toImage());
    clipboard->setPixmap(screenshot);
    ui.statusBar->showMessage("Image copied successfully to clipboard");

}

/************************************/
/*                                  */
/*      Input Output functions      */
/*                                  */
/************************************/

void MLDemos::ActivateIO()
{
    QList<QAction *> pluginActions = ui.menuInput_Output->actions();
    FOR(i, inputoutputs.size())
    {
        if(i<pluginActions.size() && inputoutputs[i] && pluginActions[i])
        {
            if(pluginActions[i]->isChecked())
            {
                bInputRunning[i] = true;
                inputoutputs[i]->Start();
            }
            else if(bInputRunning[i])
            {
                bInputRunning[i] = false;
                inputoutputs[i]->Stop();
            }
        }
    }
}

void MLDemos::DisactivateIO(QObject *io)
{
    if(!io) return;
    // first we find the right plugin
    int pluginIndex = -1;
    FOR(i, inputoutputs.size())
    {
        if(inputoutputs[i]->object() == io)
        {
            pluginIndex = i;
            break;
        }
    }
    if(pluginIndex == -1)
    {
        statusBar()->showMessage("Unable to unload plugin: ");
        return; // something weird is going on!
    }
    QList<QAction *> pluginActions = ui.menuInput_Output->actions();
    if(pluginIndex < pluginActions.size() && pluginActions[pluginIndex])
    {
        pluginActions[pluginIndex]->setChecked(false);
        if(bInputRunning[pluginIndex]) inputoutputs[pluginIndex]->Stop();
        bInputRunning[pluginIndex] = false;
    }
}

void MLDemos::SetData(std::vector<fvec> samples, ivec labels, std::vector<ipair> trajectories)
{
    canvas->data->Clear();
    canvas->data->AddSamples(samples, labels);
    if(trajectories.size())
    {
        canvas->data->AddSequences(trajectories);
    }
    canvas->FitToData();
    canvas->ResetSamples();
    canvas->repaint();
}

void MLDemos::QueryClassifier(std::vector<fvec> samples)
{
    std::vector<fvec> results;
    QMutexLocker lock(&mutex);
    fvec result;
    result.resize(1);
    if(classifier && samples.size())
    {
        results.resize(samples.size());
        FOR(i, samples.size())
        {
            result[0] = classifier->Test(samples[i]);
            results[i] = result;
        }
    }
    emit SendResults(results);
}

void MLDemos::QueryRegressor(std::vector<fvec> samples)
{
    std::vector<fvec> results;
    QMutexLocker lock(&mutex);
    if(regressor && samples.size())
    {
        results.resize(samples.size());
        FOR(i, samples.size())
        {
            results[i] = regressor->Test(samples[i]);
        }
    }
    emit SendResults(results);
}

void MLDemos::QueryDynamical(std::vector<fvec> samples)
{
    std::vector<fvec> results;
    QMutexLocker lock(&mutex);
    if(dynamical && samples.size())
    {
        results.resize(samples.size());
        FOR(i, samples.size())
        {
            results[i] = dynamical->Test(samples[i]);
        }
    }
    emit SendResults(results);
}

void MLDemos::QueryClusterer(std::vector<fvec> samples)
{
    std::vector<fvec> results;
    QMutexLocker lock(&mutex);
    if(clusterer && samples.size())
    {
        results.resize(samples.size());
        FOR(i, samples.size())
        {
            results[i] = clusterer->Test(samples[i]);
        }
    }
    emit SendResults(results);
}

void MLDemos::QueryMaximizer(std::vector<fvec> samples)
{
    std::vector<fvec> results;
    QMutexLocker lock(&mutex);
    if(maximizer && samples.size())
    {
        results.resize(samples.size());
        FOR(i, samples.size())
        {
            results[i] = maximizer->Test(samples[i]);
        }
    }
    emit SendResults(results);
}
