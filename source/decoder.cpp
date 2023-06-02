#include "decoder.h"
#include <quirc.h>
#include <QApplication>
#include <math.h>
#include <iostream>

using namespace std;

//  B               A
//  # # # # # # # # #
//  #             #
//  #           #
//  #         #
//  #       #
//  #     #
//  #   #
//  # #
//  #
//  C
//  lengths: [AB, BC, CA]

Decoder::Decoder()
{
}

Result Decoder::decodeWithQuirc(bool *bitmap, int size)
{
    quirc_code code;
    memset(&code, 0, sizeof(code));
    code.size = size;
    code.corners[0].x = 0;
    code.corners[0].y = 0;
    code.corners[1].x = size;
    code.corners[1].y = 0;
    code.corners[2].x = size;
    code.corners[2].y = size;
    code.corners[3].x = 0;
    code.corners[3].y = size;
    for (int x = 0; x < code.size; x++)
    {
        for (int y = 0; y < code.size; y++)
        {
            int pos = y * code.size + x;
            code.cell_bitmap[pos >> 3] |= bitmap[pos] ? 0 : (1 << (pos & 7));
        }
    }
    quirc_data data;
    quirc_decode_error_t err = quirc_decode(&code, &data);
    Result result;
    if (not err)
    {
        result.success = true;
        result.raw_data = data.payload;
        result.text = QString((char *)data.payload);
    }
    else
    {
        result.success = false;
        result.text = QString(quirc_strerror(err));
    }
    result.text = result.text.toLocal8Bit().data();
    return result;
}

Result Decoder::mainSequence(const QPixmap *image, QPoint *blocks, Settings settings)
{
    Result result;
    const QPixmap decoderImage = image->copy();
    switch (settings.mode)
    {
    case 0:
        result = rotationSequence(decoderImage, blocks, settings.qr_size, settings.generate_image, settings.generate_text, false, settings.generate_log);
        if (not result.success)
        {
            QString arch_log = result.log;
            result = rotationSequence(decoderImage, blocks, settings.qr_size, settings.generate_image, settings.generate_text, true, settings.generate_log);
            result.log = arch_log + result.log;
        }
        break;
    case 1:
        result = rotationSequence(decoderImage, blocks, settings.qr_size, settings.generate_image, settings.generate_text, false, settings.generate_log);
        break;
    case 2:
        result = rotationSequence(decoderImage, blocks, settings.qr_size, settings.generate_image, settings.generate_text, true, settings.generate_log);
        break;
    default:
        result = rotationSequence(decoderImage, blocks, settings.qr_size, settings.generate_image, settings.generate_text, false, settings.generate_log);
        if (not result.success)
        {
            QString arch_log = result.log;
            result = rotationSequence(decoderImage, blocks, settings.qr_size, settings.generate_image, settings.generate_text, true, settings.generate_log);
            result.log = arch_log + result.log;
        }
        break;
    }
    cout << "TEXT_START " << result.text.toLocal8Bit().data() << " TEXT_END" << endl;
    return result;
}

