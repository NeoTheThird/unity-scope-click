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

#include "store-query.h"
#include "store-scope.h"

#include <click/application.h>
#include <click/interface.h>
#include <click/key_file_locator.h>
#include <click/qtbridge.h>
#include <click/departments-db.h>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/Department.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/SearchMetadata.h>
#include <unity/scopes/Variant.h>
#include <unity/scopes/VariantBuilder.h>

#include <iostream>
#include <iomanip>
#include<vector>
#include<set>
#include<sstream>
#include <cassert>
#include <locale>

#include <QLocale>

#include <click/click-i18n.h>

using namespace click;

namespace
{

static const std::string CATEGORY_APPS_DISPLAY = R"(
    {
        "schema-version" : 1,
        "template" : {
            "category-layout" : "grid",
            "card-size": "small"
        },
        "components" : {
            "title" : "title",
            "subtitle": "subtitle",
            "attributes": { "field": "attributes", "max-count": 4 },
            "art" : {
                "field": "art",
                "aspect-ratio": 1.13
            }
        }
    }
)";

static const std::string CATEGORY_SCOPES_DISPLAY = R"(
    {
        "schema-version" : 1,
        "template" : {
            "overlay": true,
            "category-layout" : "grid",
            "card-size": "small"
        },
        "components" : {
            "title" : "title",
            "attributes": { "field": "attributes", "max-count": 1 },
            "art" : {
                "field": "art",
                "aspect-ratio": 0.55
            }
        }
    }
)";

static const std::string CATEGORY_APP_OF_THE_WEEK = R"(
{
    "schema-version" : 1,
    "template": {
        "category-layout": "grid",
        "card-size": "large"
    },
    "components": {
        "title": "title",
        "subtitle": "subtitle",
        "attributes": { "field": "attributes", "max-count": 4 },
        "art": {
            "aspect-ratio": 2.5,
            "field": "art"
        }
    }
})";

static const std::string CATEGORY_APPS_SEARCH = R"(
    {
        "schema-version" : 1,
        "template" : {
            "category-layout" : "grid",
            "card-layout" : "horizontal",
            "collapsed-rows": 0,
            "card-size": "large"
        },
        "components" : {
            "title" : "title",
            "art" : {
                "field": "art",
                "aspect-ratio": 1.13
            },
            "subtitle": "subtitle",
            "attributes": { "field": "attributes", "max-count": 3 }
        }
    }
)";

}

struct click::Query::Private
{
    Private(click::Index& index, click::DepartmentLookup& depts,
            std::shared_ptr<click::DepartmentsDb> depts_db,
            click::HighlightList& highlights,
            const scopes::SearchMetadata& metadata,
            pay::Package& in_package)
        : index(index),
          department_lookup(depts),
          depts_db(depts_db),
          highlights(highlights),
          meta(metadata),
          pay_package(in_package)
    {
    }
    click::Index& index;
    click::DepartmentLookup& department_lookup;
    std::shared_ptr<click::DepartmentsDb> depts_db;
    click::HighlightList& highlights;
    scopes::SearchMetadata meta;
    click::web::Cancellable search_operation;
    click::web::Cancellable purchases_operation;
    pay::Package& pay_package;
};

click::Query::Query(unity::scopes::CannedQuery const& query,
                    click::Index& index,
                    click::DepartmentLookup& depts,
                    std::shared_ptr<click::DepartmentsDb> depts_db,
                    click::HighlightList& highlights,
                    scopes::SearchMetadata const& metadata,
                    pay::Package& in_package)
    : unity::scopes::SearchQueryBase(query, metadata),
      impl(new Private(index, depts, depts_db, highlights, metadata, in_package))
{
}

click::Query::~Query()
{
    qDebug() << "destroying search";
}

void click::Query::cancelled()
{
    qDebug() << "cancelling search of" << QString::fromStdString(query().query_string());
    impl->search_operation.cancel();
}

click::Interface& click::Query::clickInterfaceInstance()
{
    static QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
    static click::Interface iface(keyFileLocator);

    return iface;
}

bool click::Query::push_result(scopes::SearchReplyProxy const& searchReply, const scopes::CategorisedResult &res)
{
    return searchReply->push(res);
}

void click::Query::finished(const scopes::SearchReplyProxy &searchReply)
{
    searchReply->finished();
}

scopes::Category::SCPtr click::Query::register_category(const scopes::SearchReplyProxy &searchReply,
                                                        const std::string &id,
                                                        const std::string &title,
                                                        const std::string &icon,
                                                        const scopes::CategoryRenderer &renderer_template)
{
    return searchReply->register_category(id, title, icon, renderer_template);
}

