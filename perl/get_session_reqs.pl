#!/usr/bin/perl

# This script searches the files for a string in logs and pulls out whole part that is defined with start and end strings.
# All log parts are saved to a output file.

use strict;
use warnings;

use Sys::Hostname;
use File::Basename;
use Getopt::Std;
use Cwd 'abs_path';
use vars qw( $opt_h $opt_s $opt_f $opt_S $opt_E);

sub print_help
{
        # ./get_session_reqs.pl -s searchString -f logfile.log#logfile.log.1#logfile.log.2 -S "Try to accept new request" -E "finish request done"
        print "Usage: $0 -s [session id] -f [file or files, separated by #] -S [request start string] -E [request end string]\n";
        exit 1;
}

sub save_requests
{
        print "Saving to file\n";
        my $save_filename = $_[0];
        my @save_reqs = @{$_[1]};
print "Number of requests: " . scalar(@save_reqs) . "\n";
        open OUT_FH , ">>" , $save_filename;
        foreach my $file_request (@save_reqs) {
                print OUT_FH "START REQUEST\n";
                print OUT_FH $file_request;
                print OUT_FH "END_REQUEST\n------------------------------------------------------------------------\n";
        }
        close OUT_FH;
}

if ( ! getopts("hs:f:S:E:")) { print_help(); }
if (!$opt_s) { print_help(); }
if (!$opt_f) { print_help(); }
if (!$opt_S) { print_help(); }
if (!$opt_E) { print_help(); }

my $arg_session_id = $opt_s;
my $arg_file_names = $opt_f;
my $arg_req_start = $opt_S;
my $arg_req_end = $opt_E;

my @filenames = split "#",$arg_file_names;

print "Session id: " . $arg_session_id . "\n";
print "Arg file names: " . $arg_file_names . "\n";
print "File names: @filenames\n";
print "Start of the request to search: $arg_req_start\n";
print "End of the request to search: $arg_req_end\n";

my $req_start_found = 0;
my $request = "";
my $num_all_requests = 0;
my $num_matching_requests = 0;
my $num_matching_req_in_file = 0;
my $BUFFER_SIZE = 50;

my @req_buffer;
my $host = hostname;
my $output_file = "matching_requests_" . $host  . ".log";

open TEMP_FH , '>' , $output_file or die "Cannot create $output_file: $!\n";
close TEMP_FH or die "Cannot close $output_file: $!\n";

foreach my $file_name (@filenames) {
        print "Opening file: " . $file_name . "\n";

        open LOG_HANDLE , $file_name or die "Could not open $file_name: $!";

        $num_matching_req_in_file = 0;

        while (my $line = <LOG_HANDLE>) {
#               print $line;
                if ($line =~ /$arg_req_start/) {
                        $req_start_found = 1;
                }

                if ($req_start_found) {
                        $request = join ('', $request,$line);
                }

                if ($line =~ /$arg_req_end/) {
                        $req_start_found = 0;
                        $num_all_requests++;
                        if ($request =~ /$arg_session_id/) {
                                $num_matching_requests++;
                                $num_matching_req_in_file++;
                                push(@req_buffer, $request);

                                if (scalar(@req_buffer) > $BUFFER_SIZE) {
                                        # save requests to the file
                                        save_requests($output_file, \@req_buffer);
                                        undef(@req_buffer);
                                }
                        }
                        $request = "";
                }
        }

        if (@req_buffer) {
                # save remaining requests
print "saving remaining requests\n";
                save_requests($output_file, \@req_buffer);
                undef(@req_buffer);
        }

        print "Number of requests in this file: " . $num_matching_req_in_file . "\n";
        print "\n";
        close LOG_HANDLE;
}

print "All requests: " . $num_all_requests . "\n";
print "Matching requests: " . $num_matching_requests . "\n";
