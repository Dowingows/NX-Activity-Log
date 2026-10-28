#ifndef APPLICATION_SENDJOB_HPP
#define APPLICATION_SENDJOB_HPP

#include "Application.hpp"
#include "Aether/ThreadPool.Job.hpp"

namespace Main {
    // Sends stored, completed play sessions only when explicitly requested.
    class Application::SendJob : public Aether::ThreadPool::Job {
        private:
            Application * app;
            std::atomic<double> & percent;
            std::string & result;

        protected:
            void work();

        public:
            SendJob(Application *, std::atomic<double> &, std::string &);
    };
};

#endif
