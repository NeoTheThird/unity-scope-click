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

#ifndef CLICKPREVIEW_H
#define CLICKPREVIEW_H

#include <click/index.h>
#include <click/download-manager.h>
#include <click/pay.h>
#include <click/qtbridge.h>
#include "reviews.h"

#include <click/network_access_manager.h>

#include <unity/scopes/ActionMetadata.h>
#include <unity/scopes/OnlineAccountClient.h>
#include <unity/scopes/PreviewQueryBase.h>
#include <unity/scopes/PreviewWidget.h>
#include <unity/scopes/Result.h>
#include <unity/scopes/ScopeBase.h>
#include <unity/util/DefinesPtrs.h>
#include <set>

namespace scopes = unity::scopes;

namespace click {

class Manifest;
class PreviewStrategy;
class DepartmentsDb;

struct WidgetsInColumns
{
    struct {
        std::vector<std::string> column1;
    } singleColumn;
    struct {
        std::vector<std::string> column1;
        std::vector<std::string> column2;
    } twoColumns;

    void registerLayouts(unity::scopes::PreviewReplyProxy const& reply);
    void appendToColumn(std::vector<std::string>& column, unity::scopes::PreviewWidgetList const& widgets);
};

class CachedPreviewWidgets
{
public:
    void push(unity::scopes::PreviewWidget const &widget);
    void push(unity::scopes::PreviewWidgetList const &widgetList);
    void flush(unity::scopes::PreviewReplyProxy const& reply);
    bool has(std::string const& widget) const;

    WidgetsInColumns layout;

private:
    std::list<unity::scopes::PreviewWidget> widgets;
    std::unordered_set<std::string> widgets_lookup;
};

class DepartmentUpdater
{
protected:
    DepartmentUpdater() = default;
    DepartmentUpdater(const std::shared_ptr<click::DepartmentsDb>& depts);
    virtual ~DepartmentUpdater() = default;
    void store_department(const PackageDetails& pkg);

private:
    std::shared_ptr<click::DepartmentsDb> depts;
};

class Preview : public unity::scopes::PreviewQueryBase
{
protected:
    std::unique_ptr<PreviewStrategy> strategy;
    const unity::scopes::Result& result;
    const unity::scopes::ActionMetadata& metadata;
    std::shared_future<void> qt_ready_;
    PreviewStrategy* build_strategy(const unity::scopes::Result& result,
                                    const unity::scopes::ActionMetadata& metadata,
                                    const QSharedPointer<web::Client> &client,
                                    const QSharedPointer<pay::Package>& ppackage,
                                    const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                                    std::shared_ptr<click::DepartmentsDb> depts);
    virtual PreviewStrategy* build_installing(const std::string& download_url,
                                              const std::string& download_sha512,
                                              const unity::scopes::Result& result,
                                              const QSharedPointer<click::web::Client>& client,
                                              const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                                              std::shared_ptr<click::DepartmentsDb> depts);
public:
    UNITY_DEFINES_PTRS(Preview);
    struct Actions
    {
        Actions() = delete;

        constexpr static const char* INSTALL_CLICK{"install_click"};
        constexpr static const char* BUY_CLICK{"buy_click"};
        constexpr static const char* DOWNLOAD_COMPLETED{"finished"};
        constexpr static const char* DOWNLOAD_FAILED{"failed"};
        constexpr static const char* PURCHASE_SUCCEEDED{"purchase_succeeded"};
        constexpr static const char* PURCHASE_FAILED{"purchase_failed"};
        constexpr static const char* OPEN_CLICK{"open_click"};
        constexpr static const char* PIN_TO_LAUNCHER{"pin_to_launcher"};
        constexpr static const char* UNINSTALL_CLICK{"uninstall_click"};
        constexpr static const char* CONFIRM_UNINSTALL{"confirm_uninstall"};
        constexpr static const char* CANCEL_PURCHASE_UNINSTALLED{"cancel_purchase_uninstalled"};
        constexpr static const char* CANCEL_PURCHASE_INSTALLED{"cancel_purchase_installed"};
        constexpr static const char* SHOW_UNINSTALLED{"show_uninstalled"};
        constexpr static const char* SHOW_INSTALLED{"show_installed"};
        constexpr static const char* CONFIRM_CANCEL_PURCHASE_UNINSTALLED{"confirm_cancel_purchase_uninstalled"};
        constexpr static const char* CONFIRM_CANCEL_PURCHASE_INSTALLED{"confirm_cancel_purchase_installed"};
        constexpr static const char* OPEN_ACCOUNTS{"open_accounts"};
        constexpr static const char* RATED{"rated"};
    };

