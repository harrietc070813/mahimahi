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
    const uint64_t delay_budget_,
    const std::string logfile)
    : attack_rate(attack_rate_),
      k(k_),
      delay_budget(delay_budget_),
      initial_rate(125), // 4Mpbs
      input_rate(attack_rate_),
      acc_delay(0),
      arrival_rate(0),
      current_arrival_rate(attack_rate_),
      state(CRUISE),
      packet_queue_(),
      logfile(logfile),
      log_()
{
    // adding the logging fucntionality
    if (!logfile.empty())
    {
        log_.reset(new ofstream(logfile));
        if (!log_->is_open())
        {
            cout << logfile << ": error opening for writing" << endl;
            exit(EXIT_FAILURE);
        }

        *log_ << "# mm-bbr-attack log" << endl;
        *log_ << "# attack_rate: " << attack_rate << " queue_size: " << k << " delay_budget: " << delay_budget << endl;
        *log_ << "# base timestamp: " << timestamp() << endl;
        *log_ << "# queue: " << "droptail [bytes=1000000]" << endl;
        *log_ << "# init timestamp: " << initial_timestamp() << endl;
    }
}

void BBRAttackQueue::computeDelay(Packet &p)
{
    static uint64_t counter = timestamp();

    if (timestamp() - counter <= 30000)
    {
        attack_rate = initial_rate;
    }
    else
    {
        attack_rate = input_rate;
    }

    double total_delay = acc_delay + (double)p.contents.size() / attack_rate;
    const uint64_t d = (uint64_t)total_delay;
    acc_delay = total_delay - d;

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

    // Log the delay calculation
    if (log_)
    {
        *log_ << p.arrival_time << " DELAY_CALCULATED: " << (p.dequeue_time - p.arrival_time) << " ms" << std::endl;
        log_->flush();
    }
}

void BBRAttackQueue::read_packet(const string &contents)
{
    uint64_t now = timestamp();
    Packet p = {now, now, contents};
    computeDelay(p);
    packet_queue_.emplace(p);

    // Log the arrival
    if (log_)
    {
        *log_ << now << " + " << contents.size() << std::endl;
        log_->flush();
    }
}

void BBRAttackQueue::write_packets(FileDescriptor &fd)
{
    while ((!packet_queue_.empty()) && (packet_queue_.front().dequeue_time <= timestamp()))
    {
        fd.write(packet_queue_.front().contents);
        // Log the departure
        if (log_)
        {
            *log_ << packet_queue_.front().dequeue_time << " - " << packet_queue_.front().contents.size() << std::endl;
            log_->flush();
        }
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
