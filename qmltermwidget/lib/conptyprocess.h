/*
 * ConPtyProcess - Windows ConPTY process management
 *
 * This class parallels KPtyProcess but uses Windows ConPTY instead of
 * Unix PTY. It extends KProcess to provide the same interface for
 * program/environment management, and adds ConPTY-specific functionality.
 *
 * Copyright (C) 2024 cool-retro-term contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#ifndef CONPTYPROCESS_H
#define CONPTYPROCESS_H

#include <QtGlobal>

#ifdef Q_OS_WIN

#include "kprocess.h"
#include "conpty.h"
#include "conptydevice.h"

#include <QProcess>
#include <QThread>

#include <memory>

class ConPty;
class ConPtyDevice;

/**
 * Thread that waits for the child process to exit and notifies the main thread.
 */
class ProcessWaitThread : public QThread
{
    Q_OBJECT

public:
    explicit ProcessWaitThread(HANDLE processHandle, QObject *parent = nullptr);
    ~ProcessWaitThread() override;

    void stop();

signals:
    void processExited(quint32 exitCode);

protected:
    void run() override;

private:
    HANDLE m_processHandle;
    volatile bool m_stopped;
};

/**
 * Windows ConPTY process manager.
 *
 * This class extends KProcess to provide the same interface (setProgram,
 * setEnv, clearProgram, etc.) but overrides start() to create a ConPTY
 * and launch the child process using CreateProcess with a pseudo console.
 *
 * It provides device() and conPty() accessor methods analogous to
 * KPtyProcess::pty().
 */
class ConPtyProcess : public KProcess
{
    Q_OBJECT

public:
    explicit ConPtyProcess(QObject *parent = nullptr);
    ~ConPtyProcess() override;

    /**
     * Start the child process with the configured program and environment,
     * creating a ConPTY for I/O.
     */
    void start();

    /**
     * Wait for the process to start.
     * @param msecs timeout in milliseconds (-1 for no timeout)
     * @return true if the process started successfully
     */
    bool waitForStarted(int msecs = 30000);

    /**
     * Wait for the process to finish.
     * @param msecs timeout in milliseconds (-1 for no timeout)
     * @return true if the process finished within the timeout
     */
    bool waitForFinished(int msecs = 30000);

    /**
     * @return true if the child process is currently running
     */
    bool isRunning() const;

    /**
     * @return the process ID of the child process, or 0 if not running
     */
    qint64 processId() const;

    /**
     * Kill the child process.
     */
    void kill();

    /**
     * Terminate the child process (same as kill on Windows).
     */
    void terminate();

    /**
     * @return the ConPtyDevice used for I/O with the child process
     */
    ConPtyDevice *device() const;

    /**
     * @return the underlying ConPty pseudoconsole wrapper
     */
    ConPty *conPty() const;

    /**
     * @return the exit status of the last process
     */
    QProcess::ExitStatus exitStatus() const;

    /**
     * @return the exit code of the last process
     */
    int exitCode() const;

    /**
     * @return the current state of the process
     */
    QProcess::ProcessState state() const;

    /**
     * Set the working directory for the child process.
     */
    void setWorkingDirectory(const QString &dir);

signals:
    // Our own signals (QProcess signals are QPrivateSignal in Qt 6.8
    // and cannot be emitted from subclasses)
    void started();
    void errorOccurred(QProcess::ProcessError error);
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

private slots:
    void onProcessExited(quint32 exitCode);

private:
    bool createProcessWithPseudoConsole();

    std::unique_ptr<ConPty> m_conPty;
    std::unique_ptr<ConPtyDevice> m_device;
    HANDLE m_processHandle;
    HANDLE m_threadHandle;
    DWORD m_processId;
    ProcessWaitThread *m_waitThread;
    QString m_workingDirectory;

    QProcess::ProcessState m_state;
    QProcess::ExitStatus m_exitStatus;
    int m_exitCode;
    bool m_killed;  // flag to track if we killed the process

    LPPROC_THREAD_ATTRIBUTE_LIST m_attrList;
};

#endif // Q_OS_WIN

#endif // CONPTYPROCESS_H
