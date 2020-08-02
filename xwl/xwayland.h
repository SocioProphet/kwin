/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2019 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef KWIN_XWL_XWAYLAND
#define KWIN_XWL_XWAYLAND

#include "xwayland_interface.h"

#include <xcb/xproto.h>

class QProcess;

class xcb_screen_t;

namespace KWin
{
class ApplicationWaylandAbstract;

namespace Xwl
{
class DataBridge;

class Xwayland : public XwaylandInterface
{
    Q_OBJECT

public:
    static Xwayland *self();

    Xwayland(ApplicationWaylandAbstract *app, QObject *parent = nullptr);
    ~Xwayland() override;

    void init();
    void prepareDestroy();

    xcb_screen_t *xcbScreen() const {
        return m_xcbScreen;
    }
    const xcb_query_extension_reply_t *xfixes() const {
        return m_xfixes;
    }

Q_SIGNALS:
    void initialized();
    void criticalError(int code);

private:
    void createX11Connection();
    void destroyX11Connection();
    void continueStartupWithX();

    DragEventReply dragMoveFilter(Toplevel *target, const QPoint &pos) override;

    int m_xcbConnectionFd = -1;
    QProcess *m_xwaylandProcess = nullptr;
    QMetaObject::Connection m_xwaylandFailConnection;

    xcb_screen_t *m_xcbScreen = nullptr;
    const xcb_query_extension_reply_t *m_xfixes = nullptr;
    DataBridge *m_dataBridge = nullptr;

    ApplicationWaylandAbstract *m_app;

    Q_DISABLE_COPY(Xwayland)
};

} // namespace Xwl
} // namespace KWin

#endif
