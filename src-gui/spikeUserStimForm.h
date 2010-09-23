/****************************************************************************
** Form interface generated from reading ui file 'stimform.ui'
**
** Created: Tue Apr 7 15:28:31 2009
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef STIMFORM_H
#define STIMFORM_H

#include <QtGui>

class StimForm : public QWidget
{
    Q_OBJECT

public:
    StimForm( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~StimForm();

    QSpinBox* spinBox1_2_2;
    QLabel* textLabel1_2_2;
    QSpinBox* spinBox1;
    QLabel* textLabel1_2_3;
    QSpinBox* spinBox1_2_3;
    QLabel* textLabel1;
    QCheckBox* checkBox1;
    QLabel* textLabel1_2;
    QSpinBox* spinBox1_2;
    QLabel* textLabel1_2_3_2;
    QSpinBox* spinBox1_2_3_2;
    QLabel* textLabel1_2_4;
    QLabel* textLabel1_2_3_2_2;
    QSpinBox* spinBox1_2_3_2_2;
    QLabel* textLabel1_2_3_2_2_2;
    QSpinBox* spinBox1_2_3_2_2_2;
    QSpinBox* spinBox1_2_4;
    QPushButton* pushButton1;
    QPushButton* pushButton2;

protected:

protected slots:
    virtual void languageChange();

};

#endif // STIMFORM_H
