/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpointingdevice.h"
#include "qpointingdevice_p.h"
#include <QList>
#include <QLoggingCategory>
#include <QMutex>
#include <QCoreApplication>

#include <private/qdebug_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaInputDevices)

/*!
    \class QPointingDevice
    \brief The QPointingDevice class describes a device from which mouse, touch or tablet events originate.
    \since 6.0
    \ingroup events
    \inmodule QtGui

    Each QPointerEvent contains a QPointingDevice pointer to allow accessing
    device-specific properties like type and capabilities. It is the
    responsibility of the platform or generic plug-ins to register the
    available pointing devices via QWindowSystemInterface before generating any
    pointer events. Applications do not need to instantiate this class, they
    should just access the global instances pointed to by QPointerEvent::device().
*/

/*! \enum QInputDevice::DeviceType

    This enum represents the type of device that generated a QPointerEvent.

    \value Unknown
        The device cannot be identified.

    \value Mouse
        A mouse.

    \value TouchScreen
        In this type of device, the touch surface and display are integrated.
        This means the surface and display typically have the same size, such
        that there is a direct relationship between the touch points' physical
        positions and the coordinate reported by QEventPoint. As a
        result, Qt allows the user to interact directly with multiple QWidgets,
        QGraphicsItems, or Qt Quick Items at the same time.

    \value TouchPad
        In this type of device, the touch surface is separate from the display.
        There is not a direct relationship between the physical touch location
        and the on-screen coordinates. Instead, they are calculated relative to
        the current mouse position, and the user must use the touch-pad to move
        this reference point. Unlike touch-screens, Qt allows users to only
        interact with a single QWidget or QGraphicsItem at a time.

    \value Stylus
        A pen-like device used on a graphics tablet such as a Wacom tablet,
        or on a touchscreen that provides a separate stylus sensing capability.

    \value Airbrush
        A stylus with a thumbwheel to adjust
        \l {QTabletEvent::tangentialPressure}{tangentialPressure}.

    \value Puck
        A device that is similar to a flat mouse with a transparent circle with
        cross-hairs.

    \value Keyboard
        A keyboard.

    \value AllDevices
        Any of the above (used as a default filter value).
*/

/*! \enum QPointingDevice::PointerType

    This enum represents what is interacting with the pointing device.

    There is some redundancy between this property and \l {QInputDevice::DeviceType}.
    For example, if a touchscreen is used, then the \c DeviceType is
    \c TouchScreen and \c PointerType is \c Finger (always). But on a graphics
    tablet, it's often possible for both ends of the stylus to be used, and
    programs need to distinguish them. Therefore the concept is extended so
    that every QPointerEvent has a PointerType, and it can simplify some event
    handling code to ignore the DeviceType and react differently depending on
    the PointerType alone.

    Valid values are:

    \value Unknown
        The pointer type is unknown.
    \value Generic
        A mouse or something acting like a mouse (the core pointer on X11).
    \value Finger
        The user's finger.
    \value Pen
        The drawing end of a stylus.
    \value Eraser
        The other end of the stylus (if it has a virtual eraser on the other end).
    \value Cursor
        A transparent circle with cross-hairs as found on a
        \l {QInputDevice::DeviceType}{Puck} device.
    \value AllPointerTypes
        Any of the above (used as a default filter value).
*/

/*!
    Creates a new invalid pointing device instance.
*/
QPointingDevice::QPointingDevice()
    : QInputDevice(*(new QPointingDevicePrivate(QLatin1String("unknown"), -1,
                                              DeviceType::Unknown, PointerType::Unknown,
                                              Capability::None, 0, 0)))
{
}

QPointingDevice::~QPointingDevice()
{
}

/*!
    Creates a new pointing device instance with the given
    \a name, \a deviceType, \a pointerType, \a capabilities, \a maxPoints,
    \a buttonCount, \a seatName, \a uniqueId and \a parent.
*/
QPointingDevice::QPointingDevice(const QString &name, qint64 id, QInputDevice::DeviceType deviceType,
                                 QPointingDevice::PointerType pointerType, Capabilities capabilities, int maxPoints, int buttonCount,
                                 const QString &seatName, QPointingDeviceUniqueId uniqueId, QObject *parent)
    : QInputDevice(*(new QPointingDevicePrivate(name, id, deviceType, pointerType, capabilities, maxPoints, buttonCount, seatName, uniqueId)), parent)
{
}

/*!
    \internal
*/
QPointingDevice::QPointingDevice(QPointingDevicePrivate &d, QObject *parent)
    : QInputDevice(d, parent)
{
}

