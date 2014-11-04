use strict;
use warnings;

use Getopt::Std;
use File::Path qw(make_path);

use vars qw( $opt_f $opt_h );

sub print_help
{
	print "Usage: perl $0 -f [log file]\n";
	exit 1;
}

if ( ! getopts("hf:")) {
	print "getopts failed\n";
	print_help();
}

if (!$opt_f) {
	print "parameters empty ".$opt_f."\n";
	print_help();
}

my $src_log_file = $opt_f;

if ($src_log_file !~ /^\.\//) {
  if (($src_log_file !~ /^\.\.\//)) {
	$src_log_file = "./".$src_log_file;
  }
}

print "Source log file: ".$src_log_file."\n";

my $tmp_path = $src_log_file;
$tmp_path =~ m/^.+\//;
my $reqs_file_path = $&;

print "Request files path: ".$reqs_file_path."\n";

my $log_file_data;

open my $log_file, $src_log_file or die "Could not open $src_log_file: $!";

my $date_time;
my $id;
my $action;
my $customer;
my $req_file_name;

my $new_request;

my $build_request = 0;

my $path_with_action;

while( my $line = <$log_file>)  {   
#    print $line;
	if ($line =~ /Inbound Message$/) {
		$line =~ s/INFO\s*//;
		$line =~ s/\s*\[\[.*//;
		my ($msg_date) = $line =~ /(.*?)\s/;
		my ($msg_time) = $line =~ /\s(.*?)$/;
		$msg_date =~ s/-/_/g;
		$msg_time =~ s/:/_/g;
		$msg_time =~ s/,/_/g;
		$date_time = $msg_date."_".$msg_time;
	}

    if ($line =~ /^ID:/) {
		$line =~ s/ID: //;
		$line =~ s/\n//;
		$id = $line;
    }
    
    if ($line =~ /^--------------------------------------$/) { # save request
#		if (length($new_request) < 2000) {
#		  print $new_request."\n";
#		}
		if (length($new_request) > 0) {
		  ($action) = $new_request =~ /<name>(.*?)<\/name>/s;
		  ($customer) = $new_request =~ /<entry(.*?)<\/entry>/s;
		  $customer =~ s/.*>//;
		  $req_file_name = "soap_request_".$date_time."_".$id."_".$action."_".$customer.".xml";
		  
		  # create folder
		  $path_with_action = $reqs_file_path.$action."/";
		  make_path($path_with_action,{ verbose => 1});
		  
		  $new_request =~ s/(<\/.*?>)/$1\n/g;
		  $new_request =~ s/(<.*?\/>)/$1\n/g;
		  print "SOAP Request - Action: ".$action.", Customer: ".$customer."\n";
		  print "Saving to file: ".$path_with_action.$req_file_name."\n";
		  open(my $fh, '>', $path_with_action.$req_file_name) or die "Could not open file '$req_file_name' $!";
		  print $fh $new_request;
		  close $fh;
		}
		
		$build_request = 0;
    }
    
    if ($build_request) {
#		$line =~ s/(<\/.*?>)/$1\n/g;
# 		$line =~ s/(<.*?\/>)/$1\n/g;
		
		$new_request .= $line;
    }
    
    if ($line =~ /^Payload:/) {
#		if (length($line) < 2000) {
#		  print $line."\n";
#		}
		$line =~ s/Payload: //;
		$new_request = $line;
		
		$build_request = 1;
    }
}