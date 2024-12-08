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
    cout<<argc;
    for (int i=0; i<argc; i++)
    { 
        cout<<argv[i];
        cout<<"\n";
    }

    try
    {
        const bool passthrough_until_signal = getenv("MAHIMAHI_PASSTHROUGH_UNTIL_SIGNAL");

        /* clear environment while running as root */
        char **const user_environment = environ;
        environ = nullptr;

        const int arg_num =5;
        check_requirements(argc, argv);

        if (argc < arg_num)
        {
            // todo
            throw runtime_error("Usage: mm-bbr-attack [attack rate] [queue size] [delay budget] --attack-log=FILE");
        }

        const double attack_rate = myatof(argv[1]);
        const uint64_t queue_size = myatoi(argv[2]); // Expected number of packets in the queue
        const uint64_t delay_budget = myatoi(argv[3]);

        vector<string> command;
        string attack_logfile;

        //added the if statement

        if (argc ==arg_num)
        {
            command.push_back(shell_path());
        }
        
        for (int i = arg_num-1; i < argc; i++)
        {
            string arg = argv[i];
            cout << argc; 
            if (arg.rfind("--attack-log=", 0) == 0)  // Check if it starts with --attack-log=
            {
                attack_logfile = arg.substr(13);  // Extract the log file path
            }
            else
            {
                command.push_back(argv[i]);  // Add remaining arguments (for command)
            }
        }


        cout << "Command vector: ";
        for (const auto& cmd : command)
        {
            cout << cmd << " ";
        }
        cout << "\n";

        cout<<"Attack logfile " << attack_logfile<<"\n";
        cout << "Attack rate: " << attack_rate << "\n";
        cout << "Queue size: " << queue_size << "\n";
        cout << "Delay budget: " << delay_budget << "\n";


        PacketShell<BBRAttackQueue> bbr_attack_shell_app("bbr_attack", user_environment, passthrough_until_signal);

        bbr_attack_shell_app.start_uplink("[attack] ", command, attack_rate, queue_size, delay_budget, attack_logfile);
        bbr_attack_shell_app.start_downlink(attack_rate, queue_size, delay_budget); // Put condition for the last parameter
        return bbr_attack_shell_app.wait_for_exit();
    }
    catch (const exception &e)
    {
        print_exception(e);
        return EXIT_FAILURE;
    }
}
