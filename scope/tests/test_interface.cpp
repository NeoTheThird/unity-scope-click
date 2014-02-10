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

#include "test_data.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QTimer>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <click/interface.h>
#include <click/key_file_locator.h>

using namespace click;

namespace
{
const QString emptyQuery{};

struct MockKeyFileLocator : public click::KeyFileLocator
{
    typedef click::KeyFileLocator Super;

    MockKeyFileLocator()
    {
        using namespace ::testing;

        ON_CALL(*this, enumerateKeyFilesForInstalledApplications(_))
                .WillByDefault(
                    Invoke(
                        this,
                        &MockKeyFileLocator::doEnumerateKeyFilesForInstalledApplications));
    }

    MOCK_METHOD1(enumerateKeyFilesForInstalledApplications,
                 void(const Super::Enumerator&));

    void doEnumerateKeyFilesForInstalledApplications(const Super::Enumerator& enumerator)
    {
        Super::enumerateKeyFilesForInstalledApplications(enumerator);
    }
};
}

TEST(ClickInterface, testIsNonClickAppFalse)
{
    EXPECT_FALSE(Interface::is_non_click_app("unknown-app.desktop"));
}

TEST(ClickInterface, testIsNonClickAppNoRegression)
{
    // Loop through and check that all filenames are non-click filenames
    // If this ever breaks, something is very very wrong.
    for (const auto& element : nonClickDesktopFiles())
    {
        QString filename = element.c_str();
        EXPECT_TRUE(Interface::is_non_click_app(filename));
    }
}

TEST(ClickInterface, callsIntoKeyFileLocatorForFindingInstalledApps)
{
    using namespace ::testing;
    MockKeyFileLocator mockKeyFileLocator;
    QSharedPointer<click::KeyFileLocator> keyFileLocator(
                &mockKeyFileLocator,
                [](click::KeyFileLocator*){});

    click::Interface iface(keyFileLocator);

    EXPECT_CALL(mockKeyFileLocator, enumerateKeyFilesForInstalledApplications(_)).Times(1);

    iface.find_installed_apps(emptyQuery);
}

TEST(ClickInterface, testFindAppsInDirEmpty)
{
    std::list<Application> results;

    Interface::find_apps_in_dir(QDir::currentPath(), "foo", results);
    EXPECT_TRUE(results.empty());
}

// TODO: Get rid of file-based testing and instead make unity::util::IniParser mockable
// Maintaining this list here will become tedious over time.
TEST(ClickInterface, findInstalledAppsReturnsCorrectListOfResults)
{
    std::list<click::Application> expectedResult =
    {
        {"","Ubuntu One", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.developer.webapps.webapp-ubuntuone/./ubuntuone.png", "application:///com.ubuntu.developer.webapps.webapp-ubuntuone_webapp-ubuntuone_1.0.4.desktop"},
        {"","Stock Ticker", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.stock-ticker-mobile/icons/stock_icon_48.png", "application:///com.ubuntu.stock-ticker-mobile_stock-ticker-mobile_0.3.7.66.desktop"},
        {"","Weather", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.weather/./weather64.png", "application:///com.ubuntu.weather_weather_1.0.168.desktop"},
        {"","Twitter", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.developer.webapps.webapp-twitter/./twitter.png", "application:///com.ubuntu.developer.webapps.webapp-twitter_webapp-twitter_1.0.5.desktop"},
        {"","Music", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.music/images/music.png", "application:///com.ubuntu.music_music_1.1.329.desktop"},
        {"","Clock", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.clock/./clock64.png", "application:///com.ubuntu.clock_clock_1.0.300.desktop"},
        {"","Dropping Letters", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.dropping-letters/dropping-letters.png", "application:///com.ubuntu.dropping-letters_dropping-letters_0.1.2.2.43.desktop"},
        {"","Gmail", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.developer.webapps.webapp-gmail/./gmail.png", "application:///com.ubuntu.developer.webapps.webapp-gmail_webapp-gmail_1.0.8.desktop"},
        {"","Terminal", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.terminal/./terminal64.png", "application:///com.ubuntu.terminal_terminal_0.5.29.desktop"},
        {"","Calendar", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.calendar/./calendar64.png", "application:///com.ubuntu.calendar_calendar_0.4.182.desktop"},
        {"","Notes", "","notepad", "application:///com.ubuntu.notes_notes_1.4.242.desktop"},
        {"","Amazon", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.developer.webapps.webapp-amazon/./amazon.png", "application:///com.ubuntu.developer.webapps.webapp-amazon_webapp-amazon_1.0.6.desktop"},
        {"","Shorts", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.shorts/./rssreader64.png", "application:///com.ubuntu.shorts_shorts_0.2.162.desktop"},
        {"","File Manager", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/./filemanager64.png", "application:///com.ubuntu.filemanager_filemanager_0.1.1.97.desktop"},
        {"","Calculator", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.calculator/./calculator64.png", "application:///com.ubuntu.calculator_calculator_0.1.3.206.desktop"},
        {"","Sudoku", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.sudoku/SudokuGameIcon.png", "application:///com.ubuntu.sudoku_sudoku_1.0.142.desktop"},
        {"","eBay", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.developer.webapps.webapp-ebay/./ebay.png", "application:///com.ubuntu.developer.webapps.webapp-ebay_webapp-ebay_1.0.8.desktop"},
        {"","Facebook", "","/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.developer.webapps.webapp-facebook/./facebook.png", "application:///com.ubuntu.developer.webapps.webapp-facebook_webapp-facebook_1.0.5.desktop"},
        {"","Messaging", "","messages-app", "application:///messaging-app.desktop"},
        {"","Contacts", "","contacts-app", "application:///address-book-app.desktop"}
    };

    click::Application mustNotBePartOfResult
    {
        "",
        "NonClickAppWithoutException",
        "",
        "NonClickAppWithoutException",
        "application:///non-click-app-without-exception.desktop"
    };

    QSharedPointer<click::KeyFileLocator> keyFileLocator(
                new click::KeyFileLocator(
                    testing::systemApplicationsDirectoryForTesting(),
                    testing::userApplicationsDirectoryForTesting()));

    click::Interface iface(keyFileLocator);

    auto result = iface.find_installed_apps(emptyQuery);

    EXPECT_TRUE(result.size() > 0);

    for (const auto& app : expectedResult)
    {
        EXPECT_NE(result.end(), std::find(result.begin(), result.end(), app));
    }

    EXPECT_EQ(result.end(), std::find(result.begin(), result.end(), mustNotBePartOfResult));
}