#include "echo.h"
#include "sensor.h"
#include <QFile>
#include <QTextStream>

#define SENSOR_OFFSET   0.01175
#define SENSOR_DIST     0.0235

//#define SENSOR_OFFSET   ((0.0235+0.0003188793679592748)/2)
//#define SENSOR_DIST     (0.0235+0.0003188793679592748)

//#define SENSOR_OFFSET   ((0.0235+0.002003642308493)/2)
//#define SENSOR_DIST     (0.0235+0.002003642308493)

#define ALG1_DIST       0.01
#define SAMPLE_PERIOD   0.00001
//#define SAMPLE_PERIOD   0.000025
#define PI              3.14159265

Echo::Echo()
{
    objNum                      = 0;
    transducerActive            = false;
    signalGenerationFinished    = false;
    echoDetection               = false;
    range_Y_max = range_X_max = range_X_min = 0;
    for (short i = 0; i < 20; i++)
        echoStartTab[i] = 0;
    for (short i = 0; i < 20; i++)
        echoEndTab[i] = 0;
    for (short i = 0; i < 5; i++) {
        filterDistTab_Left[i]  = 0;
        filterDistTab_Right[i] = 0;
    }
}

void Echo::processSignal(Sensor mySensor, quint16 adc, quint16 x, int threshold)
{
    /* Wait for the acoustic wave generation period */
    if (signalGenerationFinished == false && adc < threshold && transducerActive == false) {
        adc_prv = adc;
    }
    else if (signalGenerationFinished == false && adc >= threshold && transducerActive == false) {
        transducerActive = true;
        waveGenerationTime = x;
        /* Calculations for interpolation */
        quint16 dV  = adc - adc_prv;
        quint16 dTh = threshold - adc_prv;
        if (adc != threshold) {
            genOffsetInterpol = ((double)dTh)/((double)dV);
        }
        else {
            genOffsetInterpol = 0;
        }
    }
    else if (signalGenerationFinished == false && adc <= threshold && transducerActive == true)
        signalGenerationFinished = true;

    if (signalGenerationFinished == true && x > 350) {
        if (echoDetection == false && adc >= threshold) {
            echoDetection = true;
            if (mySensor.interpolation == true) {
                quint16 dV  = adc - adc_prv;
                quint16 dTh = threshold - adc_prv;
                if (adc != threshold) {
                    detOffsetInterpol[objNum] = ((double)dTh)/((double)dV);
                }
                else {
                    detOffsetInterpol[objNum] = 0;
                }
                echoStartTab[objNum] = x;
            }
            else
                echoStartTab[objNum] = x;
        }
        else if (echoDetection == true && adc <= threshold) {
            echoDetection = false;
            echoEndTab[objNum] = x;
            objNum++;
        }
        else if (echoDetection == false && adc < threshold)
            adc_prv = adc;
    }
}

