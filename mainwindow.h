#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "canvasopengl.h"
#include "polygondrawer.h"
#include "mousefollower.h"
#include "hintboxdrawer.h"

#include <QMainWindow>
#include <QtGui>
#include <QWidget>
#include <QOpenGLWidget>


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

// ATRIBUTES
private:
    Ui::MainWindow *ui;
    CanvasOpenGL* openGlCanvas;
    QColor color;


// METHODS
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    CanvasOpenGL* Canvas() const;

    void onReset();

private:
    void keyReleaseEvent(QKeyEvent*);



// SIGNALS / SLOTS
signals:
    void keyReleased(int);
    void clearPressed();
    void editPressed();
    void shadingChanged(const QString&);

    void lightingValueChanged(int, int, int);
    void cameraRotationChanged(int, int, int);
    void cameraClippingChanged(int, int);
    void cameraLimitsChanged(int, int, int, int);
    void cameraFovChanged(double);
    void cameraPerspectiveChanged(bool);

private slots:
    void on_ClearButton_clicked();
    void on_changeColor_released();

    void on_obsXValue_valueChanged(double arg1);
    void on_obsYValue_valueChanged(double arg1);
    void on_obsZValue_valueChanged(double arg1);
    void on_lightValueX_valueChanged(double arg1);
    void on_lightValueY_valueChanged(double arg1);
    void on_lightValueZ_valueChanged(double arg1);
    void on_toningValue_currentTextChanged(const QString &arg1);
    void on_ResetButton_released();
    void on_editButton_released();
};

#endif // MAINWINDOW_H