void click::Query::run_under_qt(const std::function<void ()> &task)
{
    qt::core::world::enter_with_task([task]() {
        task();
    });
}

//
// creates department menu with narrowed-down list of subdepartments of current department, as
// returned by server call
void click::Query::populate_departments(const click::DepartmentList& subdepts, const std::string& current_dep_id, unity::scopes::Department::SPtr &root)
{
    unity::scopes::DepartmentList departments;

    // create a list of subdepartments of current department
    foreach (auto d, subdepts)
    {
        unity::scopes::Department::SPtr department = unity::scopes::Department::create(d->id(), query(), d->name());
        if (d->has_children_flag())
        {
            department->set_has_subdepartments();
        }
        departments.push_back(department);
    }

    const std::locale loc("");
    departments.sort([&loc](const unity::scopes::Department::SCPtr &d1, const unity::scopes::Department::SCPtr &d2) -> bool {
            return loc(d1->label(), d2->label()) > 0;
            });

    if (current_dep_id != "")
    {
        auto curr_dpt = impl->department_lookup.get_department_info(current_dep_id);
        if (curr_dpt != nullptr)
        {
            unity::scopes::Department::SPtr current = unity::scopes::Department::create(current_dep_id, query(), curr_dpt->name());
            if (departments.size() > 0) // this may be a leaf department
            {
                current->set_subdepartments(departments);
            }

            auto parent_info = impl->department_lookup.get_parent(current_dep_id);
            if (parent_info != nullptr)
            {
                root = unity::scopes::Department::create(parent_info->id(), query(), parent_info->name());
                root->set_subdepartments({current});
                return;
            }
            else
            {
                root = unity::scopes::Department::create("", query(), _("All"));
                root->set_subdepartments({current});
                return;
            }
        }
        else
        {
            qWarning() << "Unknown department:" << QString::fromStdString(current_dep_id);
        }
    }

    root = unity::scopes::Department::create("", query(), _("All"));
    root->set_subdepartments(departments);
}

// recursively store all departments in the departments database
void click::Query::store_departments(const click::DepartmentList& depts)
{
    assert(impl->depts_db);

    try
    {
        impl->depts_db->store_departments(depts, search_metadata().locale());
    }
    catch (const std::exception &e)
    {
        qWarning() << "Failed to update database: " << QString::fromStdString(e.what());
    }
}

void click::Query::push_package(const scopes::SearchReplyProxy& searchReply, scopes::Category::SCPtr category, const PackageSet &installedPackages, const Package& pkg)
{
    qDebug() << "pushing result" << QString::fromStdString(pkg.name);
    try {
        scopes::CategorisedResult res(category);
        res.set_title(pkg.title);
        res.set_art(pkg.icon_url);
        res.set_uri(pkg.url);
        res[click::Query::ResultKeys::NAME] = pkg.name;
        res["subtitle"] = pkg.publisher;
        auto installed = installedPackages.find(pkg);

        std::string price = _("FREE");
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << "☆ " << pkg.rating;
        std::string rating{ss.str()};

        bool purchased = false;
        double cur_price{0.00};
        auto suggested = impl->index.get_suggested_currency();
        std::string currency = Configuration::get_currency(suggested);
        if (pkg.prices.count(currency) == 1) {
            cur_price = pkg.prices.at(currency);
        } else {
            // NOTE: This is decprecated. Here for compatibility.
            currency = Configuration::CURRENCY_DEFAULT;
            cur_price = pkg.price;
        }
        res["price"] = scopes::Variant(cur_price);

        if (cur_price > 0.00f) {
            if (!Configuration::get_purchases_enabled()) {
                // Don't show priced apps if flag not set
                return;
            }
            // Check if the priced app was already purchased.
            purchased = std::find(purchased_apps.begin(), purchased_apps.end(), pkg.name) != purchased_apps.end();
        }
        if (installed != installedPackages.end()) {
            res[click::Query::ResultKeys::INSTALLED] = true;
            res[click::Query::ResultKeys::PURCHASED] = purchased;
            price = _("✔ INSTALLED");
            res[click::Query::ResultKeys::VERSION] = installed->version;
        } else if (purchased) {
            res[click::Query::ResultKeys::PURCHASED] = true;
            res[click::Query::ResultKeys::INSTALLED] = false;
            price = _("✔ PURCHASED");
        } else {
            res[click::Query::ResultKeys::INSTALLED] = false;
            res[click::Query::ResultKeys::PURCHASED] = false;
            if (cur_price > 0.00f) {
                QLocale locale;
                auto symbol = Configuration::CURRENCY_MAP.at(currency);
                price = locale.toCurrencyString(cur_price,
                                                symbol.c_str()).toUtf8().data();
                res["currency_symbol"] = symbol;
            }
        }

        // Add the price and rating as attributes.
        scopes::VariantBuilder builder;
        builder.add_tuple({
                {"value", scopes::Variant(price)},
            });
        builder.add_tuple({
                {"value", scopes::Variant("")},
            });
        builder.add_tuple({
                {"value", scopes::Variant(rating)},
            });
        builder.add_tuple({
                {"value", scopes::Variant("")},
            });
        res["attributes"] = builder.end();

        this->push_result(searchReply, res);
    } catch(const std::exception& e){
        qCritical() << "push_package: Exception: " << e.what() ;
    } catch(...){
        qDebug() << "no reason to catch";
    }
}

void click::Query::push_highlights(const scopes::SearchReplyProxy& searchReply, const HighlightList& highlights, const PackageSet &locallyInstalledApps)
{
    const scopes::CategoryRenderer renderer(CATEGORY_APPS_DISPLAY);
    const scopes::CategoryRenderer scopes_renderer(CATEGORY_SCOPES_DISPLAY);
    const scopes::CategoryRenderer aotw_renderer(CATEGORY_APP_OF_THE_WEEK);

    for (auto const& hl: highlights)
    {
        scopes::CategoryRenderer const* rdr = &renderer;
        if (hl.slug() == "app-of-the-week" || hl.packages().size() == 1)
        {
            rdr = &aotw_renderer;
        }
        if (hl.contains_scopes())
        {
            rdr = &scopes_renderer;
        }
        auto category = register_category(searchReply, hl.slug(), hl.name(), "", *rdr);
        for (auto const& pkg: hl.packages())
        {
            push_package(searchReply, category, locallyInstalledApps, pkg);
        }
    }
    qDebug() << "Highlights pushed";
}

void click::Query::push_departments(const scopes::SearchReplyProxy& searchReply, const scopes::Department::SCPtr& root)
{
    if (root != nullptr)
    {
        try
        {
            qDebug() << "pushing departments";
            searchReply->register_departments(root);
        }
        catch (const std::exception& e)
        {
            qWarning() << "Failed to register departments for query " << QString::fromStdString(query().query_string()) <<
                ", current department " << QString::fromStdString(query().department_id()) << ": " << e.what();
        }
    }
    else
    {
        qWarning() << "No departments data for query " << QString::fromStdString(query().query_string()) <<
            "', current department " << QString::fromStdString(query().department_id());
    }
}

//
// push highlights and departments
// use cached highlights for root department, otherwise run an async job for highlights of current department.
void click::Query::add_highlights(scopes::SearchReplyProxy const& searchReply, const PackageSet& locallyInstalledApps)
{
    auto curdep = impl->department_lookup.get_department_info(query().department_id());
    if (!curdep)
    {
        qWarning() << "No department information for current department" << QString::fromStdString(query().department_id());
        return;
    }

    if (query().department_id() == "") // top-level departments
    {
        unity::scopes::Department::SPtr root;
        auto subdepts = curdep->sub_departments();
        populate_departments(subdepts, query().department_id(), root);
        push_departments(searchReply, root);

        qDebug() << "pushing cached highlights";
        push_highlights(searchReply, impl->highlights, locallyInstalledApps);
        this->finished(searchReply); //FIXME: this shouldn't be needed
    }
    else
    {
        qDebug() << "starting departments call for department" << QString::fromStdString(curdep->id()) << ", href" << QString::fromStdString(curdep->href());
        impl->search_operation = impl->index.departments(curdep->href(), [this, locallyInstalledApps, searchReply](const DepartmentList& depts,
                    const HighlightList& highlights, Index::Error error, int)
                {
                    if (error == click::Index::Error::NoError)
                    {
                        qDebug() << "departments call completed";
                        unity::scopes::Department::SPtr root;
                        populate_departments(depts, query().department_id(), root);
                        push_departments(searchReply, root);
                        push_highlights(searchReply, highlights, locallyInstalledApps);
                    }
                    else
                    {
                        qWarning() << "departments call failed";
                    }
                    this->finished(searchReply); //FIXME: this shouldn't be needed
                });
        }
}