    Preview(const unity::scopes::Result& result);
    Preview(const unity::scopes::Result& result,
            const unity::scopes::ActionMetadata& metadata,
            std::shared_future<void> const& qt_ready = std::future<void>());
    virtual ~Preview();
    void choose_strategy(const QSharedPointer<web::Client> &client,
                         const QSharedPointer<pay::Package>& ppackage,
                         const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                         std::shared_ptr<click::DepartmentsDb> depts);
    // From unity::scopes::PreviewQuery
    void cancelled() override;
    virtual void run(unity::scopes::PreviewReplyProxy const& reply) override;
};

class PreviewStrategy
{
public:

    PreviewStrategy(const unity::scopes::Result& result);
    PreviewStrategy(const unity::scopes::Result& result,
                    const QSharedPointer<click::web::Client>& client);
    PreviewStrategy(const unity::scopes::Result& result,
                    const QSharedPointer<click::web::Client>& client,
                    const QSharedPointer<pay::Package>& pay_package);
    virtual ~PreviewStrategy();

    virtual void cancelled();
    virtual void run(unity::scopes::PreviewReplyProxy const& reply) = 0;
    virtual void pushPackagePreviewWidgets(CachedPreviewWidgets &reply,
                                           const PackageDetails& details,
                                           const scopes::PreviewWidgetList& button_area_widgets);

    virtual void run_under_qt(const std::function<void ()> &task);
    virtual void invalidateScope(const std::string& scope_id);

protected:
    virtual void populateDetails(std::function<void(const PackageDetails &)> details_callback,
                                 std::function<void(const click::ReviewList&,
                                                    click::Reviews::Error)> reviews_callback,
                                 bool force_cache = false);
    virtual scopes::PreviewWidgetList headerWidgets(const PackageDetails &details);
    virtual scopes::PreviewWidgetList screenshotsWidgets(const PackageDetails &details);
    virtual scopes::PreviewWidgetList descriptionWidgets(const PackageDetails &details);
    virtual scopes::PreviewWidgetList progressBarWidget(const std::string& object_path);
    virtual scopes::PreviewWidgetList reviewsWidgets(const click::ReviewList &reviewlist);
    virtual scopes::PreviewWidgetList downloadErrorWidgets();
    virtual scopes::PreviewWidgetList loginErrorWidgets(const std::string& download_url, const std::string& download_sha512);
    virtual scopes::PreviewWidgetList errorWidgets(const scopes::Variant& title,
                                                   const scopes::Variant& summary,
                                                   const scopes::Variant& action_id,
                                                   const scopes::Variant& action_label,
                                                   const scopes::Variant& action_uri = scopes::Variant::null());
    virtual void pushPackagePreviewWidgets(const unity::scopes::PreviewReplyProxy &reply,
                                           const PackageDetails& details,
                                           const scopes::PreviewWidgetList& button_area_widgets);
    virtual scopes::PreviewWidget build_other_metadata(const PackageDetails& details);
    virtual scopes::PreviewWidget build_updates_table(const PackageDetails& details);
    virtual std::string build_whats_new(const PackageDetails& details);
    virtual bool isRefundable();

    scopes::Result result;

    QSharedPointer<click::web::Client> client;
    QSharedPointer<click::Index> index;
    click::web::Cancellable index_operation;
    QSharedPointer<click::Reviews> reviews;
    click::web::Cancellable reviews_operation;
    click::web::Cancellable submit_operation;
    scopes::OnlineAccountClient oa_client;
    QSharedPointer<pay::Package> pay_package;
    click::web::Cancellable purchase_operation;
};

class DownloadErrorPreview : public PreviewStrategy
{
public:
    DownloadErrorPreview(const unity::scopes::Result& result);