double* Echo::calculateDetectionPoints(Sensor mySensor, double adjL, double adjR, QFile *angleFile)
{
    double dystans[objNum];
    resultsXY = new double[(objNum*2)];
    double dystans_xy[2];
    short dt;
    short index = 0;

    /* Clear table for the algorithm 1 and triangulation */
    if (mySensor.wybrany_czujnik == Sensor::lewy)
        for (short i = 0; i < 10; i++) {
            detDistanceAlg1_left[i] = 0;
            detDistanceAlg1_leftSamplesNo = 0;
            detDistanceAlg1_leftEchoStrength[i] = 0;
            detDistanceAlg1_right[i] = 0;
            detDistanceAlg1_rightSamplesNo = 0;
            detDistanceAlg1_rightEchoStrength[i] = 0;

            triangulationY[i] = 0;
            triangulationX[i] = 0;
            detDistTrian[i] = 0;

            detDistanceAlg1[i] = 0;
            detDistanceAlg1_Strength[i] = 0;
        }

    for (short echoIndex = 0; echoIndex < objNum; echoIndex++) {
        echoStrengthValues[echoIndex] = calculateEchoStrength(echoEndTab[echoIndex] - (short)echoStartTab[echoIndex], mySensor);

        if (mySensor.wybrany_czujnik == Sensor::lewy)
            detDistanceAlg1_leftEchoStrength[echoIndex] = echoStrengthValues[echoIndex];
        else
            detDistanceAlg1_rightEchoStrength[echoIndex] = echoStrengthValues[echoIndex];

        dt = echoStartTab[echoIndex] - waveGenerationTime;

        static QString saveFileName =
                    "/home/marekf/Dokumenty/pwr/mgr/build/build-sonar_app-Desktop_Qt_5_6_0_GCC_64bit-Debug/data/data_err.txt";
        static QFile fileErr(saveFileName);
        static QTextStream stream(&fileErr);
        fileErr.open(QIODevice::ReadWrite | QIODevice::Text);
        //stream << echoL << '\t' << echoR << endl;

        if (mySensor.wybrany_czujnik == Sensor::lewy) {
            if (mySensor.interpolation == true) {
                dystans[echoIndex] = ((adjL*(dt*SAMPLE_PERIOD))/2) + ((adjL*((1-genOffsetInterpol)*SAMPLE_PERIOD))/2)
                        - ((adjL*((1-detOffsetInterpol[echoIndex])*SAMPLE_PERIOD))/2);
                stream << dystans[echoIndex] << '\t';
            }
            else
                dystans[echoIndex] = (adjL*(dt*SAMPLE_PERIOD))/2;
        }
        else
            if (mySensor.interpolation == true) {
                dystans[echoIndex] = ((adjR*(dt*SAMPLE_PERIOD))/2) + ((adjR*((1-genOffsetInterpol)*SAMPLE_PERIOD))/2)
                        - ((adjR*((1-detOffsetInterpol[echoIndex])*SAMPLE_PERIOD))/2);
                stream << dystans[echoIndex] << endl;
            }
            else
                dystans[echoIndex] = (adjR*(dt*SAMPLE_PERIOD))/2;
    }

    /* Algorithm 1 - accept neighbours */
    /* This is also needed for the traingulation algorithm */
    if (mySensor.algorithm == Sensor::alg1 || mySensor.algorithm == Sensor::triangulation || mySensor.algorithm == Sensor::alg1_and_trian) {
        /* Acquire data for the left sensor - max. 10 samples */
        if (mySensor.wybrany_czujnik == Sensor::lewy) {
            for (int i = 0; i < objNum && i < 10; i++) {
                detDistanceAlg1_left[i] = dystans[i];
                detDistanceAlg1_leftSamplesNo = i + 1;
            }
        }
        /* Compare data from the right sensor with data from the left one and choose only close samples */
        else if (mySensor.wybrany_czujnik == Sensor::prawy) {
            for (int i = 0; i < objNum && i < 10; i++) {
                detDistanceAlg1_right[i] = dystans[i];
                detDistanceAlg1_rightSamplesNo = i + 1;
            }
            /* Comparision */
            short index_alg1 = 0;
            short index_trian = 0;
            for (int j = 0; j < detDistanceAlg1_rightSamplesNo; j++)
                for (int k = 0; k < detDistanceAlg1_leftSamplesNo; k++) {
                    if (abs(detDistanceAlg1_right[j] - detDistanceAlg1_left[k]) < mySensor.alg1_radius) {

                        if ((mySensor.algorithm == Sensor::triangulation || mySensor.algorithm == Sensor::alg1_and_trian)
                                && (detDistanceAlg1_leftEchoStrength[index_alg1]  >= 3)
                                && (detDistanceAlg1_rightEchoStrength[index_alg1] >= 3) ) {
                            echoTriangulation(mySensor, detDistanceAlg1_left[k], detDistanceAlg1_right[j], index_trian, angleFile);
                            index_trian++;
                        }

                        detDistanceAlg1[index_alg1] = ((detDistanceAlg1_right[j] + detDistanceAlg1_left[k]) / 2);
                        detDistanceAlg1_Strength[index_alg1] = static_cast<short>((detDistanceAlg1_rightEchoStrength[j] +
                                                                                   detDistanceAlg1_leftEchoStrength[k]) / 2);
                        index_alg1++;
                    }
                }
        }
    }


    for (short echoIndex = 0; echoIndex < (objNum * 2); echoIndex += 2) {
        if (mySensor.wybrany_czujnik == Sensor::lewy) {
            dystans_xy[1] = cos(mySensor.angle*PI/180) * dystans[index] + SENSOR_OFFSET * sin(mySensor.angle*PI/180);
            dystans_xy[0] = sin(mySensor.angle*PI/180) * dystans[index] - SENSOR_OFFSET * cos(mySensor.angle*PI/180);
        }
        else {
            dystans_xy[1] = cos(mySensor.angle*PI/180) * dystans[index] - SENSOR_OFFSET * sin(mySensor.angle*PI/180);
            dystans_xy[0] = sin(mySensor.angle*PI/180) * dystans[index] + SENSOR_OFFSET * cos(mySensor.angle*PI/180);
        }
        resultsXY[echoIndex] = dystans_xy[0];
        resultsXY[echoIndex + 1] = dystans_xy[1];

        index++;

        if (dystans_xy[1] > range_Y_max)
            range_Y_max = dystans_xy[1];
        if (dystans_xy[0] > range_X_max)
            range_X_max = dystans_xy[0];
        if (dystans_xy[0] < range_X_min)
            range_X_min = dystans_xy[0];
    }
    return resultsXY;
}