Result Decoder::rotationSequence(const QPixmap image, QPoint *blocks, int size, bool generate_image, bool generate_text, bool windowing, bool generate_log)
{
    Result result;
    QString mainLog = "";
    QString blockOperationsLog = "";

    if (generate_log)
    {
        blockOperationsLog += QString("Processing blocks...\nGiven blocks: [%1; %2] [%3; %4] [%5; %6]\n").arg(blocks[0].x()).arg(blocks[0].y()).arg(blocks[1].x()).arg(blocks[1].y()).arg(blocks[2].x()).arg(blocks[2].y());
    }

    QPoint arch_blocks[3];
    for (int block = 0; block < 3; block++)
    {
        arch_blocks[block] = QPoint(blocks[block].x(), blocks[block].y());
    }

    defineLengths(blocks);

    if (generate_log)
    {
        blockOperationsLog += QString("Defined central block: [%1; %2]\n").arg(blocks[1].x()).arg(blocks[1].y());
    }

    float angle = defineRotationAngle(blocks, defineQRQuadrant(blocks));

    QTransform rotTr;
    rotTr.translate(image.width() / 2, image.height() / 2);
    rotTr.rotateRadians(angle);
    rotTr.translate(-image.width() / 2, -image.height() / 2);
    QPixmap rotatedImage = image.transformed(rotTr);

    blocks[0] = rotTr.map(blocks[0]);
    blocks[1] = rotTr.map(blocks[1]);
    blocks[2] = rotTr.map(blocks[2]);

    blocks[0] = QPoint(blocks[0].x() + (rotatedImage.width() - image.width()) / 2, blocks[0].y() + (rotatedImage.height() - image.height()) / 2);
    blocks[1] = QPoint(blocks[1].x() + (rotatedImage.width() - image.width()) / 2, blocks[1].y() + (rotatedImage.height() - image.height()) / 2);
    blocks[2] = QPoint(blocks[2].x() + (rotatedImage.width() - image.width()) / 2, blocks[2].y() + (rotatedImage.height() - image.height()) / 2);

    if (generate_log)
    {
        blockOperationsLog += QString("Rotated blocks and the image by angle %1 (%2)\nCurrent blocks: [%3; %4] [%5; %6] [%7; %8]\n").arg(angle * 180 / M_PI).arg(angle).arg(blocks[0].x()).arg(blocks[0].y()).arg(blocks[1].x()).arg(blocks[1].y()).arg(blocks[2].x()).arg(blocks[2].y());
    }

    float factor = defineShearFactor(blocks);

    QTransform shrTr;
    shrTr.shear(factor, 0);
    QPixmap processedImage = rotatedImage.transformed(shrTr);

    blocks[0] = shrTr.map(blocks[0]);
    blocks[1] = shrTr.map(blocks[1]);
    blocks[2] = shrTr.map(blocks[2]);

    blocks[0] = QPoint(blocks[0].x() + (processedImage.width() - rotatedImage.width()), blocks[0].y() + (processedImage.height() - rotatedImage.height()));
    blocks[1] = QPoint(blocks[1].x() + (processedImage.width() - rotatedImage.width()), blocks[1].y() + (processedImage.height() - rotatedImage.height()));
    blocks[2] = QPoint(blocks[2].x() + (processedImage.width() - rotatedImage.width()), blocks[2].y() + (processedImage.height() - rotatedImage.height()));

    if (generate_log)
    {
        blockOperationsLog += QString("Sheared blocks and the image by factor %1\nCurrent blocks: [%2; %3] [%4; %5] [%6; %7]\nFinished processing blocks\n").arg(factor).arg(blocks[0].x()).arg(blocks[0].y()).arg(blocks[1].x()).arg(blocks[1].y()).arg(blocks[2].x()).arg(blocks[2].y());
        mainLog += blockOperationsLog;
        mainLog += QString("Decoding image...\n");
    }

    cout << rotatedImage.width() << " " << rotatedImage.height() << " " << processedImage.width() << " " << processedImage.height() << endl;
    cout << blocks[0].x() << " " << blocks[0].y() << " " << blocks[1].x() << " " << blocks[1].y() << " " << blocks[2].x() << " " << blocks[2].y() << "   " << angle * 180 / M_PI << " " << factor << endl;
    if (windowing)
    {
        for (int window_size = 4; window_size < size; window_size++)
        {
            for (float border = 0.1; border < 0.5; border += 0.1)
            {
                result = rotationWithWindowing(processedImage.copy(), blocks, size, border, false, window_size);
                if (generate_log)
                {
                    attempt_counter++;
                    mainLog += QString("Attempt No. %1: ").arg(attempt_counter);
                    if (result.success)
                    {
                        mainLog += QString("SUCCESS; ");
                    }
                    else
                    {
                        mainLog += QString("FAIL; Error code: %1; ").arg(result.text);
                    }
                    mainLog += QString("Decoding method: Rotation with windowing; Window size: %1; Border size: %2; Average value type: Simple\n").arg(window_size).arg(border);
                }
                if (result.success)
                {
                    if (generate_log)
                    {
                        mainLog += QString("Finished decoding image. Copying block operations:\n");
                        mainLog += blockOperationsLog;
                        result.log = mainLog;
                    }
                    return result;
                }
                cout << result.text.toLocal8Bit().data() << endl;
                result = rotationWithWindowing(processedImage.copy(), blocks, size, border, true, window_size);
                if (generate_log)
                {
                    attempt_counter++;
                    mainLog += QString("Attempt No. %1: ").arg(attempt_counter);
                    if (result.success)
                    {
                        mainLog += QString("SUCCESS; ");
                    }
                    else
                    {
                        mainLog += QString("FAIL; Error code: %1; ").arg(result.text);
                    }
                    mainLog += QString("Decoding method: Rotation with windowing; Window size: %1; Border size: %2; Average value type: With correcting\n").arg(window_size).arg(border);
                }
                if (result.success)
                {
                    if (generate_log)
                    {
                        mainLog += QString("Finished decoding image. Copying block operations:\n");
                        mainLog += blockOperationsLog;
                        result.log = mainLog;
                    }
                    return result;
                }
                cout << result.text.toLocal8Bit().data() << endl;
            }
        }
    }
    else
    {
        for (float border = 0.1; border < 0.5; border += 0.1)
        {
            result = rotation(processedImage.copy(), blocks, size, border, false);
            if (generate_log)
            {
                attempt_counter++;
                mainLog += QString("Attempt No. %1: ").arg(attempt_counter);
                if (result.success)
                {
                    mainLog += QString("SUCCESS; ");
                }
                else
                {
                    mainLog += QString("FAIL; Error code: %1; ").arg(result.text);
                }
                mainLog += QString("Decoding method: Rotation; Border size: %1; Average value type: Simple\n").arg(border);
            }
            if (result.success)
            {
                if (generate_log)
                {
                    mainLog += QString("Finished decoding image. Copying block operations:\n");
                    mainLog += blockOperationsLog;
                    result.log = mainLog;
                }
                return result;
            }
            cout << result.text.toLocal8Bit().data() << endl;
            result = rotation(processedImage.copy(), blocks, size, border, true);
            if (generate_log)
            {
                attempt_counter++;
                mainLog += QString("Attempt No. %1: ").arg(attempt_counter);
                if (result.success)
                {
                    mainLog += QString("SUCCESS; ");
                }
                else
                {
                    mainLog += QString("FAIL; Error code: %1; ").arg(result.text);
                }
                mainLog += QString("Decoding method: Rotation; Border size: %1; Average value type: With correcting\n").arg(border);
            }
            if (result.success)
            {
                if (generate_log)
                {
                    mainLog += QString("Finished decoding image. Copying block operations:\n");
                    mainLog += blockOperationsLog;
                    result.log = mainLog;
                }
                return result;
            }
            cout << result.text.toLocal8Bit().data() << endl;
        }
    }

    for (int block = 0; block < 3; block++)
    {
        blocks[block] = QPoint(arch_blocks[block].x(), arch_blocks[block].y());
    }

    if (generate_log)
    {
        mainLog += QString("Decoding image failed. Copying block operations:\n");
        mainLog += blockOperationsLog;
        result.log = mainLog;
    }
    return result;
}

