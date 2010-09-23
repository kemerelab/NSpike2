/*
 * spikeGLPane.cpp: The openGL window object for nspike
 *
 * Copyright 2005 Loren M. Frank
 *
 * This program is part of the nspike data acquisition package.
 * nspike is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * nspike is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nspike; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "spikecommon.h"
#include "spike_main.h"
#include "spikeGLPane.h"

#include <string>
#include <sstream>
#include <iostream>

extern DisplayInfo dispinfo;
extern SysInfo   sysinfo;

/* Tetrode window constructor */
SpikeGLPane::SpikeGLPane(QWidget *parent, int paneNum,  int nElect) : 
  QGLWidget(QGLFormat(QGL::SingleBuffer),parent), paneNum(paneNum), 
  nElect(nElect)
{
    int i, startElect, elect;

    neegchan1 = neegchan2 = 0;
    startElect = paneNum * NELECT_PER_SCREEN;

    singleSpikePane = FALSE;
    spikePane = TRUE;
    EEGPane = FALSE;
    posPane = FALSE;

    /* set up the focus so that this widget get keyboard events. This is
     * necessary to get the keyboard and mouse to behave properly */
    this->setFocus();
    this->setFocusPolicy(Qt::StrongFocus);
    this->setEnabled(1);

    /* create a set of spikeTetInputs and Infos for the tetrode displays */
    spikeTetInput = new SpikeTetInput* [nElect];
    spikeTetInfo = new SpikeTetInfo* [nElect];


    elect = startElect;
    for (i = 0; i < nElect; i++, elect++) {
  spikeTetInput[i] = new SpikeTetInput(this, elect);
  spikeTetInfo[i] = new SpikeTetInfo(this, elect);
  this->fullScreenElect = FALSE;
  /* we also need to set the pointer for dispinfo' spikeTetInput  and
   * spikeTetInfo */
  dispinfo.spikeTetInput[elect] = spikeTetInput[i];
  dispinfo.spikeTetInfo[elect] = spikeTetInfo[i];
    }
    this->updateAllInfo();
}

/* Single tetrode windows constructor */
SpikeGLPane::SpikeGLPane(QWidget *parent, int paneNum, bool fullScreenElect) : 
    QGLWidget(QGLFormat(QGL::SingleBuffer),parent), 
       paneNum(paneNum), fullScreenElect(fullScreenElect) 
{
    neegchan1 = neegchan2 = 0;

    singleSpikePane = TRUE;
    spikePane = FALSE;
    EEGPane = FALSE;
    posPane = FALSE;

    this->nElect = 1;
    spikeTetInput = new SpikeTetInput* [nElect];
    spikeTetInfo = new SpikeTetInfo* [nElect];
    /* we will start off with a default of the first electrode */
    spikeTetInput[0] = new SpikeTetInput(this, 0, TRUE);
    spikeTetInfo[0] = new SpikeTetInfo(this, 0, TRUE);
    /* put this in at the end of the spikeTetInput array in dispinfo */
    dispinfo.spikeTetInput[sysinfo.nelectrodes] = spikeTetInput[0];
    dispinfo.spikeTetInfo[sysinfo.nelectrodes] = spikeTetInfo[0];
    this->updateAllInfo();
}

/* EEG window constructor */
SpikeGLPane::SpikeGLPane(int paneNum, QWidget *parent, const char *name, 
  int neegchan1, 
  int neegchan2, bool posPane):QGLWidget(QGLFormat(QGL::SingleBuffer), parent,
  name), paneNum(paneNum), neegchan1(neegchan1), 
  neegchan2(neegchan2), posPane(posPane) 
{
    spikePane = FALSE;
    singleSpikePane = FALSE;
    EEGPane = TRUE;

    this->nElect = 0;
    /* create the EEG buttons */
    spikeEEGInfo = new SpikeEEGInfo* [2];
    spikeEEGInfo[0] = new SpikeEEGInfo(this, 0);
    spikeEEGInfo[1] = new SpikeEEGInfo(this, 1);
    dispinfo.spikeEEGInfo = spikeEEGInfo;
    if (posPane) {
  spikePosInfo = new SpikePosInfo(this);
  dispinfo.spikePosInfo = spikePosInfo;
    }
    this->updateAllInfo();
}


