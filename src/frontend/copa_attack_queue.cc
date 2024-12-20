/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>

#include "copa_attack_queue.hh"
#include "timestamp.hh"

using namespace std;

CopaAttackQueue::CopaAttackQueue(const uint64_t &delay_budget, const uint64_t &duration,
                                 const uint64_t &interval, const string &link_log,
                                 const std::string &attack_log) : delay_budget_(delay_budget),
                                                                  duration_(duration),
                                                                  interval_(interval),
                                                                  packet_queue_(),
                                                                  log_(),
                                                                  attack_log_(),
                                                                  acc_delay(0),
                                                                  last_send_time(0)
{
    /* open logfile if called for */
    if (not link_log.empty())
    {
        log_.reset(new ofstream(link_log));
        if (not log_->good())
        {
            throw runtime_error(link_log + ": error opening for writing");
        }

        *log_ << "# queue: " << "droptail [bytes=1000000]" << endl;
        *log_ << "# init timestamp: " << initial_timestamp() << endl;
        *log_ << "# base timestamp: " << timestamp() << endl;
        const char *prefix = getenv("MAHIMAHI_SHELL_PREFIX");
        if (prefix)
        {
            *log_ << "# mahimahi config: " << prefix << endl;
        }
    }

    if (not attack_log.empty())
    {
        attack_log_.reset(new ofstream(attack_log));
        if (not attack_log_->good())
        {
            throw runtime_error(attack_log + ": error opening for writing");
        }
    }
}

void CopaAttackQueue::record_arrival(const uint64_t arrival_time, const size_t pkt_size)
{
    /* log it */
    if (log_)
    {
        *log_ << arrival_time << " + " << pkt_size << endl;
    }
}

void CopaAttackQueue::record_departure(const uint64_t departure_time, const uint64_t arrival_time, const size_t pkt_size)
{
    /* log the delivery */
    if (log_)
    {
        *log_ << departure_time << " - " << pkt_size
              << " " << departure_time - arrival_time << endl;
    }
}

uint64_t CopaAttackQueue::computeDelay(const string &contents, uint64_t arrival_time)
{
    const double attack_rate = 125; // 1pkt/ms

    double total_delay = acc_delay + (double)contents.size() / attack_rate;
    const uint64_t d = (uint64_t)total_delay;
    acc_delay = total_delay - d;

    uint64_t last = arrival_time;
    if (!packet_queue_.empty())
    {
        Packet prev = packet_queue_.back();
        last = max(prev.dequeue_time, last);
    }
    return last + d;
}

void CopaAttackQueue::read_packet(const string &contents)
{
    uint64_t now = timestamp();
    record_arrival(now, contents.size());

    uint64_t dequeue_time = now;
    uint64_t delay = 0;

    uint64_t interval = duration_ + interval_;
    uint64_t duration = duration_;
    uint64_t init_start = 400;
    uint64_t init_end = 600;

    // Only add delay once in a delay-adding phase
    if (now > init_start && now < init_end)
    {
        delay = computeDelay(contents, now);
        last_send_time = now;
    }
    else if (now >= init_end)
    {
        if (now - last_send_time > interval)
        {
            delay = delay_budget_;
            last_send_time = now;
        }
        else if (now - last_send_time < duration)
        {
            delay = delay_budget_;
        }
    }
    // Log computed delay
    if (attack_log_)
    {
        *attack_log_ << "timestamp " << now << ", delay " << delay << endl;
    }

    dequeue_time += delay;
    Packet p = {now, dequeue_time, contents};
    packet_queue_.emplace(p);
}

void CopaAttackQueue::write_packets(FileDescriptor &fd)
{
    uint64_t now = timestamp();
    while ((!packet_queue_.empty()) && (packet_queue_.front().dequeue_time <= now))
    {
        Packet &p = packet_queue_.front();
        fd.write(p.contents);
        record_departure(now, p.arrival_time, p.contents.size());
        packet_queue_.pop();
    }
}

unsigned int CopaAttackQueue::wait_time(void) const
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
