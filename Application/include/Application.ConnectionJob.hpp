#ifndef APPLICATION_CONNECTIONJOB_HPP
#define APPLICATION_CONNECTIONJOB_HPP

#include "Application.hpp"
#include "Aether/ThreadPool.Job.hpp"

namespace Main {
    // Checks the configured PlayLog API health endpoint only on user request.
    class Application::ConnectionJob : public Aether::ThreadPool::Job {
        private:
            Application * app;
            std::atomic<double> & percent;
            std::string & result;

        protected:
            void work();

        public:
            ConnectionJob(Application *, std::atomic<double> &, std::string &);
    };
};

#endif
