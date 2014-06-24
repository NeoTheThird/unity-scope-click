/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp launcher.xml -p launcher -c Launcher -N
 *
 * qdbusxml2cpp is Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef LAUNCHER_H_1403062747
#define LAUNCHER_H_1403062747

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface com.ubuntu.unity.launcher.Installations
 */
class Launcher: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "com.ubuntu.unity.launcher.Installations"; }

public:
    Launcher(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~Launcher();

public Q_SLOTS: // METHODS
    inline Q_NOREPLY void completeInstallation(const QString &package_name, const QString &app_id)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(package_name) << QVariant::fromValue(app_id);
        callWithArgumentList(QDBus::NoBlock, QLatin1String("completeInstallation"), argumentList);
    }

    inline Q_NOREPLY void startInstallation(const QString &title, const QString &icon_url, const QString &package_name)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(title) << QVariant::fromValue(icon_url) << QVariant::fromValue(package_name);
        callWithArgumentList(QDBus::NoBlock, QLatin1String("startInstallation"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

#endif
