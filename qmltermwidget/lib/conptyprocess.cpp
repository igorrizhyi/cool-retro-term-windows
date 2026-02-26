/*
 * ConPtyProcess - Windows ConPTY process management implementation
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

#include "conptyprocess.h"

#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

// ===== ProcessWaitThread =====

ProcessWaitThread::ProcessWaitThread(HANDLE processHandle, QObject *parent)
    : QThread(parent)
    , m_processHandle(processHandle)
    , m_stopped(false)
{
}

ProcessWaitThread::~ProcessWaitThread()
{
    stop();
}

void ProcessWaitThread::stop()
{
    m_stopped = true;
    if (isRunning()) {
        wait(5000);
        if (isRunning()) {
            terminate();
            wait(1000);
        }
    }
}

void ProcessWaitThread::run()
{
    DWORD result = WaitForSingleObject(m_processHandle, INFINITE);

    if (m_stopped)
        return;

    if (result == WAIT_OBJECT_0) {
        DWORD exitCode = 0;
        GetExitCodeProcess(m_processHandle, &exitCode);
        emit processExited(static_cast<quint32>(exitCode));
    }
}

// ===== ConPtyProcess =====

ConPtyProcess::ConPtyProcess(QObject *parent)
    : KProcess(parent)
    , m_processHandle(INVALID_HANDLE_VALUE)
    , m_threadHandle(INVALID_HANDLE_VALUE)
    , m_processId(0)
    , m_waitThread(nullptr)
    , m_state(QProcess::NotRunning)
    , m_exitStatus(QProcess::NormalExit)
    , m_exitCode(0)
    , m_killed(false)
    , m_attrList(nullptr)
{
    m_conPty = std::make_unique<ConPty>();
    m_device = std::make_unique<ConPtyDevice>(this);
}

ConPtyProcess::~ConPtyProcess()
{
    if (m_state == QProcess::Running) {
        kill();
        waitForFinished(3000);
    }

    if (m_waitThread) {
        m_waitThread->stop();
        delete m_waitThread;
        m_waitThread = nullptr;
    }

    m_device->close();
    m_conPty->close();

    if (m_processHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_processHandle);
        m_processHandle = INVALID_HANDLE_VALUE;
    }

    if (m_threadHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_threadHandle);
        m_threadHandle = INVALID_HANDLE_VALUE;
    }

    if (m_attrList) {
        DeleteProcThreadAttributeList(m_attrList);
        HeapFree(GetProcessHeap(), 0, m_attrList);
        m_attrList = nullptr;
    }
}

void ConPtyProcess::start()
{
    if (m_state == QProcess::Running) {
        qWarning() << "ConPtyProcess::start() - process already running";
        return;
    }

    m_killed = false;

    // Create the pseudoconsole with default size (will be resized later)
    if (!m_conPty->create(80, 24)) {
        qWarning() << "ConPtyProcess::start() - failed to create pseudoconsole";
        m_state = QProcess::NotRunning;
        emit errorOccurred(QProcess::FailedToStart);
        return;
    }

    // Open the device for I/O
    if (!m_device->open(m_conPty.get())) {
        qWarning() << "ConPtyProcess::start() - failed to open device";
        m_conPty->close();
        m_state = QProcess::NotRunning;
        emit errorOccurred(QProcess::FailedToStart);
        return;
    }

    // Create the child process
    if (!createProcessWithPseudoConsole()) {
        qWarning() << "ConPtyProcess::start() - failed to create process";
        m_device->close();
        m_conPty->close();
        m_state = QProcess::NotRunning;
        emit errorOccurred(QProcess::FailedToStart);
        return;
    }

    m_state = QProcess::Running;
    emit started();

    // Start a thread to wait for process exit
    m_waitThread = new ProcessWaitThread(m_processHandle, this);
    connect(m_waitThread, &ProcessWaitThread::processExited,
            this, &ConPtyProcess::onProcessExited, Qt::QueuedConnection);
    m_waitThread->start();
}

bool ConPtyProcess::waitForStarted(int msecs)
{
    Q_UNUSED(msecs)
    return m_state == QProcess::Running;
}

bool ConPtyProcess::waitForFinished(int msecs)
{
    if (m_state != QProcess::Running)
        return true;

    if (m_processHandle == INVALID_HANDLE_VALUE)
        return true;

    DWORD timeout = (msecs < 0) ? INFINITE : static_cast<DWORD>(msecs);
    DWORD result = WaitForSingleObject(m_processHandle, timeout);

    if (result == WAIT_OBJECT_0) {
        DWORD exitCode = 0;
        GetExitCodeProcess(m_processHandle, &exitCode);
        // Process the exit inline since we're waiting synchronously
        onProcessExited(static_cast<quint32>(exitCode));
        return true;
    }

    return false;
}

bool ConPtyProcess::isRunning() const
{
    return m_state == QProcess::Running;
}

qint64 ConPtyProcess::processId() const
{
    return static_cast<qint64>(m_processId);
}

void ConPtyProcess::kill()
{
    if (m_processHandle != INVALID_HANDLE_VALUE) {
        m_killed = true;
        TerminateProcess(m_processHandle, 1);
    }
}

void ConPtyProcess::terminate()
{
    // On Windows, terminate and kill are the same
    kill();
}

ConPtyDevice *ConPtyProcess::device() const
{
    return m_device.get();
}

ConPty *ConPtyProcess::conPty() const
{
    return m_conPty.get();
}

QProcess::ExitStatus ConPtyProcess::exitStatus() const
{
    return m_exitStatus;
}

int ConPtyProcess::exitCode() const
{
    return m_exitCode;
}

QProcess::ProcessState ConPtyProcess::state() const
{
    return m_state;
}

void ConPtyProcess::setWorkingDirectory(const QString &dir)
{
    m_workingDirectory = dir;
    KProcess::setWorkingDirectory(dir);
}

void ConPtyProcess::onProcessExited(quint32 exitCode)
{
    if (m_state != QProcess::Running)
        return;

    m_exitCode = static_cast<int>(exitCode);
    m_exitStatus = m_killed ? QProcess::CrashExit : QProcess::NormalExit;
    m_state = QProcess::NotRunning;

    // Clean up the wait thread
    if (m_waitThread) {
        m_waitThread->stop();
        delete m_waitThread;
        m_waitThread = nullptr;
    }

    // Close the device and pseudoconsole
    m_device->close();
    m_conPty->close();

    emit finished(m_exitCode, m_exitStatus);
}

bool ConPtyProcess::createProcessWithPseudoConsole()
{
    // Build the command line
    QString program = KProcess::program().join(QLatin1Char(' '));
    if (program.isEmpty()) {
        // If no program set, use cmd.exe as default
        program = QStringLiteral("cmd.exe");
    }

    // Build environment block
    QProcessEnvironment env = processEnvironment();
    if (env.isEmpty()) {
        env = QProcessEnvironment::systemEnvironment();
    }

    // Convert environment to Windows format (null-separated, double-null terminated)
    QStringList envList = env.toStringList();
    QString envBlock;
    for (const QString &var : envList) {
        envBlock += var;
        envBlock += QChar(QLatin1Char('\0'));
    }
    envBlock += QChar(QLatin1Char('\0'));

    // Initialize the thread attribute list for the pseudoconsole
    SIZE_T attrListSize = 0;
    InitializeProcThreadAttributeList(NULL, 1, 0, &attrListSize);

    m_attrList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
        HeapAlloc(GetProcessHeap(), 0, attrListSize));

    if (!m_attrList) {
        qWarning() << "ConPtyProcess: failed to allocate attribute list";
        return false;
    }

    if (!InitializeProcThreadAttributeList(m_attrList, 1, 0, &attrListSize)) {
        qWarning() << "ConPtyProcess: InitializeProcThreadAttributeList failed, error:"
                    << GetLastError();
        HeapFree(GetProcessHeap(), 0, m_attrList);
        m_attrList = nullptr;
        return false;
    }

    HPCON hPC = m_conPty->pseudoConsoleHandle();
    if (!UpdateProcThreadAttribute(m_attrList, 0,
                                    PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                    hPC, sizeof(HPCON), NULL, NULL)) {
        qWarning() << "ConPtyProcess: UpdateProcThreadAttribute failed, error:"
                    << GetLastError();
        DeleteProcThreadAttributeList(m_attrList);
        HeapFree(GetProcessHeap(), 0, m_attrList);
        m_attrList = nullptr;
        return false;
    }

    // Set up STARTUPINFOEX
    STARTUPINFOEXW si;
    ZeroMemory(&si, sizeof(si));
    si.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    si.lpAttributeList = m_attrList;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    // Determine working directory
    QString workDir = m_workingDirectory;
    if (workDir.isEmpty()) {
        workDir = QDir::currentPath();
    }

    // Create the child process
    std::wstring cmdLine = program.toStdWString();
    std::wstring workDirW = QDir::toNativeSeparators(workDir).toStdWString();
    std::wstring envBlockW = envBlock.toStdWString();

    BOOL success = CreateProcessW(
        NULL,                                          // application name (use command line)
        const_cast<wchar_t *>(cmdLine.c_str()),        // command line
        NULL,                                          // process security attributes
        NULL,                                          // thread security attributes
        FALSE,                                         // inherit handles
        EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT,  // creation flags
        const_cast<wchar_t *>(envBlockW.c_str()),      // environment
        workDirW.c_str(),                              // current directory
        &si.StartupInfo,                               // startup info
        &pi                                            // process information
    );

    if (!success) {
        qWarning() << "ConPtyProcess: CreateProcessW failed, error:" << GetLastError()
                    << "command:" << program;
        DeleteProcThreadAttributeList(m_attrList);
        HeapFree(GetProcessHeap(), 0, m_attrList);
        m_attrList = nullptr;
        return false;
    }

    m_processHandle = pi.hProcess;
    m_threadHandle = pi.hThread;
    m_processId = pi.dwProcessId;

    qDebug() << "ConPtyProcess: process created successfully, PID:" << m_processId
             << "command:" << program;

    return true;
}

#endif // Q_OS_WIN