void Decoder::defineLengths(QPoint *blocks)
{
    lengths[0] = sqrt(pow((blocks[0].x() - blocks[1].x()), 2) + pow((blocks[0].y() - blocks[1].y()), 2));
    lengths[1] = sqrt(pow((blocks[1].x() - blocks[2].x()), 2) + pow((blocks[1].y() - blocks[2].y()), 2));
    lengths[2] = sqrt(pow((blocks[2].x() - blocks[0].x()), 2) + pow((blocks[2].y() - blocks[0].y()), 2));
    if (lengths[0] > lengths[1] and lengths[0] > lengths[2])
    {
        float buff1 = lengths[2];
        lengths[2] = lengths[0];
        lengths[0] = buff1;

        QPoint buff2 = blocks[2];
        blocks[2] = blocks[1];
        blocks[1] = buff2;
    }
    else if (lengths[1] > lengths[0] and lengths[1] > lengths[2])
    {
        float buff = lengths[2];
        lengths[2] = lengths[1];
        lengths[1] = buff;

        QPoint buff2 = blocks[0];
        blocks[0] = blocks[1];
        blocks[1] = buff2;
    }
}

int Decoder::defineQRQuadrant(QPoint *blocks)
{
    int ax = blocks[0].x() - blocks[1].x();
    int ay = blocks[0].y() - blocks[1].y();
    //  bx = 0;
    //  by = 0;
    int cx = blocks[2].x() - blocks[1].x();
    int cy = blocks[2].y() - blocks[1].y();
    if (ax >= 0 and ay >= 0 and cx <= 0 and cy >= 0) // rot to quadrant 3
    {
        return 0;
    }
    if (cx >= 0 and cy >= 0 and ax <= 0 and ay >= 0)
    {
        QPoint buff = blocks[2];
        blocks[2] = blocks[0];
        blocks[0] = buff;

        return 0;
    }
    if (ax <= 0 and ay >= 0 and cx <= 0 and cy <= 0) // rot to quadrant 2
    {
        return 1;
    }
    if (cx <= 0 and cy >= 0 and ax <= 0 and ay <= 0)
    {
        QPoint buff = blocks[2];
        blocks[2] = blocks[0];
        blocks[0] = buff;

        return 1;
    }
    if (ax <= 0 and ay <= 0 and cx >= 0 and cy <= 0) // rot to quadrant 1
    {
        return 2;
    }
    if (cx <= 0 and cy <= 0 and ax >= 0 and ay <= 0)
    {
        QPoint buff = blocks[2];
        blocks[2] = blocks[0];
        blocks[0] = buff;

        return 2;
    }
    if (ax >= 0 and ay <= 0 and cx >= 0 and cy >= 0) // rot to quadrant 4
    {
        return 3;
    }
    if (cx >= 0 and cy <= 0 and ax >= 0 and ay >= 0)
    {
        QPoint buff = blocks[2];
        blocks[2] = blocks[0];
        blocks[0] = buff;

        return 3;
    }
    if (ax >= 0 and ay >= 0 and cx >= 0 and cy >= 0)
    {
        if (ax > cx and ay < cy)
        {
            return 0;
        }
        if (ax < cx and ay > cy)
        {
            QPoint buff = blocks[2];
            blocks[2] = blocks[0];
            blocks[0] = buff;

            return 0;
        }
        else
        {
            return -1;
        }
    }
    if (ax <= 0 and ay >= 0 and cx <= 0 and cy >= 0)
    {
        if (ax > cx and ay > cy)
        {
            return 1;
        }
        if (ax < cx and ay < cy)
        {
            QPoint buff = blocks[2];
            blocks[2] = blocks[0];
            blocks[0] = buff;

            return 1;
        }
        else
        {
            return -1;
        }
    }
    if (ax <= 0 and ay <= 0 and cx <= 0 and cy <= 0)
    {
        if (ax < cx and ay > cy)
        {
            return 2;
        }
        if (ax > cx and ay < cy)
        {
            QPoint buff = blocks[2];
            blocks[2] = blocks[0];
            blocks[0] = buff;

            return 2;
        }
        else
        {
            return -1;
        }
    }
    if (ax >= 0 and ay <= 0 and cx >= 0 and cy <= 0)
    {
        if (ax < cx and ay < cy)
        {
            return 3;
        }
        if (ax > cx and ay > cy)
        {
            QPoint buff = blocks[2];
            blocks[2] = blocks[0];
            blocks[0] = buff;

            return 3;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
}

float Decoder::defineRotationAngle(QPoint *blocks, int quadrant)
{
    float angle = atan2((blocks[0].y() - blocks[1].y()), (blocks[0].x() - blocks[1].x()));
    cout << blocks[0].x() << " " << blocks[0].y() << " " << blocks[1].x() << " " << blocks[1].y() << " " << blocks[2].x() << " " << blocks[2].y() << endl;
    cout << angle * 180 / M_PI << " " << quadrant << endl;
    if (abs(angle) > M_PI /2)
    {
        if (angle < 0)
        {
        angle += M_PI;
        }
        else
        {
        angle -= M_PI / 2;
        }
    }
    if ((blocks[0].x() - blocks[1].x() > 0 and blocks[0].y() - blocks[1].y() > 0) or (blocks[0].x() - blocks[1].x() < 0 and blocks[0].y() - blocks[1].y() < 0))
    {
        angle *= -1;
    }
    else if ((blocks[0].x() - blocks[1].x() < 0 and blocks[0].y() - blocks[1].y() > 0) or (blocks[0].x() - blocks[1].x() > 0 and blocks[0].y() - blocks[1].y() < 0))
    {
        angle += M_PI / 2;
        angle *= -1;
    }
    cout << angle * 180 / M_PI << " " << quadrant << endl;
    return angle - quadrant * (M_PI / 2);
}

float Decoder::defineShearFactor(QPoint *blocks)
{
    float x1 = blocks[0].x() - blocks[1].x(), x2 = blocks[2].x() - blocks[1].x();
    float y1 = blocks[0].y() - blocks[1].y(), y2 = blocks[2].y() - blocks[1].y();
    float d1 = sqrt(x1 * x1 + y1 * y1);
    float d2 = sqrt(x2 * x2 + y2 * y2);
    float angle =  acos((x1 * x2 + y1 * y2) / (d1 * d2)) - (M_PI / 2);
    cout << angle * 180 / M_PI << endl;
    return tan(angle);
}

Result Decoder::rotation(QPixmap image, QPoint* blocks, int size, float border_size, bool correcting)
{
    int sumMatrix[size][size];
    bool bitmap[size * size];

    float average = 0;
    int area = size * size;

    int size_pixels = blocks[0].x() - blocks[1].x();
    float vertical_scale_factor = float(size_pixels) / float(blocks[2].y() - blocks[1].y());
    int pixels_per_module = ceil(float(size_pixels) / float(size - 1));
    if (pixels_per_module % 2 != 0)
    {
        pixels_per_module++;
    }
    int border = pixels_per_module * border_size;

    QPoint arch_blocks[3];
    for (int block = 0; block < 3; block++)
    {
        arch_blocks[block] = QPoint(blocks[block].x(), blocks[block].y());
    }

    float image_scale_factor = float(pixels_per_module) / (float(size_pixels) / float(size - 1));
    QImage qImage = image.scaled(int(image.width() * image_scale_factor), int(image.height() * image_scale_factor * vertical_scale_factor)).toImage();
    blocks[0] = QPoint(blocks[0].x() * image_scale_factor, blocks[0].y() * image_scale_factor * vertical_scale_factor);
    blocks[1] = QPoint(blocks[1].x() * image_scale_factor, blocks[1].y() * image_scale_factor * vertical_scale_factor);
    blocks[2] = QPoint(blocks[2].x() * image_scale_factor, blocks[2].y() * image_scale_factor * vertical_scale_factor);

    int start_x = 0;
    int start_y = 0;
    if (blocks[1].x() != 0 and blocks[1].y() != 0)
    {
        start_x = blocks[1].x() - pixels_per_module / 2;
        start_y = blocks[1].y() - pixels_per_module / 2;
    }

    for (int module_y = 0; module_y < size; module_y++)
    {
        for (int module_x = 0; module_x < size; module_x++)
        {
            if (correcting)
            {
                sumMatrix[module_y][module_x] = 0;
                int pixels_checked = 0;
                int i;
                int left = border;
                int right = pixels_per_module - border;
                int top = border;
                int bottom = pixels_per_module - border;

                while (left <= right && top <= bottom)
                {
                    for (i = left; i <= right; ++i)
                    {
                        int element = qGray(qImage.pixel(start_x + module_x * pixels_per_module + i, start_y + module_y * pixels_per_module + top));
                        if (pixels_checked > (pixels_per_module - 1) * 4 and ((sumMatrix[module_y][module_x] / pixels_checked) * 0.8 > element or (sumMatrix[module_y][module_x] / pixels_checked) * 1.2 < element))
                        {
                            element = sumMatrix[module_y][module_x] / pixels_checked;
                        }
                        sumMatrix[module_y][module_x] += element;
                        qImage.setPixelColor(start_x + module_x * pixels_per_module + i, start_y + module_y * pixels_per_module + top, Qt::blue);
                        pixels_checked++;
                    }
                    top++;
                    for (i = top; i <= bottom; ++i)
                    {
                        int element = qGray(qImage.pixel(start_x + module_x * pixels_per_module + right, start_y + module_y * pixels_per_module + i));
                        if (pixels_checked > (pixels_per_module - 1) * 4 and ((sumMatrix[module_y][module_x] / pixels_checked) * 0.8 > element or (sumMatrix[module_y][module_x] / pixels_checked) * 1.2 < element))
                        {
                            element = sumMatrix[module_y][module_x] / pixels_checked;
                        }
                        sumMatrix[module_y][module_x] += element;
                        qImage.setPixelColor(start_x + module_x * pixels_per_module + right, start_y + module_y * pixels_per_module + i, Qt::blue);
                        pixels_checked++;
                    }
                    right--;
                    if (top <= bottom)
                    {
                        for (i = right; i >= left; --i)
                        {
                            int element = qGray(qImage.pixel(start_x + module_x * pixels_per_module + i, start_y + module_y * pixels_per_module + bottom));
                            if (pixels_checked > (pixels_per_module - 1) * 4 and ((sumMatrix[module_y][module_x] / pixels_checked) * 0.8 > element or (sumMatrix[module_y][module_x] / pixels_checked) * 1.2 < element))
                            {
                                element = sumMatrix[module_y][module_x] / pixels_checked;
                            }
                            sumMatrix[module_y][module_x] += element;
                            qImage.setPixelColor(start_x + module_x * pixels_per_module + i, start_y + module_y * pixels_per_module + bottom, Qt::blue);
                            pixels_checked++;
                        }
                        bottom--;
                    }
                    if (left <= right)
                    {
                        for (i = bottom; i >= top; --i)
                        {
                            int element = qGray(qImage.pixel(start_x + module_x * pixels_per_module + left, start_y + module_y * pixels_per_module + i));
                            if (pixels_checked > (pixels_per_module - 1) * 4 and ((sumMatrix[module_y][module_x] / pixels_checked) * 0.8 > element or (sumMatrix[module_y][module_x] / pixels_checked) * 1.2 < element))
                            {
                                element = sumMatrix[module_y][module_x] / pixels_checked;
                            }
                            sumMatrix[module_y][module_x] += element;
                            qImage.setPixelColor(start_x + module_x * pixels_per_module + left, start_y + module_y * pixels_per_module + i, Qt::blue);
                            pixels_checked++;
                        }
                        left++;
                    }
                }
            }
            else
            {
                sumMatrix[module_y][module_x] = 0;
                for (int pixel_y = border; pixel_y < pixels_per_module - border; pixel_y++)
                {
                    for (int pixel_x = border; pixel_x < pixels_per_module - border; pixel_x++)
                    {
                        sumMatrix[module_y][module_x] += qGray(qImage.pixel(start_x + module_x * pixels_per_module + pixel_x, start_y + module_y * pixels_per_module + pixel_y));
                        qImage.setPixelColor(start_x + module_x * pixels_per_module + pixel_x, start_y + module_y * pixels_per_module + pixel_y, Qt::blue);
                    }
                }
            }
            average += sumMatrix[module_y][module_x] / float(area);
        }
    }
    for (int module_y = 0; module_y < size; module_y++)
    {
        for (int module_x = 0; module_x < size; module_x++)
        {
            if (sumMatrix[module_y][module_x] > average)
            {
                bitmap[module_y * size + module_x] = true;
            }
            else
            {
                bitmap[module_y * size + module_x] = false;
            }
        }
    }
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            if (bitmap[y * size + x])
            {
                 cout << "  ";
            }
            else
            {
                cout << "██";
            }
        }
        cout << endl;
    }
    cout << endl;
    cout << start_x << " " << start_y << " " << pixels_per_module << " " << image_scale_factor << " " << size_pixels << endl;
    cout << correcting << " " << border_size << endl;

//    if (correcting)
//    {
//        for (int block = 0; block < 3; block++)
//        {
//            qImage.setPixelColor(blocks[block].x() - 1, blocks[block].y() - 1, Qt::red);
//            qImage.setPixelColor(blocks[block].x() - 1, blocks[block].y(), Qt::red);
//            qImage.setPixelColor(blocks[block].x() - 1, blocks[block].y() + 1, Qt::red);
//            qImage.setPixelColor(blocks[block].x(), blocks[block].y() - 1, Qt::red);
//            qImage.setPixelColor(blocks[block].x(), blocks[block].y(), Qt::red);
//            qImage.setPixelColor(blocks[block].x(), blocks[block].y() + 1, Qt::red);
//            qImage.setPixelColor(blocks[block].x() + 1, blocks[block].y() - 1, Qt::red);
//            qImage.setPixelColor(blocks[block].x() + 1, blocks[block].y(), Qt::red);
//            qImage.setPixelColor(blocks[block].x() + 1, blocks[block].y() + 1, Qt::red);
//        }
//        qImage.save(QString("_dec_%1_%2.png").arg(correcting).arg(border_size));
//    }

    for (int block = 0; block < 3; block++)
    {
        blocks[block] = QPoint(arch_blocks[block].x(), arch_blocks[block].y());
    }

    Result result = decodeWithQuirc(bitmap, size);
    if (result.success)
    {
        result.image = qImage;
    }

    return result;
}

