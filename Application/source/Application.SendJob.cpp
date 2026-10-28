#include "Application.SendJob.hpp"
#include <ctime>
#include <fstream>
#include "nlohmann/json.hpp"
#include "utils/Curl.hpp"
#include "utils/Utils.hpp"

namespace Main {
    namespace {
        const char * DEBUG_PATH = "/switch/PlayLog-Switch/playlog-send-debug.json";

        std::string isoUtc(uint64_t timestamp) {
            time_t value = static_cast<time_t>(timestamp);
            struct tm utc;
            if (gmtime_r(&value, &utc) == nullptr) return "";
            char buffer[32];
            if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc) == 0) return "";
            return buffer;
        }

        bool writeDebug(const nlohmann::json & debug) {
            std::ofstream file(DEBUG_PATH, std::ios::out | std::ios::trunc);
            if (!file.is_open()) return false;
            file << debug.dump(2) << std::endl;
            return file.good();
        }
    }

    Application::SendJob::SendJob(Application * app, std::atomic<double> & percent, std::string & result)
        : Aether::ThreadPool::Job(), percent(percent), result(result) {
        this->app = app;
    }

    void Application::SendJob::work() {
        this->percent = 0;
        const std::string endpoint = this->app->config()->playlogUrl();
        const std::string url = endpoint + (endpoint.empty() || endpoint.back() == '/' ? "" : "/") + "api/sync/sessions";
        std::vector<std::string> payloads;

        // Build a stable, repeatable request body for each completed session.
        for (NX::User * user : this->app->users) {
            const std::string userId = Utils::formatHexString(user->ID().uid[0]) + Utils::formatHexString(user->ID().uid[1]);
            const std::string deviceId = "nxal-switch-" + userId;
            for (NX::Title * title : this->app->titles) {
                std::vector<NX::PlaySession> sessions = this->app->playdata_->getPlaySessionsForUser(title->titleID(), user->ID());
                for (const NX::PlaySession & session : sessions) {
                    // A session without a recorded close/focus-loss is still in progress.
                    // This POC intentionally sends historical sessions only.
                    if (session.startTimestamp == 0 || session.endTimestamp <= session.startTimestamp) continue;
                    if (session.playtime > 2592000) continue; // PlayLog contract: max 30 days.
                    const std::string startedAt = isoUtc(session.startTimestamp);
                    const std::string endedAt = isoUtc(session.endTimestamp);
                    if (startedAt.empty() || endedAt.empty()) continue;

                    nlohmann::json body;
                    body["session_id"] = "nxal:" + userId + ":" + Utils::formatHexString(title->titleID()) + ":" +
                        std::to_string(session.startTimestamp) + ":" + std::to_string(session.endTimestamp);
                    body["device_id"] = deviceId;
                    body["platform"] = "nintendo_switch";
                    body["game_title"] = title->name();
                    body["system"] = "Switch";
                    body["started_at"] = startedAt;
                    body["ended_at"] = endedAt;
                    body["duration_seconds"] = session.playtime;
                    body["status"] = "ended";
                    payloads.push_back(body.dump());
                }
            }
        }

        const size_t totalSessions = payloads.size();
        nlohmann::json debug;
        debug["endpoint"] = url;
        debug["attempted"] = 0;
        debug["succeeded"] = 0;
        debug["failed"] = 0;
        debug["requests"] = nlohmann::json::array();
        bool debugSaved = writeDebug(debug);

        for (size_t i = 0; i < payloads.size(); i++) {
            long status = 0;
            std::string response;
            std::string transportError;
            const bool networkOk = Utils::Curl::postJson(url, payloads[i], status, response, transportError);
            const bool success = networkOk && status >= 200 && status < 300;

            nlohmann::json entry;
            entry["payload"] = nlohmann::json::parse(payloads[i]);
            entry["payload_raw"] = payloads[i]; // Exact UTF-8 request body sent to the API.
            entry["http_status"] = status;
            entry["transport_ok"] = networkOk;
            entry["success"] = success;
            entry["response"] = response;
            entry["transport_error"] = transportError;
            debug["requests"].push_back(entry);
            debug["attempted"] = i + 1;
            debug[success ? "succeeded" : "failed"] = debug[success ? "succeeded" : "failed"].get<size_t>() + 1;
            debugSaved = writeDebug(debug);
            this->percent = 99.0 * (static_cast<double>(i + 1) / totalSessions);
        }

        this->result = "Envio concluído: " + std::to_string(debug["succeeded"].get<size_t>()) +
            " enviadas, " + std::to_string(debug["failed"].get<size_t>()) +
            " falhas, total " + std::to_string(totalSessions) + ".\n" +
            (debugSaved ? "Log salvo em: " : "Não foi possível salvar o log em: ") + DEBUG_PATH;
        this->percent = 99.9;
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        this->percent = 100;
    }
};