SpikeGLPane::~SpikeGLPane() {
}


void SpikeGLPane::initializeGL() {

    float txinc = TET_SPIKE_WIN_WIDTH;
    float tyinc = TET_SPIKE_WIN_HEIGHT;
    float pxinc = TET_PROJ_WIN_WIDTH;
    float pyinc = TET_PROJ_WIN_HEIGHT;
    int i;
    float tickinc;

    /* position variables */
    int   ncolors = MAX_POS_THRESH;
    float   color;
    float   xpixel = dispinfo.xunit / 5;
    float   ypixel = dispinfo.yunit / 5;

    /* set up the focus so that this widget get keyboard events */
    //this->setFocus();
    //this->setFocusPolicy(QWidget::StrongFocus);
    //this->setEnabled(1);

    glClearColor(0.0, 0.0, 0.0, 0.0);

    /* set up the call list for the eeg windows */
    glNewList(EEG_CALL_LIST, GL_COMPILE);
    
    /* draw a black box over the first eeg window */
    glBegin(GL_QUADS);
    glColor3f(0, 0, 0);
    glVertex2f(EEG_WIN1_X_START, EEG_WIN1_Y_START);
    glVertex2f(EEG_WIN1_X_START + EEG_WIN_WIDTH, EEG_WIN1_Y_START);
    glVertex2f(EEG_WIN1_X_START + EEG_WIN_WIDTH, EEG_WIN1_Y_START + EEG_WIN1_HEIGHT);
    glVertex2f(EEG_WIN1_X_START, EEG_WIN1_Y_START + EEG_WIN1_HEIGHT);
    glEnd();

    /* draw a black box over the second eeg window */
    glBegin(GL_QUADS);
    glVertex2f(EEG_WIN2_X_START, EEG_WIN2_Y_START);
    glVertex2f(EEG_WIN2_X_START + EEG_WIN_WIDTH, EEG_WIN2_Y_START);
    glVertex2f(EEG_WIN2_X_START + EEG_WIN_WIDTH, EEG_WIN2_Y_START + EEG_WIN2_HEIGHT);
    glVertex2f(EEG_WIN2_X_START, EEG_WIN2_Y_START + EEG_WIN2_HEIGHT);
    glEnd();

    /* draw the four lines for the first EEG window */
    glBegin(GL_LINE_STRIP);
    glColor3f(0, 0, 1.0);
    glLineWidth(1.0f);
    glVertex2f(EEG_WIN1_X_START, EEG_WIN1_Y_START);
    glVertex2f(EEG_WIN1_X_START + EEG_WIN_WIDTH, EEG_WIN1_Y_START);
    glVertex2f(EEG_WIN1_X_START + EEG_WIN_WIDTH, EEG_WIN1_Y_START + EEG_WIN1_HEIGHT);
    glVertex2f(EEG_WIN1_X_START, EEG_WIN1_Y_START + EEG_WIN1_HEIGHT);
    glVertex2f(EEG_WIN1_X_START, EEG_WIN1_Y_START);
    glEnd();

    /* draw 10 ticks on the window */
    tickinc = EEG_WIN_WIDTH / 10;
    glBegin(GL_LINES);
    for (i = 0; i < 11; i++) {
  glVertex2f(EEG_WIN1_X_START + tickinc * i, EEG_WIN1_Y_START + 
    EEG_WIN1_HEIGHT - EEG_TICK_HEIGHT);
  glVertex2f(EEG_WIN1_X_START + tickinc * i, EEG_WIN1_Y_START + 
    EEG_WIN1_HEIGHT + EEG_TICK_HEIGHT);
  glVertex2f(EEG_WIN1_X_START + tickinc * i, EEG_WIN1_Y_START + 
    EEG_TICK_HEIGHT);
  glVertex2f(EEG_WIN1_X_START + tickinc * i, EEG_WIN1_Y_START - 
    EEG_TICK_HEIGHT);
    }
    glEnd();

    /* draw the four lines for the second EEG window */
    glBegin(GL_LINE_STRIP);
    glVertex2f(EEG_WIN2_X_START, EEG_WIN2_Y_START);
    glVertex2f(EEG_WIN2_X_START + EEG_WIN_WIDTH, EEG_WIN2_Y_START);
    glVertex2f(EEG_WIN2_X_START + EEG_WIN_WIDTH, EEG_WIN2_Y_START + 
      EEG_WIN2_HEIGHT);
    glVertex2f(EEG_WIN2_X_START, EEG_WIN2_Y_START + EEG_WIN2_HEIGHT);
    glVertex2f(EEG_WIN2_X_START, EEG_WIN2_Y_START);
    glEnd();

    /* draw 10 ticks on the window */
    glBegin(GL_LINES);
    for (i = 0; i < 11; i++) {
  glVertex2f(EEG_WIN2_X_START + tickinc * i, EEG_WIN2_Y_START + 
    EEG_WIN2_HEIGHT - EEG_TICK_HEIGHT);
  glVertex2f(EEG_WIN2_X_START + tickinc * i, EEG_WIN2_Y_START + 
    EEG_WIN2_HEIGHT + EEG_TICK_HEIGHT);
  glVertex2f(EEG_WIN2_X_START + tickinc * i, EEG_WIN2_Y_START +
    EEG_TICK_HEIGHT);
  glVertex2f(EEG_WIN2_X_START + tickinc * i, EEG_WIN2_Y_START - 
    EEG_TICK_HEIGHT);
    }
    glEnd();
    glEndList();

    /* set up the call lists for the tetrode windows */

    glNewList(TET_SPIKE_CALL_LIST, GL_COMPILE);

    /* first draw a black box over the four spike window boxes */
    glColor3f(0, 0, 0);
    glBegin(GL_QUADS);
    glVertex2f(TET_SPIKE_WIN_X_START, TET_SPIKE_WIN_Y_START);
    glVertex2f(TET_SPIKE_WIN_X_START + NCHAN_PER_ELECTRODE*txinc, TET_SPIKE_WIN_Y_START);
    glVertex2f(TET_SPIKE_WIN_X_START + NCHAN_PER_ELECTRODE*txinc, TET_SPIKE_WIN_Y_START + tyinc);
    glVertex2f(TET_SPIKE_WIN_X_START, TET_SPIKE_WIN_Y_START + tyinc);
    glEnd();

    /* first draw the spike window boxes in blue */
    glColor3f(0, 0, 1.0);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    /* bottom line */
    glVertex2f(TET_SPIKE_WIN_X_START, TET_SPIKE_WIN_Y_START);
    glVertex2f(TET_SPIKE_WIN_X_START + NCHAN_PER_ELECTRODE*txinc, TET_SPIKE_WIN_Y_START);
    /* top line */
    glVertex2f(TET_SPIKE_WIN_X_START, TET_SPIKE_WIN_Y_START + tyinc);
    glVertex2f(TET_SPIKE_WIN_X_START + NCHAN_PER_ELECTRODE*txinc, TET_SPIKE_WIN_Y_START + tyinc);
    /* left edge of each box */
    for (i = 0; i < 5; i++) {
  glVertex2f(TET_SPIKE_WIN_X_START + i * txinc , TET_SPIKE_WIN_Y_START);
  glVertex2f(TET_SPIKE_WIN_X_START + i * txinc, TET_SPIKE_WIN_Y_START + tyinc);
    }
    /* now draw the zero ticks in a dark red */
    for (i = 0; i < NCHAN_PER_ELECTRODE; i++) {
        glVertex2f(dispinfo.spikewinorig[i].x, dispinfo.spikewinorig[i].y);
        glVertex2f(dispinfo.spikewinorig[i].x+1*dispinfo.xunit, dispinfo.spikewinorig[i].y);
    }
    glEnd();
     
    glEndList();

    glNewList(TET_PROJ_CALL_LIST, GL_COMPILE);
    /* erase the old projection windows */
    glColor3f(0, 0, 0);
    glBegin(GL_QUADS);
    glVertex2f(TET_PROJ_WIN_X_START, TET_PROJ_WIN_Y_START);
    glVertex2f(TET_PROJ_WIN_X_START+2*pxinc, TET_PROJ_WIN_Y_START);
    glVertex2f(TET_PROJ_WIN_X_START+2*pxinc, TET_PROJ_WIN_Y_START+3*pyinc);
    glVertex2f(TET_PROJ_WIN_X_START, TET_PROJ_WIN_Y_START+3*pyinc);
    glEnd();

    /* now draw the projection windows in gray */
    glColor3f(.9, .9, .9);
    glBegin(GL_LINES);
    /* bottom line */
    for (i = 0; i < 4; i++) {
  glVertex2f(TET_PROJ_WIN_X_START, TET_PROJ_WIN_Y_START + i*pyinc);
  glVertex2f(TET_PROJ_WIN_X_START + 2*pxinc, TET_PROJ_WIN_Y_START + i*pyinc);
    }
    /* left edge of each box */
    for (i = 0; i < 3; i++) {
  glVertex2f(TET_PROJ_WIN_X_START + i*pxinc, TET_PROJ_WIN_Y_START);
  glVertex2f(TET_PROJ_WIN_X_START + i*pxinc, TET_PROJ_WIN_Y_START + 3*pyinc);
    }
    glEnd();
    glEndList();

    glNewList(POSITION_CALL_LIST, GL_COMPILE);
    /* draw a line for each color of the color bar */
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (i = 0; i <  ncolors; i++) {
        color = ((float) i) / ncolors; 
  glColor3f(color, color, color);
  glVertex2f(dispinfo.poscolorbarloc[0].x, dispinfo.poscolorbarloc[0].y + 
                                           dispinfo.colorbaryscale * (float) i);
  glVertex2f(dispinfo.poscolorbarloc[1].x, dispinfo.poscolorbarloc[0].y + 
                                           dispinfo.colorbaryscale * (float) i);
    }
    /* draw a blue box around the position display area */
    glColor3f(0, 0, 1.0);
    glLineWidth(1.0f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(dispinfo.posloc.x - xpixel, dispinfo.posloc.y - ypixel);
    glVertex2f(dispinfo.posloc.x + POS_WIN_WIDTH + xpixel, dispinfo.posloc.y - ypixel);
    glVertex2f(dispinfo.posloc.x + POS_WIN_WIDTH + xpixel, dispinfo.posloc.y + POS_WIN_HEIGHT + ypixel);
    glVertex2f(dispinfo.posloc.x - xpixel, dispinfo.posloc.y + POS_WIN_HEIGHT + ypixel);
    glVertex2f(dispinfo.posloc.x - xpixel, dispinfo.posloc.y - ypixel);
    glEnd();

    /* draw a blue box around the color bar area */
    glColor3f(0, 0, 1.0);
    glLineWidth(1.0f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(dispinfo.poscolorbarloc[0].x - xpixel, dispinfo.poscolorbarloc[0].y - ypixel);
    glVertex2f(dispinfo.poscolorbarloc[0].x - xpixel, dispinfo.poscolorbarloc[1].y + ypixel);
    glVertex2f(dispinfo.poscolorbarloc[1].x + xpixel, dispinfo.poscolorbarloc[1].y + ypixel);
    glVertex2f(dispinfo.poscolorbarloc[1].x + xpixel, dispinfo.poscolorbarloc[0].y - ypixel);
    glVertex2f(dispinfo.poscolorbarloc[0].x - xpixel, dispinfo.poscolorbarloc[0].y - ypixel);
    glEnd();
    glEndList();

}

void SpikeGLPane::resizeGL(int w, int h) 
{
  int i, x, y, xsize, ysize;
  double width, tmpheight;
  int elect;
  Point tmppoint;
  QRect geom = this->geometry();

  glViewport(0, 0, (GLint)w, (GLint) h);
  glLoadIdentity();       // Reset The Projection Matrix
  glOrtho(X_MIN, X_MAX, Y_MIN, Y_MAX, -1000, 1000);
  glMatrixMode(GL_MODELVIEW);
  dispinfo.screen_width = w;
  dispinfo.screen_height = h;
  glMatrixMode(GL_MODELVIEW);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

  if (posPane) {
    this->spikePosInfo->updatePosInfo();
    SetPosLoc();
  }

  if ((w == prevW) || (h == prevH)) {
    /* if the size hasn't changed we don't need to resize the buttons */
    return;
  }

  /* check to see if this is a tetrode window */
  if (this->spikePane || this->singleSpikePane) {
    /* now resize the input windows if the window size has changed */
    /* get the coordinates for the size of the window */
    tmppoint.x = X_MIN + NCHAN_PER_ELECTRODE * TET_SPIKE_WIN_WIDTH;
    tmppoint.y = Y_MIN + TET_SPIKE_BUTTON_HEIGHT;
    GLToWindow(tmppoint, geom, &xsize, &ysize, 1.0, 1.0); 
    /* the GL and QT screens have opposite y directions, so we have to flip 
     * the result to get a size rather than a coordinate */
    ysize = geom.height() - ysize;
    if (this->fullScreenElect == FALSE) {
      elect = this->paneNum * NELECT_PER_SCREEN;
      for (i = 0; i < this->nElect; i++, elect++) {
        tmppoint.x = dispinfo.electloc[elect].x + TET_SPIKE_WIN_X_START;
        tmppoint.y = dispinfo.electloc[elect].y + TET_PROJ_WIN_Y_START;
        GLToWindow(tmppoint, geom, &x, &y, 1.0, 1.0); 
        if ((this->spikeTetInput != NULL) && 
            (this->spikeTetInput[i] != NULL)) {
          this->spikeTetInput[i]->setGeometry(x, y - ysize, xsize, ysize);
        }
      } 
    }
    else {
      /* this is the full screen electrode */
      tmppoint.x = X_MIN + NCHAN_PER_ELECTRODE * TET_SPIKE_WIN_WIDTH;
      tmppoint.y = Y_MIN + TET_SPIKE_BUTTON_HEIGHT;
      GLToWindow(tmppoint, geom, &xsize, &ysize, 2.0, 2.0); 
      ysize = geom.height() - ysize;
      tmppoint.x = TET_FULL_SCREEN_XSTART + TET_SPIKE_WIN_X_START * 2.0;
      tmppoint.y = TET_FULL_SCREEN_YSTART + TET_PROJ_WIN_Y_START * 2.0;
      GLToWindow(tmppoint, geom, &x, &y, 1.0, 1.0); 
      if (this->spikeTetInput[0] != NULL) {
        this->spikeTetInput[0]->setGeometry(x, y - ysize, xsize, ysize);
      }
    }
    /* now resize the info windows if the window size has changed */
    /* get the coordinates for the size of the window */
    tmppoint.x = X_MIN + NCHAN_PER_ELECTRODE * TET_SPIKE_WIN_WIDTH;
    tmppoint.y = Y_MIN + TET_SPIKE_BUTTON_HEIGHT / 1.5;
    GLToWindow(tmppoint, geom, &xsize, &ysize, 1.0, 1.0); 
    /* the GL and QT screens have opposite y directions, so we have to flip 
     * the result to get a size rather than a coordinate */
    ysize = geom.height() - ysize;
    if (this->fullScreenElect == FALSE) {
      elect = this->paneNum * NELECT_PER_SCREEN;
      for (i = 0; i < this->nElect; i++, elect++) {
        tmppoint.x = dispinfo.electloc[elect].x + TET_SPIKE_WIN_X_START;
        tmppoint.y = dispinfo.electloc[elect].y + 
          TET_SPIKE_WIN_Y_START + TET_SPIKE_WIN_HEIGHT + 
          dispinfo.yunit / 2;
        GLToWindow(tmppoint, geom, &x, &y, 1.0, 1.0); 
        if ((this->spikeTetInfo != NULL) && 
            (this->spikeTetInfo[i] != NULL)) {
          this->spikeTetInfo[i]->setGeometry(x, y - ysize, xsize, ysize);
        }
      } 
    }
    else {
      /* this is the full screen electrode */
      tmppoint.x = X_MIN + NCHAN_PER_ELECTRODE * TET_SPIKE_WIN_WIDTH;
      tmppoint.y = Y_MIN + TET_SPIKE_BUTTON_HEIGHT / 1.5;
      GLToWindow(tmppoint, geom, &xsize, &ysize, 2.0, 2.0); 
      ysize = geom.height() - ysize;
      tmppoint.x = TET_FULL_SCREEN_XSTART + TET_SPIKE_WIN_X_START * 2.0;
      tmppoint.y = TET_FULL_SCREEN_YSTART + 
        (TET_SPIKE_WIN_Y_START + TET_SPIKE_WIN_HEIGHT) * 2.0;
      GLToWindow(tmppoint, geom, &x, &y, 1.0, 1.0); 
      if (this->spikeTetInfo[0] != NULL) {
        this->spikeTetInfo[0]->setGeometry(x, y - ysize, xsize, ysize);
      }
    }
  }
  else if (this->EEGPane) {
    /* resize the first row of eeg buttons */
    width = EEG_WIN1_X_START - dispinfo.xunit;
    tmppoint.x = width;
    tmpheight = EEG_WIN1_HEIGHT / (dispinfo.neegchan1 + 1);
    tmppoint.y = Y_MIN + EEG_WIN1_HEIGHT - tmpheight;
    GLToWindow(tmppoint, geom, &xsize, &ysize, 1.0, 1.0); 
    ysize = geom.height() - ysize;
    tmppoint.x = X_MIN + EEG_WIN1_X_START - width;
    tmppoint.y = EEG_WIN1_Y_START + tmpheight / 2;
    GLToWindow(tmppoint, geom, &x, &y, 1.0, 1.0); 
    if (this->spikeEEGInfo[0] != NULL) {
      this->spikeEEGInfo[0]->setGeometry(x, y - ysize, xsize, ysize);
    }
    /* resize the second row of eeg buttons */
    tmppoint.x = width;
    tmpheight = EEG_WIN2_HEIGHT / (dispinfo.neegchan2 + 1);
    tmppoint.y = Y_MIN + EEG_WIN2_HEIGHT - tmpheight;
    GLToWindow(tmppoint, geom, &xsize, &ysize, 1.0, 1.0); 
    ysize = geom.height() - ysize;
    tmppoint.x = X_MIN + EEG_WIN2_X_START - width;
    tmppoint.y = EEG_WIN2_Y_START + tmpheight / 2;
    GLToWindow(tmppoint, geom, &x, &y, 1.0, 1.0); 
    if (this->spikeEEGInfo[1] != NULL) {
      this->spikeEEGInfo[1]->setGeometry(x, y - ysize, xsize, ysize);
    }
  }
  if (this->posPane) {
    /* resize the position inputs  */
    tmppoint.x = X_MIN + POS_WIN_WIDTH;
    tmppoint.y = Y_MIN + COMMON_BUTTON_HEIGHT;
    GLToWindow(tmppoint, geom, &xsize, &ysize, 1.0, 1.0); 
    ysize = geom.height() - ysize;
    tmppoint.x = POS_WIN_X_START;
    tmppoint.y = POS_WIN_Y_START + POS_WIN_HEIGHT;
    //tmppoint.y = POS_WIN_Y_START + POS_WIN_HEIGHT + dispinfo.yunit / 5;
    GLToWindow(tmppoint, geom, &x, &y, 1.0, 1.0); 
    //this->spikePosInfo->setGeometry(x, y - ysize, xsize, ysize); 
    this->spikePosInfo->setGeometry(x, y - spikePosInfo->height(), xsize, ysize); 
  } 
  /* now update the info buttons */
  tmppoint.x = X_MAX; 
  tmppoint.y = Y_MIN + 2 * COMMON_BUTTON_HEIGHT;
  GLToWindow(tmppoint, geom, &xsize, &ysize, 1.0, 1.0); 
  ysize = geom.height() - ysize;
  tmppoint.x = X_MIN;
  tmppoint.y = Y_MIN;
  GLToWindow(tmppoint, geom, &x, &y, 1.0, 1.0); 

  prevW = w;
  prevH = h;
}


void SpikeGLPane::paintGL() 
{
    DrawInitialScreen();
}


void SpikeGLPane::updateAllInfo(void) 
{
    int i;
    /* if this is the full screen window (singleSpikePane) then we need to
     * reset the electrode number to dispinfo.fullscreenelect */
    if ((singleSpikePane) && (dispinfo.fullscreenelect != -1)) {
  this->spikeTetInput[0]->electNum = dispinfo.fullscreenelect;
  this->spikeTetInfo[0]->electNum = dispinfo.fullscreenelect;
    }
    /* go through all of the widgets on the screen and update them */
    for (i = 0; i < this->nElect; i++) {
  this->spikeTetInput[i]->updateTetInput();
  this->spikeTetInfo[i]->updateTetInfo();
    }
    if (neegchan1) {
  this->spikeEEGInfo[0]->updateAll();
    }
    if (neegchan2) {
  this->spikeEEGInfo[1]->updateAll();
    }
}

void SpikeGLPane::updateInfo(void) 
{
}

