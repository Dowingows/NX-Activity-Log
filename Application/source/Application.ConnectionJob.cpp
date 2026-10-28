#include "Application.ConnectionJob.hpp"
#include <ctime>
#include <fstream>
#include "nlohmann/json.hpp"
#include "utils/Curl.hpp"

namespace Main {
    namespace {
        const char * DEBUG_PATH = "/switch/PlayLog-Switch/playlog-connection-debug.json";

        std::string isoUtcNow() {
            const time_t value = std::time(nullptr);
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

    Application::ConnectionJob::ConnectionJob(Application * app, std::atomic<double> & percent, std::string & result)
        : Aether::ThreadPool::Job(), percent(percent), result(result) {
        this->app = app;
    }

    void Application::ConnectionJob::work() {
        this->percent = 5;
        const std::string endpoint = this->app->config()->playlogUrl();
        const std::string url = endpoint + (endpoint.empty() || endpoint.back() == '/' ? "" : "/") + "api/health";
        long status = 0;
        std::string response;
        std::string transportError;
        const bool transportOk = Utils::Curl::getJson(url, status, response, transportError);
        const bool success = transportOk && status >= 200 && status < 300;

        nlohmann::json debug;
        debug["endpoint"] = url;
        debug["checked_at"] = isoUtcNow();
        debug["transport_ok"] = transportOk;
        debug["http_status"] = status;
        debug["success"] = success;
        debug["response"] = response;
        debug["transport_error"] = transportError;
        const bool debugSaved = writeDebug(debug);

        if (success) {
            this->result = "Conexão com PlayLog OK (HTTP " + std::to_string(status) + ").";
        } else if (!transportError.empty()) {
            this->result = "Não foi possível conectar: " + transportError;
        } else {
            this->result = "O servidor respondeu HTTP " + std::to_string(status) + ".";
        }
        this->result += "\n" + std::string(debugSaved ? "Diagnóstico salvo em: " : "Não foi possível salvar o diagnóstico em: ") + DEBUG_PATH;
        this->percent = 100;
    }
};
