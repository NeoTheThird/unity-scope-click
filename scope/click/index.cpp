#include "index.h"

namespace click
{

Index::Index(const QSharedPointer<click::web::Service>& service) : service(service)
{

}

void Index::search (std::string query, std::function<void(std::list<click::Package>)> callback)
{
    Q_UNUSED(query)
    Q_UNUSED(callback)
    click::web::CallParams params;
    service->call("xx", params);
}

Index::Index()
{
}

Index::~Index()
{
}

} // namespace click
