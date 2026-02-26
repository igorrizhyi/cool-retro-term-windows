/*
 * ConPtyDevice - QIODevice wrapper for Windows ConPTY
 *
 * Provides a QIODevice interface for reading/writing to a ConPTY,
 * analogous to KPtyDevice for Unix PTYs. Uses background threads
 * for non-blocking I/O with Windows pipe handles.
 *
 * Copyright (C) 2024 cool-retro-term contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#ifndef CONPTYDEVICE_H
#define CONPTYDEVICE_H

#include <QtGlobal>

#ifdef Q_OS_WIN

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <QIODevice>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "ringbuffer.h"

class ConPty;

/**
 * Background thread that reads from the ConPTY output pipe
 * and buffers data into a KRingBuffer for the main thread.
 */
class ReaderThread : public QThread
{
    Q_OBJECT

public:
    explicit ReaderThread(HANDLE readHandle, QObject *parent = nullptr);
    ~ReaderThread() override;

    void stop();

    int bytesAvailable() const;
    int read(char *data, int maxSize);
    bool canReadLine() const;
    int readLine(char *data, int maxSize);

signals:
    void dataReady();
    void readEof();

protected:
    void run() override;

private:
    HANDLE m_readHandle;
    KRingBuffer m_buffer;
    mutable QMutex m_mutex;
    volatile bool m_stopped;
};

/**
 * Background thread that writes buffered data to the ConPTY input pipe.
 * Uses a separate mutex for buffer emptiness check vs the wake condition
 * to prevent deadlock.
 */
class WriterThread : public QThread
{
    Q_OBJECT

public:
    explicit WriterThread(HANDLE writeHandle, QObject *parent = nullptr);
    ~WriterThread() override;

    void stop();
    void writeData(const char *data, int size);

signals:
    void bytesWritten(qint64 bytes);

protected:
    void run() override;

private:
    HANDLE m_writeHandle;
    KRingBuffer m_buffer;
    QMutex m_mutex;
    QMutex m_bufferCheckMutex;  // separate mutex for buffer check to avoid deadlock
    QWaitCondition m_waitCondition;
    volatile bool m_stopped;
};

/**
 * QIODevice implementation for ConPTY.
 *
 * This class wraps a ConPty instance and provides the standard QIODevice
 * interface (read/write/readyRead signals) using background I/O threads.
 * It serves the same role as KPtyDevice but for Windows ConPTY.
 */
class ConPtyDevice : public QIODevice
{
    Q_OBJECT

public:
    explicit ConPtyDevice(QObject *parent = nullptr);
    ~ConPtyDevice() override;

    /**
     * Open the device for reading and writing using the given ConPty.
     * The ConPty must already be created.
     */
    bool open(ConPty *pty, OpenMode mode = ReadWrite);

    /**
     * Close the device and stop I/O threads.
     */
    void close() override;

    bool isSequential() const override;
    bool canReadLine() const override;
    bool atEnd() const override;
    qint64 bytesAvailable() const override;
    qint64 bytesToWrite() const override;

    bool waitForBytesWritten(int msecs = -1) override;
    bool waitForReadyRead(int msecs = -1) override;

    /**
     * Set whether the device is suspended (not reading data).
     */
    void setSuspended(bool suspended);

    /**
     * @return true if the device is suspended
     */
    bool isSuspended() const;

signals:
    void readEof();

protected:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 readLineData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;

private slots:
    void onDataReady();
    void onReadEof();

private:
    ConPty *m_pty;
    ReaderThread *m_reader;
    WriterThread *m_writer;
    bool m_suspended;
};

#endif // Q_OS_WIN

#endif // CONPTYDEVICE_H
