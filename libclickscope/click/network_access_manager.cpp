/*
 * Copyright (C) 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "network_access_manager.h"
#include <QNetworkDiskCache>
#include <QStandardPaths>
#include <click/configuration.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <QDateTime>
#include <time.h>

click::network::Reply::Reply(QNetworkReply* reply, int id) : reply(reply)
{
    connect(this->reply.data(),
            &QNetworkReply::finished,
            this,
            &Reply::finished);

    typedef void(QNetworkReply::*QNetworkReplyErrorSignal)(QNetworkReply::NetworkError);
    connect(this->reply.data(),
            static_cast<QNetworkReplyErrorSignal>(&QNetworkReply::error),
            this,
            &Reply::error);

#ifdef NETWORK_TIMINGS
    auto url = reply->url().toString().toStdString();
    connect(this->reply.data(),
            &QNetworkReply::finished,
            [url, id, reply] () {
                bool fromCache = reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
                std::cerr << ">> " << id << " " << QDateTime::currentMSecsSinceEpoch() << " FINISHED: " << url << " cache:" << fromCache << std::endl;
            });


    connect(this->reply.data(),
            static_cast<QNetworkReplyErrorSignal>(&QNetworkReply::error),
            [url, id] () {
                std::cerr << ">> " << id << QDateTime::currentMSecsSinceEpoch() << " ERROR: " << url << std::endl;
            });
#else
    Q_UNUSED(id);
#endif
}

click::network::Reply::~Reply()
{
}

void click::network::Reply::abort()
{
    reply->abort();
}

QByteArray click::network::Reply::readAll()
{
    return reply->readAll();
}

QVariant click::network::Reply::attribute(QNetworkRequest::Attribute code)
{
    return reply->attribute(code);
}

bool click::network::Reply::hasRawHeader(const QByteArray &headerName)
{
    return reply->hasRawHeader(headerName);
}

QString click::network::Reply::rawHeader(const QByteArray &headerName)
{
    return reply->rawHeader(headerName);
}

QList<QPair<QByteArray, QByteArray>> click::network::Reply::rawHeaderPairs()
{
    return reply->rawHeaderPairs();
}

QString click::network::Reply::errorString()
{
    return reply->errorString();
}

click::network::Reply::Reply()
{
}

namespace
{

QNetworkAccessManager& networkAccessManagerInstance()
{
    static QNetworkAccessManager nam;
    if (!nam.cache())
    {
        QNetworkDiskCache* cache = new QNetworkDiskCache(&nam);
        cache->setCacheDirectory(QString("%1/unity-scope-click/network").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)));
        nam.setCache(cache);

        // FIXME: workaround for https://bugreports.qt.io/browse/QTBUG-14750 (no support for Vary header),
        // should be removed once Qt network cache implements it.
        if (click::network::AccessManager::languageChanged()) {
            qDebug() << "Language change detected, clearing network cache";
            nam.cache()->clear();
        }
    }
    return nam;
}
}

bool click::network::AccessManager::languageChanged()
{
    const QString langFilePath = QString("%1/unity-scope-click/language").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    std::string lastLanguage;
    {
        std::ifstream lastLanguageFile(langFilePath.toStdString());
        if (lastLanguageFile) {
            std::stringstream ss;
            ss << lastLanguageFile.rdbuf();
            lastLanguage = ss.str();
        }
    }

    auto const langs = Configuration().get_accept_languages();
    if (lastLanguage != langs) {
        std::ofstream lastLanguageFile(langFilePath.toStdString(), std::ios::out|std::ios::trunc);
        lastLanguageFile << langs;
        if (!lastLanguageFile) {
            qWarning() << "Failed to write language file";
        }
        return true;
    }
    return false;
}

static int request_id = static_cast<int>(time(nullptr)); // this ensures request ids will be unique even if scope is restarted

QSharedPointer<click::network::Reply> click::network::AccessManager::get(QNetworkRequest& request)
{
    int id = ++request_id;
#ifdef NETWORK_TIMINGS
    std::cerr << ">> " << id << " " << QDateTime::currentMSecsSinceEpoch() << " GET: " << request.url().toString().toStdString() << std::endl;
#endif
    return QSharedPointer<click::network::Reply>(new click::network::Reply(networkAccessManagerInstance().get(request), id));
}

QSharedPointer<click::network::Reply> click::network::AccessManager::head(QNetworkRequest& request)
{
    int id = ++request_id;
#ifdef NETWORK_TIMINGS
    std::cerr << ">> " << id << " " << QDateTime::currentMSecsSinceEpoch() << " HEAD: " << request.url().toString().toStdString() << std::endl;
#endif
    return QSharedPointer<click::network::Reply>(new click::network::Reply(networkAccessManagerInstance().head(request), id));
}

QSharedPointer<click::network::Reply> click::network::AccessManager::post(QNetworkRequest& request, QByteArray& data)
{
    int id = ++request_id;
#ifdef NETWORK_TIMINGS
    std::cerr << ">> " << id << " " << QDateTime::currentMSecsSinceEpoch() << " POST: " << request.url().toString().toStdString() << std::endl;
#endif
    return QSharedPointer<click::network::Reply>(new click::network::Reply(networkAccessManagerInstance().post(request, data), id));
}

QSharedPointer<click::network::Reply> click::network::AccessManager::sendCustomRequest(QNetworkRequest& request, QByteArray& verb, QIODevice *data)
{
    int id = ++request_id;
#ifdef NETWORK_TIMINGS
    std::cerr << ">> " << id << " " << QDateTime::currentMSecsSinceEpoch() << " CUSTOM: " << request.url().toString().toStdString() << std::endl;
#endif
    return QSharedPointer<click::network::Reply>(new click::network::Reply(networkAccessManagerInstance().sendCustomRequest(request, verb, data), id));
}

