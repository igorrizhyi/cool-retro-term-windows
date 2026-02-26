/*
 * ConPty - Windows Pseudo Console (ConPTY) wrapper
 *
 * Provides a C++ wrapper around the Windows ConPTY API for creating
 * and managing pseudo-console sessions on Windows 10+.
 *
 * Copyright (C) 2024 pushingpandas
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#ifndef CONPTY_H
#define CONPTY_H

#include <QtGlobal>

#ifdef Q_OS_WIN

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <QSize>
#include <QString>

/**
 * Low-level wrapper around the Windows ConPTY API.
 *
 * This class manages the lifecycle of a pseudoconsole (HPCON),
 * including creation, resizing, and destruction. It also provides
 * access to the input/output pipe handles used for communication
 * with the child process.
 */
class ConPty
{
public:
    ConPty();
    ~ConPty();

    ConPty(const ConPty &) = delete;
    ConPty &operator=(const ConPty &) = delete;

    /**
     * Create the pseudo console with the given initial size.
     *
     * @param cols number of columns
     * @param rows number of rows
     * @return true if the pseudoconsole was created successfully
     */
    bool create(int cols, int rows);

    /**
     * Close the pseudo console and release all resources.
     */
    void close();

    /**
     * Resize the pseudo console.
     *
     * @param rows number of rows
     * @param cols number of columns
     * @return true on success
     */
    bool resize(int rows, int cols);

    /**
     * @return true if the pseudoconsole has been created and is valid
     */
    bool isValid() const;

    /**
     * @return the handle used to write input to the pseudoconsole
     */
    HANDLE inputWriteHandle() const { return m_pipeIn; }

    /**
     * @return the handle used to read output from the pseudoconsole
     */
    HANDLE outputReadHandle() const { return m_pipeOut; }

    /**
     * @return the raw HPCON handle (for use with CreateProcess)
     */
    HPCON pseudoConsoleHandle() const { return m_hPC; }

    /**
     * @return current size
     */
    QSize size() const { return m_size; }

private:
    HPCON m_hPC;
    HANDLE m_pipeIn;       // write end - we write to this to send input to child
    HANDLE m_pipeOut;      // read end - we read from this to get output from child
    HANDLE m_pipeInChild;  // read end given to child process
    HANDLE m_pipeOutChild; // write end given to child process
    QSize m_size;
};

#endif // Q_OS_WIN

#endif // CONPTY_H