void Echo::echoTriangulation(Sensor mySensor, double echoL, double echoR, short index, QFile *angleFile2)
{
    /* Create file for dLi and dRi - measurement error calculation */
    /*static QString saveFileName =
            "/home/marekf/Dokumenty/pwr/mgr/build/build-sonar_app-Desktop_Qt_5_6_0_GCC_64bit-Debug/data/data_err.txt";
    static QFile fileErr(saveFileName);
    static QTextStream stream(&fileErr);
    fileErr.open(QIODevice::ReadWrite | QIODevice::Text);
    stream << echoL << '\t' << echoR << endl;*/
    /* --- */

    /*double angle_tmp = acos(((echoL*echoL)+(SENSOR_DIST*SENSOR_DIST)-(echoR*echoR))/(2*echoL*SENSOR_DIST));

    if (angle_tmp > (PI/2))
        xTrian = (1/(2*SENSOR_DIST))*((echoL*echoL)-(echoR*echoR)-(SENSOR_DIST*SENSOR_DIST));
    else
        xTrian = (1/(2*SENSOR_DIST))*((echoL*echoL)-(echoR*echoR)+(SENSOR_DIST*SENSOR_DIST));*/

    double xTrian = ((echoL*echoL)-(echoR*echoR)+(SENSOR_DIST*SENSOR_DIST))/(2*SENSOR_DIST);
    double yTrian = sqrt((echoL*echoL)-(xTrian*xTrian));

    double theta_local = -atan(xTrian/yTrian);
    double theta_local_saveDeg = 180*theta_local/PI;

    double theta = (mySensor.angle*PI/180);
    QTextStream stream2angleFile(angleFile2);
    (*angleFile2).open(QIODevice::ReadWrite | QIODevice::Text);

    stream2angleFile << mySensor.angle << '\t' <<
                        mySensor.angle << '\t' <<
                        theta_local_saveDeg << '\t' <<
                        echoL << '\t' << echoR << '\t' << endl;


    double mRot[3][3] = { { cos(theta), sin(theta), SENSOR_OFFSET*(1-cos(theta))},
                          {-sin(theta), cos(theta), SENSOR_OFFSET*sin(theta)},
                          { 0,          0,          1} };
    double mTran[3] = {xTrian, yTrian, 1};
    double mPos[3]  = {0, 0, 0};

    for (short m = 0; m < 3; m++)
        for (short n = 0; n < 3; n++)
            mPos[m] += mRot[m][n] * mTran[n];

    triangulationX[index] = mPos[0];
    triangulationY[index] = mPos[1];
}

double Echo::getTrianX(short i)
{
    return triangulationX[i];
}

double Echo::getTrianY(short i)
{
    return triangulationY[i];
}

double Echo::getYmax()
{
    return range_Y_max;
}

double Echo::getXmax()
{
    return range_X_max;
}

double Echo::getXmin()
{
    return range_X_min;
}

short Echo::getNumberOfObjects()
{
    return objNum;
}

short Echo::getEchoStrengthValue(short index)
{
    return echoStrengthValues[index];
}

short Echo::calculateEchoStrength(quint16 echoStr, Sensor sensor)
{
    switch (sensor.wybrany_czujnik) {
        case 0:
            if (echoStr == 0)
              return 0;
            else if (echoStr < 35)
            //else if (echoStr < 13)
                return 1;
            else if (echoStr < 70)
            //else if (echoStr < 28)
                return 2;
            else
                return 3;
            break;
        case 1:
            if (echoStr == 0)
             return 0;
            else if (echoStr < 30)
            //else if (echoStr < 12)
             return 1;
            else if (echoStr < 60)
            //else if (echoStr < 24)
             return 2;
            else
             return 3;
            break;
        default:
            break;
    }
    //return (static_cast<int>(echoStr / 25) + 1);
}

void Echo::deleteResults()
{
    delete [] resultsXY;
}

void Echo::restoreToDefaultEcho()
{
    objNum                      = 0;
    transducerActive            = false;
    signalGenerationFinished    = false;
    echoDetection               = false;
    range_Y_max = range_X_max = range_X_min = 0;
    for (short i = 0; i < 20; i++)
        echoStartTab[i] = 0;
    for (short i = 0; i < 20; i++)
        echoEndTab[i] = 0;
}
