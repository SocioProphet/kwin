/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#include "inputpanelclient.h"
#include "deleted.h"
#include "wayland_server.h"
#include "workspace.h"
#include <KWaylandServer/output_interface.h>
#include <KWaylandServer/seat_interface.h>
#include <KWaylandServer/surface_interface.h>

using namespace KWin;
using namespace KWaylandServer;

InputPanelV1Client::InputPanelV1Client(KWaylandServer::InputPanelSurfaceV1Interface *panelSurface)
    : WaylandClient(panelSurface->surface())
    , m_panelSurface(panelSurface)
{
    setSkipPager(true);
    setSkipTaskbar(true);
    setKeepAbove(true);
    setupCompositing();

    setObjectName(QStringLiteral("Input Panel"));

    connect(surface(), &SurfaceInterface::aboutToBeDestroyed, this, &InputPanelV1Client::destroyClient);
    connect(surface(), &SurfaceInterface::sizeChanged, this, &InputPanelV1Client::reposition);
    connect(surface(), &SurfaceInterface::mapped, this, &InputPanelV1Client::updateDepth);

    connect(panelSurface, &KWaylandServer::InputPanelSurfaceV1Interface::topLevel, this, &InputPanelV1Client::showTopLevel);
    connect(panelSurface, &KWaylandServer::InputPanelSurfaceV1Interface::overlayPanel, this, &InputPanelV1Client::showOverlayPanel);
    connect(panelSurface, &KWaylandServer::InputPanelSurfaceV1Interface::destroyed, this, &InputPanelV1Client::destroyClient);
}

void InputPanelV1Client::showOverlayPanel()
{
    m_output = nullptr;
    m_mode = Overlay;
    reposition();
}

void InputPanelV1Client::showTopLevel(KWaylandServer::OutputInterface *output, KWaylandServer::InputPanelSurfaceV1Interface::Position position)
{
    Q_UNUSED(position);
    m_mode = Toplevel;
    m_output = output;
    reposition();
}

void KWin::InputPanelV1Client::reposition()
{
    m_visible = true;
    switch (m_mode) {
        case Toplevel: {
            if (m_output) {
                const QSize panelSize = surface()->size();
                if (!panelSize.isValid() || panelSize.isEmpty())
                    return;

                QRect geo(m_output->globalPosition(), panelSize);
                geo.translate((m_output->pixelSize().width() - panelSize.width())/2, m_output->pixelSize().height() - panelSize.height());
                setFrameGeometry(geo);
            }
        }   break;
        case Overlay: {
            auto focusedField = waylandServer()->findClient(waylandServer()->seat()->focusedTextInputSurface());
            if (focusedField) {
                setFrameGeometry({focusedField->pos(), surface()->size()});
            }
        }   break;
    }
}

void InputPanelV1Client::setFrameGeometry(const QRect &geometry, ForceGeometry_t force)
{
    Q_UNUSED(force);
    if (m_frameGeometry != geometry) {
        m_frameGeometry = geometry;

        emit frameGeometryChanged(this, m_frameGeometry);
        emit clientGeometryChanged(this, m_frameGeometry);
        emit bufferGeometryChanged(this, m_frameGeometry);

        setReadyForPainting();

        Q_EMIT windowShown(this);
        autoRaise();
    }
}

void InputPanelV1Client::destroyClient()
{
    markAsZombie();

    Deleted *deleted = Deleted::create(this);
    emit windowClosed(this, deleted);
    StackingUpdatesBlocker blocker(workspace());
    waylandServer()->removeClient(this);
    deleted->unrefWindow();

    delete this;
}

void InputPanelV1Client::debug(QDebug &stream) const
{
    stream << "InputPanelClient(" << static_cast<const void*>(this) << "," << resourceClass() << m_frameGeometry << ')';
}

NET::WindowType InputPanelV1Client::windowType(bool, int) const
{
    return NET::Utility;
}

void InputPanelV1Client::hideClient(bool hide)
{
    m_visible = !hide;
    if (hide) {
        workspace()->clientHidden(this);
        emit windowHidden(this);
    } else {
        reposition();
        addRepaintFull();
    }
}

