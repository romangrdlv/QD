#ifndef DECODER_H
#define DECODER_H

#include "decoderpopup.h"
#include <QWidget>

struct Result
{
    bool success;
    QImage image;
    uint8_t *raw_data;
    QString text;
    QString log;
};

class Decoder
{
public:
    Decoder();
    Result mainSequence(const QPixmap* image, QPoint* blocks, Settings settings);
private:
    Result rotationSequence(const QPixmap image, QPoint* blocks, int size, bool generate_image, bool generate_text, bool windowing, bool show_debug_popup);
    Result decodeWithQuirc(bool *bitmap, int size);
    void defineLengths(QPoint* blocks);
    int defineQRQuadrant(QPoint* blocks);
    float defineRotationAngle(QPoint* blocks, int quadrant);
    float defineShearFactor(QPoint *blocks);
    Result rotation(QPixmap image, QPoint* blocks, int size, float border_size, bool correcting);
    Result rotationWithWindowing(QPixmap image, QPoint* blocks, int size, float border_size, bool correcting, int window_size);
private:
    float lengths[3];
    int attempt_counter = 0;
};

#endif
