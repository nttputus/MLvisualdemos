# ##########################
# Configuration      #
# ##########################
TEMPLATE = app
QT -= network
QT += svg
TARGET = mldemos
NAME = mldemos
MLPATH =..
DESTDIR = $$MLPATH

macx:ICON = logo.icns
win32:RC_FILE = MachineLearning.rc
RESOURCES += mldemos.qrc

CONFIG += mainApp
include($$MLPATH/MLDemos_variables.pri)

macx:INCLUDEPATH += uiMac

# ##########################
# Source Files       #
# ##########################
FORMS += aboutDialog.ui \
	algorithmOptions.ui \
	optsClassify.ui \
	optsCluster.ui \
	optsRegress.ui \
	optsMaximize.ui \
	optsDynamic.ui \
	optsCompare.ui \
	mldemos.ui \
	drawingTools.ui \
	drawingToolsContext1.ui \
	drawingToolsContext2.ui \
	drawingToolsContext3.ui \
	drawingToolsContext4.ui \
	statisticsDialog.ui \
	viewOptions.ui \
    compare.ui

HEADERS += basicMath.h \
    canvas.h \
    datasetManager.h \
	optimization_test_functions.h \
	gettimeofday.h \
	drawUtils.h \
	drawSVG.h \
	drawTimer.h \
    mldemos.h \
    mymaths.h \
    public.h \
	roc.h \
    types.h \
    widget.h \
    interfaces.h \
	classifier.h \
	obstacles.h \
	regressor.h \
	maximize.h \
	dynamical.h \
    clusterer.h \
    compare.h

SOURCES += canvas.cpp \
    datasetManager.cpp \
    drawUtils.cpp \
	drawSVG.cpp \
	drawTimer.cpp \
    main.cpp \
	mldemos.cpp \
    mlprocessing.cpp \
    mlstats.cpp \
    mlsaving.cpp \
    mymaths.cpp \
	roc.cpp \
    widget.cpp \
    compare.cpp