/*!
    \internal
    \deprecated Please use the constructor rather than setters.

    Sets the device type \a devType and infers the pointer type.
*/
void QPointingDevice::setType(DeviceType devType)
{
    Q_D(QPointingDevice);
    d->deviceType = devType;
    if (d->pointerType == PointerType::Unknown)
        switch (devType) {
        case DeviceType::Mouse:
            d->pointerType = PointerType::Generic;
            break;
        case DeviceType::TouchScreen:
        case DeviceType::TouchPad:
            d->pointerType = PointerType::Finger;
            break;
        case DeviceType::Puck:
            d->pointerType = PointerType::Cursor;
            break;
        case DeviceType::Stylus:
        case DeviceType::Airbrush:
            d->pointerType = PointerType::Pen;
            break;
        default:
            break;
        }
}

/*!
    \internal
    \deprecated Please use the constructor rather than setters.
*/
void QPointingDevice::setCapabilities(QInputDevice::Capabilities caps)
{
    Q_D(QPointingDevice);
    d->capabilities = caps;
}

/*!
    \internal
    \deprecated Please use the constructor rather than setters.
*/
void QPointingDevice::setMaximumTouchPoints(int c)
{
    Q_D(QPointingDevice);
    d->maximumTouchPoints = c;
}

/*!
    Returns the pointer type.
*/
QPointingDevice::PointerType QPointingDevice::pointerType() const
{
    Q_D(const QPointingDevice);
    return d->pointerType;
}

/*!
    Returns the maximum number of simultaneous touch points (fingers) that
    can be detected.
*/
int QPointingDevice::maximumPoints() const
{
    Q_D(const QPointingDevice);
    return d->maximumTouchPoints;
}

/*!
    Returns the maximum number of on-device buttons that can be detected.
*/
int QPointingDevice::buttonCount() const
{
    Q_D(const QPointingDevice);
    return d->buttonCount;
}

/*!
    Returns a unique ID (of dubious utility) for the device.

    You probably should rather be concerned with QPointerEventPoint::uniqueId().
*/
QPointingDeviceUniqueId QPointingDevice::uniqueId() const
{
    Q_D(const QPointingDevice);
    return d->uniqueId;
}

/*!
    Returns the primary pointing device (the core pointer, traditionally
    assumed to be a mouse) on the given seat \a seatName.

    If multiple pointing devices are registered, this function prefers a mouse
    or touchpad that matches the given \a seatName and that does not have
    another device as its parent. Usually only one master or core device does
    not have a parent device. But if such a device is not found, this function
    creates a new virtual "core pointer" mouse. Thus Qt continues to work on
    platforms that are not yet doing input device discovery and registration.
*/
const QPointingDevice *QPointingDevice::primaryPointingDevice(const QString& seatName)
{
    const auto v = devices();
    const QPointingDevice *mouse = nullptr;
    const QPointingDevice *touchpad = nullptr;
    for (const QInputDevice *dev : v) {
        if (dev->seatName() != seatName)
            continue;
        if (dev->type() == QInputDevice::DeviceType::Mouse) {
            if (!mouse)
                mouse = static_cast<const QPointingDevice *>(dev);
            // the core pointer is likely a mouse, and its parent is not another input device
            if (!mouse->parent() || !qobject_cast<const QInputDevice *>(mouse->parent()))
                return mouse;
        } else if (dev->type() == QInputDevice::DeviceType::TouchPad) {
            if (!touchpad || !dev->parent() || dev->parent()->metaObject() != dev->metaObject())
                touchpad = static_cast<const QPointingDevice *>(dev);
        }
    }
    if (!mouse && !touchpad) {
        qCDebug(lcQpaInputDevices) << "no mouse-like devices registered for seat" << seatName
                                   << "The platform plugin should have provided one via "
                                      "QWindowSystemInterface::registerInputDevice(). Creating a default mouse for now.";
        mouse = new QPointingDevice(QLatin1String("core pointer"), 1, DeviceType::Mouse,
                                    PointerType::Generic, Capability::Position, 1, 3, seatName,
                                    QPointingDeviceUniqueId(), QCoreApplication::instance());
        QInputDevicePrivate::registerDevice(mouse);
        return mouse;
    }
    if (v.length() > 1)
        qCDebug(lcQpaInputDevices) << "core pointer ambiguous for seat" << seatName;
    if (mouse)
        return mouse;
    return touchpad;
}