void click::Query::add_available_apps(scopes::SearchReplyProxy const& searchReply,
                                      const PackageSet& installedPackages,
                                      const std::string& categoryTemplate)
{
    // this assertion is here to ensure unit tests are properly implemented.
    // this pointer is never null during normal execution.
    assert(searchReply);

    run_under_qt([=]()
    {
        auto search_cb = [this, searchReply, categoryTemplate, installedPackages](Packages packages, Packages recommends) {
                qDebug("search callback");

                const scopes::CategoryRenderer categoryRenderer(categoryTemplate);

                std::string cat_title(_("Available"));
                {
                    char tmp[512];
                    unsigned int num_results = static_cast<unsigned int>(packages.size());
                    if (snprintf(tmp, sizeof(tmp),
                                 dngettext(GETTEXT_PACKAGE,
                                           "%u result in Ubuntu Store",
                                           "%u results in Ubuntu Store",
                                           num_results), num_results) > 0) {
                        cat_title = tmp;
                    }
                }
                auto category = register_category(searchReply, "appstore", cat_title, "", categoryRenderer);

                const scopes::CategoryRenderer recommendsCatRenderer(categoryTemplate);
                auto recommendsCategory = register_category(searchReply, "recommends",
                                                _("Recommended"), "",
                                                recommendsCatRenderer);

                // handle packages data
                foreach (auto p, packages) {
                    push_package(searchReply, category, installedPackages, p);
                }
                foreach (auto r, recommends) {
                    push_package(searchReply, recommendsCategory,
                                 installedPackages, r);
                }
                qDebug() << "search completed";
                this->finished(searchReply); //FIXME: this shouldn't be needed
            };

            // this is the case when we do bootstrap for the first time, or it failed last time
            if (impl->department_lookup.size() == 0)
            {
                qDebug() << "performing bootstrap request";
                impl->search_operation = impl->index.bootstrap([this, search_cb, searchReply, installedPackages](const DepartmentList& deps, const
                            HighlightList& highlights, click::Index::Error error, int) {
                if (error == click::Index::Error::NoError)
                {
                    qDebug() << "bootstrap request completed";
                    auto root = std::make_shared<click::Department>("", _("All"), "", true);
                    root->set_subdepartments(deps);
                    DepartmentList rdeps { root };
                    impl->department_lookup.rebuild(rdeps);
                    impl->highlights = highlights;
                    qDebug() << "Total number of departments:" << impl->department_lookup.size() << ", highlights:" << highlights.size();

                    if (impl->depts_db)
                    {
                        qDebug() << "Storing departments in the database";
                        store_departments(deps);
                    }
                    else
                    {
                        qWarning() << "Departments db not available";
                    }
                }
                else
                {
                    qWarning() << "bootstrap request failed";
                }

                if (error == click::Index::Error::NoError)
                {
                    if (query().query_string().empty())
                    {
                        add_highlights(searchReply, installedPackages);
                    }
                    else
                    {
                        qDebug() << "starting search of" << QString::fromStdString(query().query_string());
                        impl->search_operation = impl->index.search(query().query_string(), search_cb);
                    }
                }
            });
        }
        else
        {
            if (query().query_string().empty())
            {
                add_highlights(searchReply, installedPackages);
            }
            else // normal search
            {
                qDebug() << "starting search of" << QString::fromStdString(query().query_string());
                impl->search_operation = impl->index.search(query().query_string(), search_cb);
            }
        }
    });
}

PackageSet click::Query::get_installed_packages()
{
    std::promise<PackageSet> installed_promise;
    std::future<PackageSet> installed_future = installed_promise.get_future();

    run_under_qt([&]()
    {
        clickInterfaceInstance().get_installed_packages(
                    [&installed_promise](PackageSet installedPackages, InterfaceError){
            installed_promise.set_value(installedPackages);
        });
    });

    return installed_future.get();
}


void click::Query::run(scopes::SearchReplyProxy const& searchReply)
{
    auto q = query().query_string();
    std::string categoryTemplate = CATEGORY_APPS_SEARCH;
    if (q.empty()) {
        categoryTemplate = CATEGORY_APPS_DISPLAY;
    }
    if (Configuration::get_purchases_enabled()) {
        std::promise<pay::PurchasedList> purchased_promise;
        std::future<pay::PurchasedList> purchased_future = purchased_promise.get_future();
        qDebug() << "Getting list of purchased apps.";
        run_under_qt([this, &purchased_promise]() {
                impl->purchases_operation = impl->pay_package.get_purchases([&purchased_promise](const pay::PurchasedList& purchases) {
                        purchased_promise.set_value(purchases);
                    });
            });
        purchased_apps = purchased_future.get();
    }

    add_available_apps(searchReply, get_installed_packages(), categoryTemplate);
}