    virtual ~DownloadErrorPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;
};

class InstallingPreview : public PreviewStrategy, public DepartmentUpdater
{
public:
    InstallingPreview(const unity::scopes::Result& result) : PreviewStrategy(result) {}
    InstallingPreview(const std::string& download_url,
                      const std::string& download_sha512,
                      const unity::scopes::Result& result,
                      const QSharedPointer<click::web::Client>& client,
                      const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                      std::shared_ptr<click::DepartmentsDb> depts);

    virtual ~InstallingPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

protected:
    std::string download_url;
    std::string download_sha512;
    QSharedPointer<click::DownloadManager> dm;
    std::shared_ptr<click::DepartmentsDb> depts_db;
    CachedPreviewWidgets cachedWidgets;
    void startLauncherAnimation(const PackageDetails& details);
};

class InstalledPreview : public PreviewStrategy, public DepartmentUpdater
{
public:
    InstalledPreview(const unity::scopes::Result& result,
                     const unity::scopes::ActionMetadata& metadata,
                     const QSharedPointer<click::web::Client>& client,
                     const QSharedPointer<pay::Package>& ppackage,
                     const std::shared_ptr<click::DepartmentsDb>& depts);

    virtual ~InstalledPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

    std::string getApplicationUri(const Manifest& manifest);
    std::string get_consumer_key();
    scopes::PreviewWidgetList createButtons(const click::Manifest& manifest);
    scopes::PreviewWidget createRatingWidget(const click::Review& review) const;

private:
    scopes::ActionMetadata metadata;
    CachedPreviewWidgets cachedWidgets;
    PackageDetails cachedDetails;
};

class InstalledScopePreview : public PreviewStrategy
{
public:
    InstalledScopePreview(const unity::scopes::Result& result);
    void run(unity::scopes::PreviewReplyProxy const& reply) override;
};

class PurchasingPreview : public PreviewStrategy
{
public:
    PurchasingPreview(const unity::scopes::Result& result,
                      const QSharedPointer<click::web::Client>& client);
    virtual ~PurchasingPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

protected:
    virtual scopes::PreviewWidgetList purchasingWidgets(const PackageDetails &);
};

class CancelPurchasePreview : public PreviewStrategy
{
public:
    CancelPurchasePreview(const unity::scopes::Result& result, bool installed);

    virtual ~CancelPurchasePreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

protected:
    scopes::PreviewWidgetList build_widgets();
    bool installed;
};

class UninstallConfirmationPreview : public PreviewStrategy
{
public:
    UninstallConfirmationPreview(const unity::scopes::Result& result);

    virtual ~UninstallConfirmationPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;
};

class UninstalledPreview : public PreviewStrategy, public DepartmentUpdater
{
public:
    UninstalledPreview(const unity::scopes::Result& result,
                       const unity::scopes::ActionMetadata& metadata,
                       const QSharedPointer<click::web::Client>& client,
                       const std::shared_ptr<click::DepartmentsDb>& depts,
                       const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                       const QSharedPointer<pay::Package>& ppackage);

    virtual ~UninstalledPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;
    virtual scopes::PreviewWidgetList uninstalledActionButtonWidgets(const PackageDetails &details);

protected:
    scopes::ActionMetadata metadata;
    PackageDetails found_details;
    CachedPreviewWidgets cachedWidgets;
    std::string found_object_path;

    QSharedPointer<click::DownloadManager> dm;
};

// TODO: this is only necessary to perform uninstall.
// That should be moved to a separate action, and this class removed.
class UninstallingPreview : public UninstalledPreview
{
public:
    UninstallingPreview(const unity::scopes::Result& result,
                        const unity::scopes::ActionMetadata& metadata,
                        const QSharedPointer<click::web::Client>& client,
                        const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                        const QSharedPointer<pay::Package>& ppackage);

    virtual ~UninstallingPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

protected:
    void uninstall();

};

class CancellingPurchasePreview : public UninstallingPreview
{
public:
    CancellingPurchasePreview(const unity::scopes::Result& result,
                              const unity::scopes::ActionMetadata& metadata,
                              const QSharedPointer<click::web::Client>& client,
                              const QSharedPointer<pay::Package>& ppackage,
                              const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                              bool installed);

    virtual ~CancellingPurchasePreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

protected:
    void cancel_purchase();
    bool installed;
};

} // namespace click

#endif
