// SPDX-License-Identifier: GPL-2.0-or-later
#include "vtf_gui_dispatch.h"

#include <QApplication>
#include <QFuture>
#include <QThread>
#include <QtConcurrent>
#include <QtTest>

class TestGuiDispatch : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void runsDirectlyOnGuiThread();
    void dispatchesWorkerToGuiThread();
};

void TestGuiDispatch::runsDirectlyOnGuiThread()
{
    QThread *executedOn = nullptr;
    QVERIFY(VtfGuiDispatch::runBlocking([&]() { executedOn = QThread::currentThread(); }));
    QCOMPARE(executedOn, qApp->thread());
}

void TestGuiDispatch::dispatchesWorkerToGuiThread()
{
    QThread *executedOn = nullptr;
    QFuture<bool> future = QtConcurrent::run([&]() {
        return VtfGuiDispatch::runBlocking([&]() { executedOn = QThread::currentThread(); });
    });
    QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 5000);
    QVERIFY(future.result());
    QCOMPARE(executedOn, qApp->thread());
}

QTEST_MAIN(TestGuiDispatch)
#include "test_gui_dispatch.moc"