Result Decoder::rotationWithWindowing(QPixmap image, QPoint *blocks, int size, float border_size, bool correcting, int window_size)
{
    int sumMatrix[size][size];
    int cntMatrix[size][size];
    bool bitmap[size * size];

    int size_pixels = blocks[0].x() - blocks[1].x();
    float vertical_scale_factor = float(size_pixels) / float(blocks[2].y() - blocks[1].y());
    int pixels_per_module = ceil(float(size_pixels) / float(size - 1));
    if (pixels_per_module % 2 != 0)
    {
        pixels_per_module++;
    }
    int border = pixels_per_module * border_size;

    QPoint arch_blocks[3];
    for (int block = 0; block < 3; block++)
    {
        arch_blocks[block] = QPoint(blocks[block].x(), blocks[block].y());
    }

    float image_scale_factor = float(pixels_per_module) / (float(size_pixels) / float(size - 1));
    QImage qImage = image.scaled(int(image.width() * image_scale_factor), int(image.height() * image_scale_factor * vertical_scale_factor)).toImage();
    blocks[0] = QPoint(blocks[0].x() * image_scale_factor, blocks[0].y() * image_scale_factor * vertical_scale_factor);
    blocks[1] = QPoint(blocks[1].x() * image_scale_factor, blocks[1].y() * image_scale_factor * vertical_scale_factor);
    blocks[2] = QPoint(blocks[2].x() * image_scale_factor, blocks[2].y() * image_scale_factor * vertical_scale_factor);

    int start_x = 0;
    int start_y = 0;
    if (blocks[1].x() != 0 and blocks[1].y() != 0)
    {
        start_x = blocks[1].x() - pixels_per_module / 2;
        start_y = blocks[1].y() - pixels_per_module / 2;
    }
    float area = window_size * window_size;
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            cntMatrix[y][x] = 0;
        }
    }
    for (int module_y = 0; module_y < size; module_y++)
    {
        for (int module_x = 0; module_x < size; module_x++)
        {
            if (correcting)
            {
                sumMatrix[module_y][module_x] = 0;
                int pixels_checked = 0;
                int i;
                int left = border;
                int right = pixels_per_module - border;
                int top = border;
                int bottom = pixels_per_module - border;

                while (left <= right && top <= bottom)
                {
                    for (i = left; i <= right; ++i)
                    {
                        int element = qGray(qImage.pixel(start_x + module_x * pixels_per_module + i, start_y + module_y * pixels_per_module + top));
                        if (pixels_checked > (pixels_per_module - 1) * 4 and ((sumMatrix[module_y][module_x] / pixels_checked) * 0.8 > element or (sumMatrix[module_y][module_x] / pixels_checked) * 1.2 < element))
                        {
                            element = sumMatrix[module_y][module_x] / pixels_checked;
                        }
                        sumMatrix[module_y][module_x] += element;
                        qImage.setPixelColor(start_x + module_x * pixels_per_module + i, start_y + module_y * pixels_per_module + top, Qt::blue);
                        pixels_checked++;
                    }
                    top++;
                    for (i = top; i <= bottom; ++i)
                    {
                        int element = qGray(qImage.pixel(start_x + module_x * pixels_per_module + right, start_y + module_y * pixels_per_module + i));
                        if (pixels_checked > (pixels_per_module - 1) * 4 and ((sumMatrix[module_y][module_x] / pixels_checked) * 0.8 > element or (sumMatrix[module_y][module_x] / pixels_checked) * 1.2 < element))
                        {
                            element = sumMatrix[module_y][module_x] / pixels_checked;
                        }
                        sumMatrix[module_y][module_x] += element;
                        qImage.setPixelColor(start_x + module_x * pixels_per_module + right, start_y + module_y * pixels_per_module + i, Qt::blue);
                        pixels_checked++;
                    }
                    right--;
                    if (top <= bottom)
                    {
                        for (i = right; i >= left; --i)
                        {
                            int element = qGray(qImage.pixel(start_x + module_x * pixels_per_module + i, start_y + module_y * pixels_per_module + bottom));
                            if (pixels_checked > (pixels_per_module - 1) * 4 and ((sumMatrix[module_y][module_x] / pixels_checked) * 0.8 > element or (sumMatrix[module_y][module_x] / pixels_checked) * 1.2 < element))
                            {
                                element = sumMatrix[module_y][module_x] / pixels_checked;
                            }
                            sumMatrix[module_y][module_x] += element;
                            qImage.setPixelColor(start_x + module_x * pixels_per_module + i, start_y + module_y * pixels_per_module + bottom, Qt::blue);
                            pixels_checked++;
                        }
                        bottom--;
                    }
                    if (left <= right)
                    {
                        for (i = bottom; i >= top; --i)
                        {
                            int element = qGray(qImage.pixel(start_x + module_x * pixels_per_module + left, start_y + module_y * pixels_per_module + i));
                            if (pixels_checked > (pixels_per_module - 1) * 4 and ((sumMatrix[module_y][module_x] / pixels_checked) * 0.8 > element or (sumMatrix[module_y][module_x] / pixels_checked) * 1.2 < element))
                            {
                                element = sumMatrix[module_y][module_x] / pixels_checked;
                            }
                            sumMatrix[module_y][module_x] += element;
                            qImage.setPixelColor(start_x + module_x * pixels_per_module + left, start_y + module_y * pixels_per_module + i, Qt::blue);
                            pixels_checked++;
                        }
                        left++;
                    }
                }
            }
            else
            {
                sumMatrix[module_y][module_x] = 0;
                for (int pixel_y = border; pixel_y < pixels_per_module - border; pixel_y++)
                {
                    for (int pixel_x = border; pixel_x < pixels_per_module - border; pixel_x++)
                    {
                        sumMatrix[module_y][module_x] += qGray(qImage.pixel(start_x + module_x * pixels_per_module + pixel_x, start_y + module_y * pixels_per_module + pixel_y));
                        qImage.setPixelColor(start_x + module_x * pixels_per_module + pixel_x, start_y + module_y * pixels_per_module + pixel_y, Qt::blue);
                    }
                }
            }
        }
    }
    for (int window_y = 0; window_y < size - window_size + 1; window_y++)
    {
        for (int window_x = 0; window_x < size - window_size + 1; window_x++)
        {
            float sum = 0;
            for (int module_y = 0; module_y < window_size; module_y++)
            {
                for (int module_x = 0; module_x < window_size; module_x++)
                {
                    sum += sumMatrix[window_y + module_y][window_x + module_x];
                }
            }
            int filter = sum / area;
            for (int module_y = 0; module_y < window_size; module_y++)
            {
                for (int module_x = 0; module_x < window_size; module_x++)
                {
                    if (sumMatrix[window_y + module_y][window_x + module_x] > filter)
                    {
                        cntMatrix[window_y + module_y][window_x + module_x] -= 1;
                    }
                    else
                    {
                        cntMatrix[window_y + module_y][window_x + module_x] += 1;
                    }
                }
            }
        }
    }
    for (int module_y = 0; module_y < size; module_y++)
    {
        for (int module_x = 0; module_x < size; module_x++)
        {
            if (cntMatrix[module_y][module_x] <= 0)
            {
                bitmap[module_y * size + module_x] = true;
            }
            else
            {
                bitmap[module_y * size + module_x] = false;
            }
        }
    }
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            if (bitmap[y * size + x])
            {
                cout << "  ";
            }
            else
            {
                cout << "██";
            }
        }
        cout << endl;
    }
    cout << endl;
    cout << start_x << " " << start_y << " " << pixels_per_module << " " << image_scale_factor << " " << vertical_scale_factor << " " << size_pixels << endl;
    cout << correcting << " " << border_size << " " << window_size << endl;

    for (int block = 0; block < 3; block++)
    {
        blocks[block] = QPoint(arch_blocks[block].x(), arch_blocks[block].y());
    }

    Result result = decodeWithQuirc(bitmap, size);
    if (result.success)
    {
        result.image = qImage;
    }

    return result;
}
