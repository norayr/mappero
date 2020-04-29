/*
 * Copyright (C) 2019-2020 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This file is part of Mappero.
 *
 * Mappero is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mappero is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Photokinesis.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "updater.h"

#include <QCoreApplication>
#include <QDebug>
#include <QLocale>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QTest>
#include <QUrlQuery>
#include "fake_network.h"

using namespace Mardy;

class UpdaterTest: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testStart_data();
    void testStart();
    void testUserAgent();
};

void UpdaterTest::testStart_data()
{
    QTest::addColumn<QString>("ourVersion");
    QTest::addColumn<QString>("serverReply");
    QTest::addColumn<Updater::Status>("expectedStatus");
    QTest::addColumn<QJsonObject>("expectedVersion");

    QTest::newRow("invalid json") <<
        "0.1.0" <<
        "You bet\n" <<
        Updater::Failed <<
        QJsonObject {};

    QTest::newRow("older") <<
        "0.4.0" <<
        R"({"windows":"0.3.1","linux":"0.3.1","osx":"0.3.1"})" <<
        Updater::Succeeded <<
        QJsonObject {
            { "version", "0.3.1" },
            { "isNewer", false },
        };

    QTest::newRow("same") <<
        "0.9.2" <<
        R"({"windows":"0.9.2","linux":"0.9.2","osx":"0.9.2"})" <<
        Updater::Succeeded <<
        QJsonObject {
            { "version", "0.9.2" },
            { "isNewer", false },
        };

    QTest::newRow("newer") <<
        "0.9.2" <<
        R"({"windows":"0.10.0","linux":"0.10.0","osx":"0.10.0"})" <<
        Updater::Succeeded <<
        QJsonObject {
            { "version", "0.10.0" },
            { "isNewer", true },
        };
}

void UpdaterTest::testStart()
{
    QFETCH(QString, ourVersion);
    QFETCH(QString, serverReply);
    QFETCH(Updater::Status, expectedStatus);
    QFETCH(QJsonObject, expectedVersion);

    QCoreApplication::setApplicationVersion(ourVersion);
    FakeNamFactory *namFactory = new FakeNamFactory();
    QSignalSpy requestCreated(namFactory, &FakeNamFactory::requestCreated);

    QQmlEngine engine;
    engine.setNetworkAccessManagerFactory(namFactory);
    qmlRegisterType<Updater>("MyTest", 1, 0, "Updater");
    QQmlComponent component(&engine);
    component.setData("import MyTest 1.0\n"
                      "Updater {\n"
                      "}",
                      QUrl());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object);

    Updater *updater = qobject_cast<Updater*>(object.data());
    QVERIFY(updater != 0);
    QSignalSpy finished(updater, &Updater::finished);

    QCOMPARE(updater->status(), Updater::Idle);

    updater->start();
    QCOMPARE(updater->status(), Updater::Busy);

    QTRY_COMPARE(requestCreated.count(), 1);
    FakeReply *reply = requestCreated.at(0).at(0).value<FakeReply*>();
    QVERIFY(reply);

    reply->setData(serverReply.toUtf8());
    QVERIFY(finished.wait());
    QCOMPARE(finished.count(), 1);

    QCOMPARE(updater->latestVersion(), expectedVersion);
    QCOMPARE(updater->status(), expectedStatus);
}

void UpdaterTest::testUserAgent()
{
    QString appName("SuperApp");
    QString appVersion("0.2");
    QString locale("it_IT");

    QCoreApplication::setApplicationName(appName);
    QCoreApplication::setApplicationVersion(appVersion);
    QLocale::setDefault(QLocale(locale));

    QString expectedUserAgent = QString("SuperApp/0.2 (%1/%2 %3; it_IT)")
        .arg(QSysInfo::productType())
        .arg(QSysInfo::productVersion())
        .arg(QSysInfo::currentCpuArchitecture());

    FakeNamFactory *namFactory = new FakeNamFactory();
    QSignalSpy requestCreated(namFactory, &FakeNamFactory::requestCreated);

    QQmlEngine engine;
    engine.setNetworkAccessManagerFactory(namFactory);
    qmlRegisterType<Updater>("MyTest", 1, 0, "Updater");
    QQmlComponent component(&engine);
    component.setData("import MyTest 1.0\n"
                      "Updater {\n"
                      "}",
                      QUrl());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object);

    Updater *updater = qobject_cast<Updater*>(object.data());
    QVERIFY(updater != 0);
    QSignalSpy finished(updater, &Updater::finished);
    updater->start();

    QTRY_COMPARE(requestCreated.count(), 1);
    FakeReply *reply = requestCreated.at(0).at(0).value<FakeReply*>();
    QVERIFY(reply);

    QNetworkRequest req = reply->request();
    QCOMPARE(req.header(QNetworkRequest::UserAgentHeader).toString(),
             expectedUserAgent);

    // cleanup
    reply->setData("");
    QVERIFY(finished.wait());
}

QTEST_GUILESS_MAIN(UpdaterTest)

#include "tst_updater.moc"
