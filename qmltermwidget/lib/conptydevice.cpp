/*
 * ConPtyDevice - QIODevice wrapper for Windows ConPTY implementation
 *
 * Copyright (C) 2024 cool-retro-term contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <QtGlobal>

#ifdef Q_OS_WIN

#include "conptydevice.h"
#include "conpty.h"

#include <QDebug>

// ===== ReaderThread =====

ReaderThread::ReaderThread(HANDLE readHandle, QObject *parent)
    : QThread(parent)
    , m_readHandle(readHandle)
    , m_stopped(false)
{
}

ReaderThread::~ReaderThread()
{
    stop();
}

void ReaderThread::stop()
{
    m_stopped = true;
    // Cancel any pending I/O on the read handle to unblock ReadFile
    CancelIoEx(m_readHandle, NULL);
    if (isRunning()) {
        wait(5000);
        if (isRunning()) {
            qWarning() << "ReaderThread: thread did not stop in time, terminating";
            terminate();
            wait(1000);
        }
    }
}

int ReaderThread::bytesAvailable() const
{
    QMutexLocker locker(&m_mutex);
    return m_buffer.size();
}

int ReaderThread::read(char *data, int maxSize)
{
    QMutexLocker locker(&m_mutex);
    return m_buffer.read(data, maxSize);
}

bool ReaderThread::canReadLine() const
{
    QMutexLocker locker(&m_mutex);
    return m_buffer.canReadLine();
}

int ReaderThread::readLine(char *data, int maxSize)
{
    QMutexLocker locker(&m_mutex);
    return m_buffer.readLine(data, maxSize);
}

void ReaderThread::run()
{
    const int BUFSIZE = 4096;
    char buf[BUFSIZE];

    while (!m_stopped) {
        DWORD bytesRead = 0;
        BOOL result = ReadFile(m_readHandle, buf, BUFSIZE, &bytesRead, NULL);

        if (!result || bytesRead == 0) {
            DWORD error = GetLastError();
            if (m_stopped || error == ERROR_OPERATION_ABORTED) {
                break;
            }
            if (error == ERROR_BROKEN_PIPE || !result) {
                emit readEof();
                break;
            }
            continue;
        }

        {
            QMutexLocker locker(&m_mutex);
            m_buffer.write(buf, static_cast<int>(bytesRead));
        }

        emit dataReady();
    }
}

// ===== WriterThread =====

WriterThread::WriterThread(HANDLE writeHandle, QObject *parent)
    : QThread(parent)
    , m_writeHandle(writeHandle)
    , m_stopped(false)
{
}

WriterThread::~WriterThread()
{
    stop();
}

void WriterThread::stop()
{
    m_stopped = true;
    m_waitCondition.wakeAll();
    CancelIoEx(m_writeHandle, NULL);
    if (isRunning()) {
        wait(5000);
        if (isRunning()) {
            qWarning() << "WriterThread: thread did not stop in time, terminating";
            terminate();
            wait(1000);
        }
    }
}

void WriterThread::writeData(const char *data, int size)
{
    {
        QMutexLocker locker(&m_mutex);
        m_buffer.write(data, size);
    }
    m_waitCondition.wakeOne();
}

void WriterThread::run()
{
    while (!m_stopped) {
        // Wait for data to be available
        {
            QMutexLocker locker(&m_mutex);

            // Use separate check: lock m_mutex to check, then wait
            while (!m_stopped) {
                bool empty;
                {
                    // Check buffer under lock (using the same mutex is fine here
                    // since we hold it)
                    empty = m_buffer.isEmpty();
                }
                if (!empty)
                    break;
                m_waitCondition.wait(&m_mutex);
            }

            if (m_stopped)
                break;
        }

        // Read data from buffer and write to pipe
        char buf[4096];
        int bytesToWrite = 0;

        {
            QMutexLocker locker(&m_mutex);
            if (m_buffer.isEmpty())
                continue;

            int available = qMin(m_buffer.size(), static_cast<int>(sizeof(buf)));
            bytesToWrite = m_buffer.read(buf, available);
        }

        if (bytesToWrite > 0) {
            DWORD written = 0;
            BOOL result = WriteFile(m_writeHandle, buf, static_cast<DWORD>(bytesToWrite),
                                    &written, NULL);

            if (!result) {
                DWORD error = GetLastError();
                if (m_stopped || error == ERROR_OPERATION_ABORTED) {
                    break;
                }
                qWarning() << "WriterThread: WriteFile failed, error:" << error;
                break;
            }

            if (written > 0) {
                emit bytesWritten(static_cast<qint64>(written));
            }
        }
    }
}

// ===== ConPtyDevice =====

ConPtyDevice::ConPtyDevice(QObject *parent)
    : QIODevice(parent)
    , m_pty(nullptr)
    , m_reader(nullptr)
    , m_writer(nullptr)
    , m_suspended(false)
{
}

ConPtyDevice::~ConPtyDevice()
{
    close();
}

bool ConPtyDevice::open(ConPty *pty, OpenMode mode)
{
    if (!pty || !pty->isValid()) {
        qWarning() << "ConPtyDevice::open() - invalid ConPty";
        return false;
    }

    m_pty = pty;

    // Create reader thread
    m_reader = new ReaderThread(pty->outputReadHandle(), this);
    connect(m_reader, &ReaderThread::dataReady, this, &ConPtyDevice::onDataReady,
            Qt::QueuedConnection);
    connect(m_reader, &ReaderThread::readEof, this, &ConPtyDevice::onReadEof,
            Qt::QueuedConnection);

    // Create writer thread
    m_writer = new WriterThread(pty->inputWriteHandle(), this);
    connect(m_writer, &WriterThread::bytesWritten, this, &QIODevice::bytesWritten,
            Qt::QueuedConnection);

    QIODevice::open(mode);

    // Start I/O threads
    m_reader->start();
    m_writer->start();

    qDebug() << "ConPtyDevice::open() - device opened successfully";
    return true;
}

void ConPtyDevice::close()
{
    if (m_reader) {
        m_reader->stop();
        delete m_reader;
        m_reader = nullptr;
    }

    if (m_writer) {
        m_writer->stop();
        delete m_writer;
        m_writer = nullptr;
    }

    QIODevice::close();
    m_pty = nullptr;
}

bool ConPtyDevice::isSequential() const
{
    return true;
}

bool ConPtyDevice::canReadLine() const
{
    if (!m_reader)
        return false;
    return QIODevice::canReadLine() || m_reader->canReadLine();
}

bool ConPtyDevice::atEnd() const
{
    if (!m_reader)
        return true;
    return QIODevice::atEnd() && (m_reader->bytesAvailable() == 0);
}

qint64 ConPtyDevice::bytesAvailable() const
{
    if (!m_reader)
        return 0;
    return QIODevice::bytesAvailable() + m_reader->bytesAvailable();
}

qint64 ConPtyDevice::bytesToWrite() const
{
    return 0; // Writer thread handles writing asynchronously
}

bool ConPtyDevice::waitForBytesWritten(int msecs)
{
    Q_UNUSED(msecs)
    return true; // Writer thread handles this
}

bool ConPtyDevice::waitForReadyRead(int msecs)
{
    if (!m_reader)
        return false;

    // Simple polling wait
    QElapsedTimer timer;
    timer.start();

    while (m_reader->bytesAvailable() == 0) {
        if (msecs >= 0 && timer.elapsed() >= msecs)
            return false;
        QThread::msleep(10);
    }
    return true;
}

void ConPtyDevice::setSuspended(bool suspended)
{
    m_suspended = suspended;
}

bool ConPtyDevice::isSuspended() const
{
    return m_suspended;
}

qint64 ConPtyDevice::readData(char *data, qint64 maxSize)
{
    if (!m_reader)
        return -1;
    return m_reader->read(data, static_cast<int>(qMin<qint64>(maxSize, KMAXINT)));
}

qint64 ConPtyDevice::readLineData(char *data, qint64 maxSize)
{
    if (!m_reader)
        return -1;
    return m_reader->readLine(data, static_cast<int>(qMin<qint64>(maxSize, KMAXINT)));
}

qint64 ConPtyDevice::writeData(const char *data, qint64 maxSize)
{
    if (!m_writer)
        return -1;
    m_writer->writeData(data, static_cast<int>(maxSize));
    return maxSize;
}

void ConPtyDevice::onDataReady()
{
    if (!m_suspended) {
        emit readyRead();
    }
}

void ConPtyDevice::onReadEof()
{
    emit readEof();
}

#endif // Q_OS_WIN
