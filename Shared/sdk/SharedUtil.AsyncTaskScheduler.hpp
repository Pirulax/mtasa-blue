#include "SharedUtil.AsyncTaskScheduler.h"

namespace SharedUtil
{
    CAsyncTaskScheduler::CAsyncTaskScheduler(std::size_t numWorkers)
    {
        for (std::size_t i = 0; i < numWorkers; ++i)
        {
            m_Workers.emplace_back(&CAsyncTaskScheduler::DoWork, this);
        }
    }

    CAsyncTaskScheduler::~CAsyncTaskScheduler()
    {
        m_Running = false;
        m_SignalCV.notify_all(); // Stop them all

        // Wait for all threads to end
        for (auto& thread : m_Workers)
        {
            if (thread.joinable())
                thread.join();
        }
    }

    void CAsyncTaskScheduler::CollectResults()
    {
        std::lock_guard<std::mutex> lock{m_TaskResultsMutex};

        for (auto& pTask : m_TaskResults)
        {
            pTask->ProcessResult();
        }
        m_TaskResults.clear();
    }

    void CAsyncTaskScheduler::DoWork()
    {
        while (m_Running)
        {
            std::unique_ptr<SBaseTask> task;

            // Grab a task...
            {
                std::unique_lock lock(m_TasksMutex);

                m_SignalCV.wait(lock); // Wait for a task...
                if (m_Tasks.empty()) // Make sure we actually have a task..
                    continue;

                task = std::move(m_Tasks.front());
                m_Tasks.pop();
            }

            // Execute time-consuming task
            task->Execute();

            // Put into result queue
            {
                std::lock_guard lock{ m_TaskResultsMutex };
                m_TaskResults.push_back(std::move(task));
            }
        }
    }
}            // namespace SharedUtil
