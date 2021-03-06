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

#include "department-lookup.h"

namespace click
{

DepartmentLookup::DepartmentLookup()
{
}

void DepartmentLookup::rebuild(const Department::SPtr& dept)
{
    departments[dept->id()] = dept;
    for (auto const& subdep: dept->sub_departments())
    {
        parent_lut[subdep->id()] = dept;
        rebuild(subdep);
    }
}

void DepartmentLookup::rebuild(const std::list<Department::SPtr>& root_departments)
{
    parent_lut.clear();
    departments.clear();
    for (auto const& dep: root_departments)
    {
        rebuild(dep);
    }
}

Department::SPtr DepartmentLookup::get_parent(const std::string& department_id) const
{
    auto it = parent_lut.find(department_id);
    if (it != parent_lut.end())
    {
        return it->second;
    }
    return Department::SPtr(nullptr);
}

Department::SPtr DepartmentLookup::get_department_info(const std::string& department_id) const
{
    auto it = departments.find(department_id);
    if (it != departments.end())
    {
        return it->second;
    }
    return nullptr;
}

int DepartmentLookup::size() const
{
    return parent_lut.size();
}

}
