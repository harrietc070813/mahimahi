/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef COPA_ATTACK_QUEUE_HH
#define COPA_ATTACK_QUEUE_HH

#include <queue>
#include <cstdint>
#include <string>
#include <fstream>
#include <memory>

#include "file_descriptor.hh"

struct Packet
{
    uint64_t arrival_time;
    uint64_t dequeue_time;
    const std::string contents;
};

class CopaAttackQueue
{
private:
    uint64_t delay_budget_;
    uint64_t duration_;
    uint64_t interval_;
    std::queue<Packet> packet_queue_;
    std::unique_ptr<std::ofstream> log_;
    std::unique_ptr<std::ofstream> attack_log_;
    double acc_delay;
    uint64_t last_send_time;
    /* release timestamp, contents */

public:
    CopaAttackQueue(const uint64_t &delay_budget, const uint64_t &duration, const uint64_t &interval, const std::string &link_log, const std::string &attack_log);

    uint64_t computeDelay(const std::string &contents, uint64_t arrival_time);

    void read_packet(const std::string &contents);

    void write_packets(FileDescriptor &fd);

    unsigned int wait_time(void) const;

    bool pending_output(void) const { return wait_time() <= 0; }

    static bool finished(void) { return false; }

    void record_arrival(const uint64_t arrival_time, const size_t pkt_size);
    // void record_drop(const uint64_t time, const size_t pkts_dropped, const size_t bytes_dropped);
    // void record_departure_opportunity(void);
    void record_departure(const uint64_t departure_time, const uint64_t arrival_time, const size_t pkt_size);
};

#endif /* COPA_ATTACK_QUEUE_HH */
