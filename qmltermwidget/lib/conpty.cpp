/*
 * ConPty - Windows Pseudo Console (ConPTY) wrapper implementation
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

#include "conpty.h"

#include <QDebug>

ConPty::ConPty()
    : m_hPC(INVALID_HANDLE_VALUE)
    , m_pipeIn(INVALID_HANDLE_VALUE)
    , m_pipeOut(INVALID_HANDLE_VALUE)
    , m_pipeInChild(INVALID_HANDLE_VALUE)
    , m_pipeOutChild(INVALID_HANDLE_VALUE)
    , m_size(80, 24)
{
}

ConPty::~ConPty()
{
    close();
}

bool ConPty::create(int cols, int rows)
{
    if (isValid()) {
        qWarning() << "ConPty::create() - pseudoconsole already created";
        return false;
    }

    // Create the pipes for pseudoconsole I/O
    HANDLE hPipeInRead = INVALID_HANDLE_VALUE;
    HANDLE hPipeInWrite = INVALID_HANDLE_VALUE;
    HANDLE hPipeOutRead = INVALID_HANDLE_VALUE;
    HANDLE hPipeOutWrite = INVALID_HANDLE_VALUE;

    if (!CreatePipe(&hPipeInRead, &hPipeInWrite, NULL, 0)) {
        qWarning() << "ConPty::create() - failed to create input pipe, error:" << GetLastError();
        return false;
    }

    if (!CreatePipe(&hPipeOutRead, &hPipeOutWrite, NULL, 0)) {
        qWarning() << "ConPty::create() - failed to create output pipe, error:" << GetLastError();
        CloseHandle(hPipeInRead);
        CloseHandle(hPipeInWrite);
        return false;
    }

    // Create the pseudoconsole
    COORD size;
    size.X = static_cast<SHORT>(cols);
    size.Y = static_cast<SHORT>(rows);

    HRESULT hr = CreatePseudoConsole(size, hPipeInRead, hPipeOutWrite, 0, &m_hPC);

    if (FAILED(hr)) {
        qWarning() << "ConPty::create() - CreatePseudoConsole failed, hr:" << Qt::hex << hr;
        CloseHandle(hPipeInRead);
        CloseHandle(hPipeInWrite);
        CloseHandle(hPipeOutRead);
        CloseHandle(hPipeOutWrite);
        return false;
    }

    // Store the handles we need
    m_pipeIn = hPipeInWrite;       // we write input to child through this
    m_pipeOut = hPipeOutRead;      // we read output from child through this
    m_pipeInChild = hPipeInRead;   // child reads input from this
    m_pipeOutChild = hPipeOutWrite; // child writes output to this

    m_size = QSize(cols, rows);

    qDebug() << "ConPty::create() - pseudoconsole created successfully, size:" << cols << "x" << rows;
    return true;
}

void ConPty::close()
{
    if (m_hPC != INVALID_HANDLE_VALUE) {
        ClosePseudoConsole(m_hPC);
        m_hPC = INVALID_HANDLE_VALUE;
    }

    if (m_pipeIn != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipeIn);
        m_pipeIn = INVALID_HANDLE_VALUE;
    }

    if (m_pipeOut != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipeOut);
        m_pipeOut = INVALID_HANDLE_VALUE;
    }

    if (m_pipeInChild != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipeInChild);
        m_pipeInChild = INVALID_HANDLE_VALUE;
    }

    if (m_pipeOutChild != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipeOutChild);
        m_pipeOutChild = INVALID_HANDLE_VALUE;
    }
}

bool ConPty::resize(int rows, int cols)
{
    if (!isValid()) {
        qWarning() << "ConPty::resize() - pseudoconsole not created";
        return false;
    }

    COORD size;
    size.X = static_cast<SHORT>(cols);
    size.Y = static_cast<SHORT>(rows);

    HRESULT hr = ResizePseudoConsole(m_hPC, size);
    if (FAILED(hr)) {
        qWarning() << "ConPty::resize() - ResizePseudoConsole failed, hr:" << Qt::hex << hr;
        return false;
    }

    m_size = QSize(cols, rows);
    return true;
}

bool ConPty::isValid() const
{
    return m_hPC != INVALID_HANDLE_VALUE;
}

#endif // Q_OS_WIN
