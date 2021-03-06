#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qcustomplot.h"
#include "settingsserialdialog.h"
#include "echo.h"
#include "sensor.h"

#include <QMainWindow>
#include <QSerialPort>
#include <QPoint>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void drawMap(QCustomPlot *customPlot);
    void drawSignalPlot(QCustomPlot *customPlot);
    ~MainWindow();

private slots:
    void sendAngleToSTM();
    void readSerialData();
    void drawDataOnMap(Echo *echoCpy);
    void threshold_changed();

    void on_actionSerial_port_triggered();
    void on_actionConnect_triggered();

    void contextMenuRequest(QPoint pos);

    void on_singleMeasurementBtn_clicked();

    void contexMenu_setBothSonarsInvisible();
    void contexMenu_setBothSonarsVisible();
    void contexMenu_setLeftSonarInvisible();
    void contexMenu_setLeftSonarVisible();
    void contexMenu_setRightSonarInvisible();
    void contexMenu_setRightSonarVisible();

    void on_scanBtn_clicked();

    void on_updateAngleBtn_clicked();

    void on_connectBtn_clicked();

    void on_cBox_left_sensor_stateChanged(int arg1);

    void on_cBox_right_sensor_stateChanged(int arg1);

    void on_spinBox_alg1_valueChanged(double arg1);

    void on_lSon_visible_stateChanged(int arg1);

    void on_rSon_visible_stateChanged(int arg1);

    void on_alg1_visible_stateChanged(int arg1);

    void showPointCoordinates(QMouseEvent *event);

    void zoomWithScroll(QWheelEvent *event);

    void on_rBtn_Trian_clicked();

    void on_cBox_alg1_clicked();

    void on_cBox_Tr_visible_clicked();

    void on_btn_save_clicked();

    void on_loadBtn_clicked();

    void on_interpolation_cBox_toggled(bool checked);

private:
    Ui::MainWindow *ui;
    void dataHandler(Echo *echo_ptr, quint16 *x);
    SettingsSerialDialog *settingsSerDialog;

    enum measurementType {
        none = 0,
        single = 1,
        scan = 2
    };

    measurementType currentMeasurementType;

    int             threshold_value;
    Sensor          mbSonar;
    QSerialPort     *stm32_serial;
    QString         stm32_port_name;
    bool            stm32_available;
    bool            loadOperation;
    short           t1, dt;
    double          dystans_y_max;
    double          dystans_x_min;
    double          dystans_x_max;
    QString         serialBuffer;
    QString         angleFileName;
    QFile           angleFileMain;
    QStringList     bufferSplit;
};

#endif // MAINWINDOW_H
