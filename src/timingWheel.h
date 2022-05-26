#pragma once
#include <stdint.h>
#include <vector>
#include <list>
#include <string>
#include <map>
#include <set>

#define MAX_WHEEL_COUNT 5       // 轮子数
#define UNIT_SCALE_VALUE 20     // 单位刻度（20ms）
int WheelScale[MAX_WHEEL_COUNT] = { 1000 / UNIT_SCALE_VALUE, 3600, 24, 360, 100 };

typedef int(__cdecl *TIMING_WHEEL_TASK_CB)(std::string m_uniqueID);

struct twTask
{
    int m_activeCount;
    int m_cycle;
    TIMING_WHEEL_TASK_CB m_func;
    std::string m_uniqueID;

    twTask()
    {
        m_activeCount = 0;
        m_cycle = 0;
        m_func = nullptr;
    }
};

struct WheelSlot
{
    typedef std::list<twTask*>::iterator twTaskIter;

    std::list<twTask*> m_list;

    WheelSlot()
    {
        twTask* rlt = new twTask;
        m_list.push_back(rlt);
    }
    
    twTaskIter addTask(twTask* task)
    {
        std::list<twTask*>::const_iterator end_it = m_list.cend();
        return m_list.insert(end_it++, task);
    }
};

struct OneWheel
{
    int m_scale;
    int m_curPos;
    std::vector<WheelSlot> m_slot;

    OneWheel(int scale = 0, int slotCount = 0) :m_scale(scale), m_curPos(0)
    {
        for (int i = 0; i < slotCount; ++i)
        {
            WheelSlot ws;
            m_slot.push_back(ws);
        }
    }
};

struct TimingWheel
{
    int64_t m_nTickTime;
    OneWheel m_twOneWheel[MAX_WHEEL_COUNT];
    std::map<std::string, std::set<std::string>> m_searchMap;

    TimingWheel(int64_t tickTime = 0):m_nTickTime(tickTime)
    {
        int nScale = 0;
        for (int i = 0; i < MAX_WHEEL_COUNT; ++i)
        {
            nScale = i == 0 ? UNIT_SCALE_VALUE : nScale * WheelScale[i - 1];
            m_twOneWheel[i] = OneWheel(nScale, WheelScale[i]);
        }
    }

    bool AddTask(twTask* newTask)
    {
        AddTask(newTask->m_uniqueID, newTask->m_cycle, newTask->m_activeCount, newTask->m_func);
    }

    bool AddTask(std::string uniqueID, int cycle, int count, TIMING_WHEEL_TASK_CB func)
    {
        for (int i = 0 ; i < MAX_WHEEL_COUNT; ++i)
        {
            if (cycle < m_twOneWheel[i].m_scale*m_twOneWheel[i].m_slot.size())
            {
                int pos = (m_twOneWheel[i].m_curPos + cycle / m_twOneWheel[i].m_scale) % int(m_twOneWheel[i].m_slot.size());
                twTask* newTask = new twTask;
                newTask->m_activeCount = count;
                newTask->m_func = func;
                newTask->m_uniqueID = uniqueID;
                newTask->m_cycle = cycle;
                m_twOneWheel[i].m_slot[pos].addTask(newTask);
                return true;
            }
        }

        return false;
    }

    void TransTask(OneWheel& oneWheel)
    {
        WheelSlot::twTaskIter begin_it = oneWheel.m_slot[oneWheel.m_curPos].m_list.begin();
        for (; begin_it != oneWheel.m_slot[oneWheel.m_curPos].m_list.end(); ++begin_it)
        {
            AddTask(*begin_it);
        }
    }

    void DoTask(OneWheel& oneWheel)
    {
        WheelSlot::twTaskIter begin_it = oneWheel.m_slot[oneWheel.m_curPos].m_list.begin();
        for (; begin_it != oneWheel.m_slot[oneWheel.m_curPos].m_list.end(); ++begin_it)
        {
            (*begin_it)->m_func((*begin_it)->m_uniqueID);
            (*begin_it)->m_activeCount -= 1;
            if ((*begin_it)->m_activeCount > 0)
            {
                AddTask(*begin_it);
            }
        }

        oneWheel.m_slot[oneWheel.m_curPos].m_list.clear();
    }

    void tickOnce()
    {
        auto slotMove = [](OneWheel& oneWheel) {
            oneWheel.m_curPos++;
            if (oneWheel.m_curPos >= oneWheel.m_slot.size())
            {
                oneWheel.m_curPos = 0;
            }
        };

        for (int i = 0; i < MAX_WHEEL_COUNT; ++i)
        {
            if (i == 0)
            {
                DoTask(m_twOneWheel[i]);
                m_nTickTime += m_twOneWheel[i].m_scale;
                slotMove(m_twOneWheel[i]);
            }
            else
            {
                slotMove(m_twOneWheel[i]);
                TransTask(m_twOneWheel[i]);
            }
            
            if (m_twOneWheel[i].m_curPos != 0)
            {
                break;
            }
        }
    }

    void tickNow(int64_t nowTick)
    {
        if (m_nTickTime < nowTick)
        {
            tickOnce();
        }
    }
};

TimingWheel NewTimingWheel(int64_t curTick)
{
    return TimingWheel(curTick);
}