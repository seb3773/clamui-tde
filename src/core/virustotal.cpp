/*
 * virustotal.cpp - VirusTotal API client implementation
 */

#include "virustotal.h"
#include <tdelocale.h>
#include "../libs/http/tqtnetwork.h"
#include "../libs/json/tqtjson.h"

class VirusTotalClient::Private {
public:
    TQtNetworkAccessManager *netManager;
};

VirusTotalClient::VirusTotalClient(const TQString &apiKey, TQObject *parent)
    : TQObject(parent), m_apiKey(apiKey)
{
    d = new Private;
    d->netManager = new TQtNetworkAccessManager(this);
}

VirusTotalClient::~VirusTotalClient()
{
    delete d;
}

void VirusTotalClient::scanHash(const TQString &hash)
{
    if (m_apiKey.isEmpty()) {
        emit error(i18n("VirusTotal API key is not configured."));
        return;
    }

    TQString url = "https://www.virustotal.com/api/v3/files/" + hash;
    
    TQtNetworkRequest req(url);
    req.setRawHeader("x-apikey", m_apiKey.local8Bit());
    req.setRawHeader("Accept", "application/json");

    TQtNetworkReply *reply = d->netManager->get(req);
    
    connect(reply, TQT_SIGNAL(finished()), this, TQT_SLOT(requestFinished()));
    connect(reply, TQT_SIGNAL(error(const TQString&)), this, TQT_SIGNAL(error(const TQString&)));
}

void VirusTotalClient::requestFinished()
{
    TQtNetworkReply *reply = (TQtNetworkReply*)sender();
    if (!reply) return;
    
    int statusCode = reply->httpStatusCode();
    TQString response = TQString::fromLocal8Bit(reply->readAll());
    reply->deleteLater();
    
    VTScanResult res;
    
    if (statusCode == 404) {
        res.status = VT_NOT_FOUND;
        res.message = i18n("File not found in VirusTotal database.");
        emit scanResult(res);
        return;
    }
    
    if (statusCode == 401) {
        emit error(i18n("Invalid VirusTotal API key."));
        return;
    }
    
    if (statusCode != 200) {
        emit error(TQString(i18n("API request failed with status %1")).arg(statusCode));
        return;
    }

    TQtJsonDocument doc = TQtJsonDocument::fromJson(response);
    if (doc.isObject()) {
        TQtJsonObject root = doc.object();
        if (root.contains("data")) {
            TQtJsonObject data = root.value("data").toObject();
            if (data.contains("attributes")) {
                TQtJsonObject attrs = data.value("attributes").toObject();
                if (attrs.contains("last_analysis_stats")) {
                    TQtJsonObject stats = attrs.value("last_analysis_stats").toObject();
                    
                    res.status = VT_FOUND;
                    res.positives = stats.value("malicious").toDouble() + stats.value("suspicious").toDouble();
                    res.total = res.positives + stats.value("undetected").toDouble() + stats.value("harmless").toDouble();
                    
                    TQString id = data.value("id").toString();
                    res.permalink = "https://www.virustotal.com/gui/file/" + id;
                    
                    emit scanResult(res);
                    return;
                }
            }
        }
    }
    
    emit error(i18n("Failed to parse VirusTotal response."));
}

#include "virustotal.moc"
