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

#include <gtest/gtest.h>
#include "fake_json.h"
#include <json/reader.h>
#include <json/value.h>
#include <click/highlights.h>
#include <click/departments.h>

TEST(BootstrapTest, testParsing)
{
    Json::Reader reader;
    Json::Value root;

    EXPECT_TRUE(reader.parse(FAKE_JSON_BOOTSTRAP, root));

    {
        auto highlights = click::Highlight::from_json_root_node(root);
        EXPECT_EQ(3u, highlights.size());
        auto it = highlights.begin();
        EXPECT_EQ("Top Apps", it->name());
        EXPECT_EQ(2u, it->packages().size());
        ++it;
        EXPECT_EQ("Most Purchased", it->name());
        EXPECT_EQ(2u, it->packages().size());
        ++it;
        EXPECT_EQ("New Releases", it->name());
        EXPECT_EQ(2u, it->packages().size());
    }
    {
        auto depts = click::Department::from_json_root_node(root);
        EXPECT_EQ(1u, depts.size());
        auto it = depts.begin();
        EXPECT_EQ("Fake Subdepartment", (*it)->name());
        EXPECT_FALSE((*it)->has_children_flag());
        EXPECT_EQ("https://search.apps.staging.ubuntu.com/api/v1/departments/fake-subdepartment", (*it)->href());
    }
}

TEST(BootstrapTest, testParsingErrors)
{
    Json::Reader reader;
    Json::Value root;

    EXPECT_TRUE(reader.parse(FAKE_JSON_BROKEN_BOOTSTRAP, root));

    {
        auto highlights = click::Highlight::from_json_root_node(root);
        EXPECT_EQ(1u, highlights.size());
        auto it = highlights.begin();
        EXPECT_EQ("Top Apps", it->name());
        EXPECT_EQ(1u, it->packages().size());
    }
    {
        auto depts = click::Department::from_json_root_node(root);
        EXPECT_EQ(0u, depts.size());
    }
}
