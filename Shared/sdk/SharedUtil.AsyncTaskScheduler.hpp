#include "SharedUtil.AsyncTaskScheduler.h"

namespace SharedUtil
{
    CAsyncTaskScheduler::CAsyncTaskScheduler(std::size_t numWorkers)
       : m_Workers(numWorkers)
    {
    }

    CAsyncTaskScheduler::~CAsyncTaskScheduler()
    {
        m_Workers.clear();
    }

    void CAsyncTaskScheduler::CollectResults(const bool awaitResults)
    {
        // await results maybe
        if (awaitResults)
        {
            for (auto& worker : m_Workers)
                worker.Await();
        }

        for (auto& worker : m_Workers)
            worker.ProcessResults();
    }

    void CAsyncTaskScheduler::Worker::DoWork()
    {
        while (m_Running)
        {
            m_TasksMutex.lock();

            // if there are no tasks sleep a little
            if (m_Tasks.empty())
            {
                m_TasksMutex.unlock();

                std::this_thread::sleep_for(std::chrono::milliseconds(4));
                continue;
            }

            // Pop task from front
            std::unique_ptr<SBaseTask> pTask = std::move(m_Tasks.front());
            m_Tasks.pop();

            m_TasksMutex.unlock();

            // Do prolly time consuming work..
            pTask->Execute();

            // Put result into queue if needed
            if (pTask->HasResult())
            {
                std::lock_guard<std::mutex> guard{ m_ResultsMutex };

                m_Results.push_back(std::move(pTask));
                m_NeedsResultCollection = true;
            }
        }
    }
}; // namespace SharedUtil
