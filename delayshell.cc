/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
// to run arping to send packet to ingress to/from machine with ip address addr  --> arping -I ingress addr

#include <sys/socket.h>
#include <pwd.h>
#include <paths.h>
#include <fstream>
#include <stdlib.h>

#include "tundevice.hh"
#include "exception.hh"
#include "ferry.hh"
#include "child_process.hh"
#include "system_runner.hh"
#include "nat.hh"

using namespace std;

/* get name of the user's shell */
string shell_path( void )
{
    struct passwd *pw = getpwuid( getuid() );
    if ( pw == nullptr ) {
        throw Exception( "getpwuid" );
    }

    string shell_path( pw->pw_shell );
    if ( shell_path.empty() ) { /* empty shell means Bourne shell */
      shell_path = _PATH_BSHELL;
    }

    return shell_path;
}

int main( int argc, char *argv[] )
{
    try {
	if ( argc != 2) {
	    throw Exception( "please enter only the one-way delay in ms" );
	}
	const uint64_t delay_ms = atoi( argv[1] );
        ifstream input;
        input.open("/proc/sys/net/ipv4/ip_forward");
        string line;
        getline( input, line );
        if ( line != "1" ) {
            throw Exception( "ip forwarding disabled" );
        }

        /* make pair of connected sockets */
        int pipes[ 2 ];
        if ( socketpair( AF_UNIX, SOCK_DGRAM, 0, pipes ) < 0 ) {
            throw Exception( "socketpair" );
        }
        FileDescriptor egress_socket( pipes[ 0 ], "socketpair" ),
            ingress_socket( pipes[ 1 ], "socketpair" );

        /* Fork */
        ChildProcess container_process( [&]()->int{
                /* Unshare network namespace */
                if ( unshare( CLONE_NEWNET ) == -1 ) {
                    throw Exception( "unshare" );
                }

                TunDevice ingress_tun( "ingress", "172.30.100.101", "172.30.100.100" );

                /* bring up localhost */
                run( "ip link set dev lo up" );

                run( "route add -net default gw 172.30.100.100" );

                /* Fork again */
                ChildProcess bash_process( []()->int{
                        const string shell = shell_path();
                        if ( execl( shell.c_str(), shell.c_str(), static_cast<char *>( nullptr ) ) < 0 ) {
                            throw Exception( "execl" );
                        }
                        return EXIT_FAILURE;
                    } );

                return ferry( ingress_tun.fd(), egress_socket, bash_process, delay_ms );
            } );

        TunDevice egress_tun( "egress", "172.30.100.100", "172.30.100.101" );

        /* set up NAT between egress and eth0 */
        NAT nat_rule;

        return ferry( egress_tun.fd(), ingress_socket, container_process, delay_ms );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}