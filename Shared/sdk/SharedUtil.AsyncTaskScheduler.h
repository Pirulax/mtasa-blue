#pragma once
#include <queue>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

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

            // Executes the passed in time consuming function
            virtual void Execute() = 0;

            // Should be called from the main thread to avoid threading issues
            virtual void ProcessResult() = 0;
        };

        template <typename TaskFunction_t, typename ReadyFunction_t>
        struct STask final : public SBaseTask
        {
            using Result_t = std::invoke_result_t<TaskFunction_t>;

            TaskFunction_t  m_TaskFunction;
            ReadyFunction_t m_ReadyFunction;
            Result_t        m_Result;

            STask(TaskFunction_t&& taskFunc, ReadyFunction_t&& readyFunc) : m_TaskFunction(taskFunc), m_ReadyFunction(readyFunc) {}

            void Execute() override { m_Result = std::move(m_TaskFunction()); }

            void ProcessResult() override { m_ReadyFunction(m_Result); }
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
        // Pushes a new task into the queue.
        //
        // taskFunc: Time-consuming function that is executed on the secondary thread (be aware of thread safety!)
        // readyFunc: Function that is called once the result is ready (called on the main thread)
        //
        // Note: If you don't need a `ready function` consider using `SharedUtil::async` instead
        // (Ctrl + T, type in `async`, press enter)
        //
        template <typename TaskFunction_t, typename ReadyFunction_t>
        void PushTask(TaskFunction_t&& taskFunc, ReadyFunction_t&& readyFunc)
        {
            // Forward arguments..
            auto task = std::make_unique<STask>(std::forward<TaskFunction_t>(taskFunc), std::forward<ReadyFunction_t>(readyFunc));

            // push task into the queue
            {
                std::lock_guard lock{ m_TasksMutex };
                m_Tasks.push(std::move(task));
            }
            m_SignalCV.notify_one();
        }

        //
        // Collects (polls) the results of the finished tasks
        // and invokes its ready-functions on the main thread
        // THIS FUNCTION MUST BE CALLED ON THE MAIN THREAD
        //
        void CollectResults();

    protected:
        void DoWork();

    private:
        using TaskList_t          = std::queue<std::unique_ptr<SBaseTask>>;
        using ProcessedTaskList_t = std::vector<std::unique_ptr<SBaseTask>>;

        std::vector<std::thread> m_Workers;
        bool                     m_Running = true;

        std::condition_variable  m_SignalCV;

        TaskList_t m_Tasks;
        std::mutex m_TasksMutex;

        ProcessedTaskList_t m_TaskResults;
        std::mutex          m_TaskResultsMutex;
    };
}            // namespace SharedUtil
