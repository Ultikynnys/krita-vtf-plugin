// SPDX-License-Identifier: GPL-2.0-or-later
#include "vtf_gui_dispatch.h"

#include <QApplication>
#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>

namespace VtfGuiDispatch {

bool runBlocking(const std::function<void()> &action)
{
    if (!qApp || QCoreApplication::closingDown()) return false;
    QThread *const guiThread = qApp->thread();
    if (!guiThread || !guiThread->isRunning()) return false;
    if (QThread::currentThread() == guiThread) {
        action();
        return true;
    }
    return QMetaObject::invokeMethod(qApp, action, Qt::BlockingQueuedConnection);
}

} // namespace VtfGuiDispatch
