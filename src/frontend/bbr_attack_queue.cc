/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>
#include <chrono>
#include <cassert>
#include <iostream>
#include <unistd.h>

#include "bbr_attack_queue.hh"
#include "timestamp.hh"

// #define DEBUG_MODE

using namespace std;

BBRAttackQueue::BBRAttackQueue(
    const double attack_rate_,
    const uint64_t k_,
    const uint64_t delay_budget_)
    : attack_rate(attack_rate_), // Bytes per millisecond
      k(k_),
      delay_budget(delay_budget_),
      arrival_rate(0),
      current_arrival_rate(attack_rate_),
      state(CRUISE),
      packet_queue_()
{
}

void BBRAttackQueue::detectState(Packet &p)
{
    const double probe_gain = 1.2; // This is a loose bound. The exact gain is 1.25
    const double drain_gain = 0.8; // This is a loose bound. The exact gain is 0.75
    // const double slow_start_bound = 1.8; // This is a loose bound. The exact gain is 2

    if (!packet_queue_.empty())
    {
        Packet prev = packet_queue_.back();
        current_arrival_rate = (double)p.contents.size() / (p.arrival_time - prev.arrival_time);

        if (current_arrival_rate >= probe_gain * arrival_rate && state == CRUISE)
            state = PROBE;
        else if (current_arrival_rate <= drain_gain * arrival_rate && state == PROBE)
            state = DRAIN;
        else
        {
            arrival_rate = current_arrival_rate; // Update arrival_rate
            state = CRUISE;
        }
    }
}

void BBRAttackQueue::computeDelay(Packet &p)
{
    const uint64_t d = (double)p.contents.size() / attack_rate;
    uint64_t last = p.arrival_time;
    if (!packet_queue_.empty())
    {
        Packet prev = packet_queue_.back();
        last = max(prev.dequeue_time, last);
    }
    p.dequeue_time = last + d;

#ifdef DEBUG_MODE
    uint64_t delay_added = p.dequeue_time - p.arrival_time;
    std::cout << getpid() << ", " << packet_queue_.size() << ", " << delay_added << ", " << p.contents.size() << ", " << p.arrival_time << endl;
#endif

    assert(p.dequeue_time - p.arrival_time <= delay_budget);
}

void BBRAttackQueue::read_packet(const string &contents)
{
    uint64_t now = timestamp();
    Packet p = {now, now, contents};
    // detectState(p);
    computeDelay(p);
    packet_queue_.emplace(p);
}

void BBRAttackQueue::write_packets(FileDescriptor &fd)
{
    while ((!packet_queue_.empty()) && (packet_queue_.front().dequeue_time <= timestamp()))
    {
        fd.write(packet_queue_.front().contents);
        packet_queue_.pop();
    }
}

unsigned int BBRAttackQueue::wait_time(void) const
{
    if (packet_queue_.empty())
    {
        return numeric_limits<uint16_t>::max();
    }

    const auto now = timestamp();

    if (packet_queue_.front().dequeue_time <= now)
    {
        return 0;
    }
    else
    {
        return packet_queue_.front().dequeue_time - now;
    }
}
