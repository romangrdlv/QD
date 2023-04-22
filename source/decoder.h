#ifndef DECODER_H
#define DECODER_H

#include <QWidget>

class Decoder
{
public:
    Decoder();
    QPixmap mainSequence(const QPixmap* image, QPoint* blocks, int size);
private:
    void defineLengths(QPoint* blocks);
    int defineQRQuadrant(QPoint* blocks);
    float defineRotationAngle(QPoint* blocks, int quadrant);
    QPoint rotatePoint(QPoint point, const QPixmap* image, QPixmap newImage, float angle);
    QPixmap qrToMatrix(QPixmap image, QPoint* blocks, int size);
private:
    float lengths[3];
    float angles[3];
};

#endif
