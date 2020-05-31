#pragma once

#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <algorithm>
#include <queue>
#include <list>
#include <type_traits>

namespace SharedUtil
{
    ///////////////////////////////////////////////////////////////
    //
    // CAsyncTaskScheduler class
    //
    // Asynchronously executes tasks in secondary worker threads
    // and returns the result back to the main thread
    //
    ///////////////////////////////////////////////////////////////
    class CAsyncTaskScheduler
    {
        struct SBaseTask
        {
            virtual ~SBaseTask() {}
            virtual void Execute() = 0;
            virtual void ProcessResult() = 0;
            virtual bool HasResult() = 0;
        };

        template <class TaskFunc_t, class ReadyFunc_t>
        struct STask : public SBaseTask
        {
            using Result_t           = std::invoke_result_t<TaskFunc_t>;

            using WrappedTaskFunc_t  = std::function<Result_t(void)>;
            using WrappedReadyFunc_t = std::function<void(Result_t&&)>;

            WrappedTaskFunc_t  m_TaskFunc;
            WrappedReadyFunc_t m_ReadyFunc;
            Result_t           m_Result;

            STask(TaskFunc_t&& taskFunc, ReadyFunc_t&& readyFunc) : m_TaskFunc(taskFunc), m_ReadyFunc(readyFunc) {}

            void Execute() override { m_Result = m_TaskFunc(); }
            void ProcessResult() override { m_ReadyFunc(std::forward<Result_t>(m_Result)); }
            bool HasResult() override { return true; }
        };

        template <class TaskFunc_t>
        struct STaskNoResult : public SBaseTask
        {
            using Result_t = std::invoke_result_t<TaskFunc_t>;

            using WrappedTaskFunc_t = std::function<Result_t(void)>;

            WrappedTaskFunc_t  m_TaskFunc;
            Result_t           m_Result;

            STaskNoResult(TaskFunc_t&& taskFunc) : m_TaskFunc(taskFunc) {}

            void Execute() override { m_TaskFunc(); }
            void ProcessResult() override {}
            bool HasResult() override { return false; }
        };

    public:
        //
        // Creates a new async task scheduler
        // with a fixed number of worker threads
        //
        CAsyncTaskScheduler(std::size_t numWorkers);

        //
        // Ends all worker threads (waits for the last task to finish)
        //
        ~CAsyncTaskScheduler();

        //
        // Pushes a new task for execution once a worker is free
        // (Template Parameter) ResultType: The type of the result
        //
        // taskFunc: Time-consuming function that is executed on the secondary thread (be aware of thread safety!)
        // readyFunc: Function that is called once the result is ready (called on the main thread)
        //
        template <class TaskFunc_t, class ReadyFunc_t>
        void PushTask(TaskFunc_t&& taskFunc, ReadyFunc_t&& readyFunc)
        {
            // create task
            auto pTask = std::make_unique<STask<TaskFunc_t, ReadyFunc_t>>(std::forward<TaskFunc_t>(taskFunc), std::forward<ReadyFunc_t>(readyFunc));

            PushTaskToWorker(std::move(pTask));
        }

        template <class TaskFunc_t>
        void PushTask(TaskFunc_t&& taskFunc)
        {
            // create task
            auto pTask = std::make_unique<STaskNoResult<TaskFunc_t>>(std::forward<TaskFunc_t>(taskFunc));

            PushTaskToWorker(std::move(pTask));
        }

        //
        // Collects (polls) the results of the finished tasks
        // and invokes its ready-functions on the main thread
        // THIS FUNCTION MUST BE CALLED ON THE MAIN THREAD
        //
        // Specifying awaitResults to true will results in the
        // calling thread to be frozen(using std::this_thread::sleep_for)
        //
        void CollectResults(const bool awaitResults = false);

    private:
        void PushTaskToWorker(std::unique_ptr<SBaseTask>&& task)
        {
            // incerement last uset worker, make sure its correct
            if (++m_LastUsedWorker == m_Workers.end())
                m_LastUsedWorker = m_Workers.begin();

            (*m_LastUsedWorker).AddTask(std::move(task));
        }

    private:
        class Worker
        {
        public:
            Worker() : m_Thread(&Worker::DoWork, this) {}

            ~Worker()
            {
                m_Running = false;
                if (m_Thread.joinable())
                    m_Thread.join();
            }

            void DoWork();

            void AddTask(std::unique_ptr<SBaseTask>&& task)
            {
                std::lock_guard<std::mutex> guard(m_TasksMutex);
                m_Tasks.emplace(std::move(task));
            }

            void Await()
            {
                while (!m_Tasks.empty())
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            void ProcessResults()
            {
                if (!m_NeedsResultCollection)
                    return;

                m_NeedsResultCollection = false;

                std::lock_guard<std::mutex> guard(m_ResultsMutex);

                for (auto& task : m_Results)
                    task->ProcessResult();
                m_Results.clear();
            }


            typedef std::queue<std::unique_ptr<SBaseTask>>  Tasks;
            typedef std::vector<std::unique_ptr<SBaseTask>> Results;

            std::thread m_Thread;

            Tasks       m_Tasks;
            std::mutex  m_TasksMutex;

            Results     m_Results;
            std::mutex  m_ResultsMutex;
            bool        m_NeedsResultCollection = false;

            bool        m_Running = true;
        };

        typedef std::vector<Worker> WorkersList;
        WorkersList           m_Workers;
        WorkersList::iterator m_LastUsedWorker = m_Workers.begin();
    };
}            // namespace SharedUtil