/*!
    \internal
    Finds the device instance belonging to the drawing or eraser end of a particular stylus,
    identified by its \a deviceType, \a pointerType, \a uniqueId and \a systemId.
    Returns the device found, or \c nullptr if none was found.

    If \a systemId is \c 0, it's not significant for the search.

    If an instance matching the given \a deviceType and \a pointerType but with
    only a default-constructed \c uniqueId is found, it will be assumed to be
    the one we're looking for, and its \c uniqueId will be updated to match the
    given \a uniqueId. This is for the benefit of any platform plugin that can
    discover the tablet itself at startup, along with the supported stylus types,
    but then discovers specific styli later on as they come into proximity.
*/
const QPointingDevice *QPointingDevicePrivate::queryTabletDevice(QInputDevice::DeviceType deviceType,
                                                                 QPointingDevice::PointerType pointerType,
                                                                 QPointingDeviceUniqueId uniqueId,
                                                                 qint64 systemId)
{
    const auto &devices = QInputDevice::devices();
    for (const QInputDevice *dev : devices) {
        if (dev->type() < QPointingDevice::DeviceType::Puck || dev->type() > QPointingDevice::DeviceType::Airbrush)
            continue;
        const QPointingDevice *pdev = static_cast<const QPointingDevice *>(dev);
        const auto devPriv = QPointingDevicePrivate::get(pdev);
        bool uniqueIdDiscovered = (devPriv->uniqueId.numericId() == 0 && uniqueId.numericId() != 0);
        if (devPriv->deviceType == deviceType && devPriv->pointerType == pointerType &&
                (!systemId || devPriv->systemId == systemId) &&
                (devPriv->uniqueId == uniqueId || uniqueIdDiscovered)) {
            if (uniqueIdDiscovered) {
                const_cast<QPointingDevicePrivate *>(devPriv)->uniqueId = uniqueId;
                qCDebug(lcQpaInputDevices) << "discovered unique ID of tablet tool" << pdev;
            }
            return pdev;
        }
    }
    return nullptr;
}

/*!
    \internal
    Finds the device instance belonging to the drawing or eraser end of a particular stylus,
    identified by its \a deviceType, \a pointerType and \a uniqueId. If an existing device
    is not found, a new one is created and registered, with a warning.

    This function is called from QWindowSystemInterface. Platform plugins should use
    \l queryTabletDeviceInstance() to check whether a tablet stylus coming into proximity
    is previously known; if not known, the plugin should create and register the stylus.
*/
const QPointingDevice *QPointingDevicePrivate::tabletDevice(QInputDevice::DeviceType deviceType,
                                                            QPointingDevice::PointerType pointerType,
                                                            QPointingDeviceUniqueId uniqueId)
{
    const QPointingDevice *dev = queryTabletDevice(deviceType, pointerType, uniqueId);
    if (!dev) {
        qCDebug(lcQpaInputDevices) << "failed to find registered tablet device"
                                   << deviceType << pointerType << Qt::hex << uniqueId.numericId()
                                   << "The platform plugin should have provided one via "
                                      "QWindowSystemInterface::registerInputDevice(). Creating a default one for now.";
        dev = new QPointingDevice(QLatin1String("fake tablet"), 2, deviceType, pointerType,
                                                   QInputDevice::Capability::Position | QInputDevice::Capability::Pressure,
                                                   1, 1, QString(), uniqueId, QCoreApplication::instance());
        QInputDevicePrivate::registerDevice(dev);
    }
    return dev;
}

bool QPointingDevice::operator==(const QPointingDevice &other) const
{
    // Wacom tablets generate separate instances for each end of each stylus;
    // QInputDevice::operator==() says they are all the same, but we use
    // the stylus unique serial number and pointerType to distinguish them
    return QInputDevice::operator==(other) &&
            pointerType() == other.pointerType() &&
            uniqueId() == other.uniqueId();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QPointingDevice *device)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();
    debug << "QPointingDevice(";
    if (device) {
        debug << '"' << device->name() << "\", type=";
        QtDebugUtils::formatQEnum(debug, device->type());
        debug << ", id=" << Qt::hex << device->systemId() << Qt::dec << ", seat=" << device->seatName();
        debug << ", pointerType=";
        QtDebugUtils::formatQEnum(debug, device->pointerType());
        debug << ", capabilities=";
        QtDebugUtils::formatQFlags(debug, device->capabilities());
        debug << ", maximumTouchPoints=" << device->maximumPoints();
        if (device->uniqueId().numericId())
            debug << ", uniqueId=" << Qt::hex << device->uniqueId().numericId() << Qt::dec;
    } else {
        debug << '0';
    }
    debug << ')';
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE