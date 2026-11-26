/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

#include "ZenLib/Thread.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

using namespace ZenLib;

namespace
{

std::atomic<int> Started(0);
std::atomic<int> Destroyed(0);

bool Check(bool Condition, const char* Message)
{
    if (Condition)
        return true;

    std::cerr << Message << '\n';
    return false;
}

class Test_Thread : public Thread
{
public:
    explicit Test_Thread(bool WaitForTerminate_) : WaitForTerminate(WaitForTerminate_) {}

    ~Test_Thread()
    {
        //Same wait as the destructors of MediaInfoLib classes running a thread
        if (IsRunning() || IsTerminating())
        {
            RequestTerminate();
            while (!IsExited())
                Yield();
        }
        Destroyed++;
    }

    void Entry()
    {
        Started++;
        if (WaitForTerminate)
            while (!IsTerminating())
                Yield();
    }

private:
    bool WaitForTerminate;
};

bool WaitFor(const std::atomic<int>& Value, int Expected)
{
    for (int i=0; i<1000 && Value!=Expected; i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return Value==Expected;
}

bool DeleteNotStarted()
{
    Test_Thread* T=new Test_Thread(false);
    T->RequestTerminateAndDelete();

    return Check(Destroyed==1, "An object of a thread not started was not deleted at once");
}

bool DeleteEnded()
{
    Test_Thread* T=new Test_Thread(false);
    if (!Check(T->Run()==Thread::Ok, "A thread did not start"))
        return false;
    for (int i=0; i<1000 && !T->IsExited(); i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (!Check(T->IsExited(), "A thread did not end"))
        return false;
    T->RequestTerminateAndDelete();

    return Check(Destroyed==2, "An object of an ended thread was not deleted at once");
}

bool DeleteRunning()
{
    Test_Thread* T=new Test_Thread(true);
    if (!Check(T->Run()==Thread::Ok, "A thread did not start"))
        return false;
    if (!Check(WaitFor(Started, 2), "A thread did not reach its entry"))
        return false;
    T->RequestTerminateAndDelete(); //The thread waits for this request

    return Check(WaitFor(Destroyed, 3), "An object of a running thread was not deleted at its end");
}

} // unnamed namespace

int main()
{
    if (!DeleteNotStarted())
        return 1;
    if (!DeleteEnded())
        return 1;
    return DeleteRunning() ? 0 : 1;
}
