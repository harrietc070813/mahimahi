/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>

#include "bbr_attack_queue.hh"
#include "util.hh"
#include "ezio.hh"
#include "packetshell.cc"

using namespace std;

int main(int argc, char *argv[])
{
    try
    {
        const bool passthrough_until_signal = getenv("MAHIMAHI_PASSTHROUGH_UNTIL_SIGNAL");

        /* clear environment while running as root */
        char **const user_environment = environ;
        environ = nullptr;

        const int arg_num = 4;
        check_requirements(argc, argv);

        if (argc < arg_num)
        {
            // todo
            throw runtime_error("Usage: mm-bbr-attack [attack rate] [queue size] [delay budget]");
        }

        const double attack_rate = myatof(argv[1]);
        const uint64_t queue_size = myatoi(argv[2]); // Expected number of packets in the queue
        const uint64_t delay_budget = myatoi(argv[3]);

        vector<string> command;

        if (argc == arg_num)
        {
            command.push_back(shell_path());
        }
        else
        {
            for (int i = arg_num; i < argc; i++)
            {
                command.push_back(argv[i]);
            }
        }

        PacketShell<BBRAttackQueue> bbr_attack_shell_app("bbr_attack", user_environment, passthrough_until_signal);

        bbr_attack_shell_app.start_uplink("[attack] ", command, attack_rate, queue_size, delay_budget);
        bbr_attack_shell_app.start_downlink(attack_rate, queue_size, delay_budget);
        return bbr_attack_shell_app.wait_for_exit();
    }
    catch (const exception &e)
    {
        print_exception(e);
        return EXIT_FAILURE;
    }
}
