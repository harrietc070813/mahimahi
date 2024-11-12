/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>

#include "copa_attack_queue.hh"
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

        check_requirements(argc, argv);

        int arg_num = 6;

        if (argc < arg_num)
        {
            throw runtime_error("Usage: " + string(argv[0]) + " delay_budget-milliseconds uplink_logfile downlink_logfile uplink_attacklog downlink_attacklog [command...]");
        }

        const uint64_t delay_budget = myatoi(argv[1]);
        const string uplink_filename = argv[2];
        const string downlink_filename = argv[3];
        const string uplink_attack_filename = argv[4];
        const string downlink_attack_filename = argv[5];

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

        PacketShell<CopaAttackQueue> delay_shell_app("copa_attack", user_environment, passthrough_until_signal);

        delay_shell_app.start_uplink("[copa_attack] ",
                                     command,
                                     delay_budget,
                                     uplink_filename,
                                     uplink_attack_filename);
        delay_shell_app.start_downlink(0, downlink_filename, downlink_attack_filename);
        return delay_shell_app.wait_for_exit();
    }
    catch (const exception &e)
    {
        print_exception(e);
        return EXIT_FAILURE;
    }
}
