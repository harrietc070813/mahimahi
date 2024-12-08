/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BBR_ATTACK_QUEUE_HH
#define BBR_ATTACK_QUEUE_HH

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

enum BBRPhase
{
    CRUISE,
    PROBE,
    DRAIN
};

class BBRAttackQueue
{
private:
    double attack_rate;
    const uint64_t k;
    const uint64_t delay_budget;
    const double initial_rate;
    const double input_rate;

    double acc_delay;

    double arrival_rate;
    double current_arrival_rate;
    BBRPhase state;
    std::queue<Packet> packet_queue_;

    std::string logfile;                 // Add logfile string member
    std::unique_ptr<std::ofstream> log_; // Add log_ for logging functionality

    void detectState(Packet &p);
    void computeDelay(Packet &p);

public:
    BBRAttackQueue(const double attack_rate_, const uint64_t k_, const uint64_t delay_budget_, const std::string logfile_ = "");

    void read_packet(const std::string &contents);

    void write_packets(FileDescriptor &fd);

    unsigned int wait_time(void) const;

    bool pending_output(void) const { return wait_time() <= 0; }

    static bool finished(void) { return false; }
};

#endif /* BBR_ATTACK_QUEUE_HH */
