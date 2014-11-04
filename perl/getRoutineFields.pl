use strict;
use warnings;

use Getopt::Std;
use File::Path qw(make_path);
use XML::Simple;
use Data::Dumper;
use XML::LibXML;

use vars qw( $opt_f $opt_r $opt_h $opt_s);

sub print_help
{
	print "Usage: perl $0 -f [xml file] -s [sql start id number]\n";
	exit 1;
}

if ( ! getopts("hr:f:s:")) {
	print "getopts failed\n";
	print_help();
}

if (!$opt_f) {
	print "parameters empty ".$opt_f."\n";
	print_help();
}

if (!$opt_s) {
	print "parameters empty ".$opt_s."\n";
	print_help();
}

if (!$opt_r) {
	print "parameters empty ".$opt_r."\n";
	print_help();
}

my $src_file = $opt_f;
my $routines_dir = $opt_r;

if ($src_file !~ /^\.\//) {
  if (($src_file !~ /^\.\.\//)) {
	$src_file = "./".$src_file;
  }
}

if ($routines_dir !~ /^\.\//) {
  if (($routines_dir !~ /^\.\.\//)) {
	$routines_dir = "./".$routines_dir;
  }
}

print "Source xml file: ".$src_file."\n";
print "Source routines xml file: ".$routines_dir."\n";

my $sql_start_id_number = $opt_s;
print "Start number: ".$sql_start_id_number."\n";

my $tmp_path = $src_file;
$tmp_path =~ m/^.+\//;
my $output_file_path = $&;

print "Output files path: ".$output_file_path."\n";

my $parser = XML::LibXML->new();
my $doc    = $parser->parse_file($src_file);

my($routine_form) = $doc->findnodes('/si.irose.app.gea.asset.store.MixedObjectsStore/__responses/si.irose.gea.mas.syncserver.LoadSiteDataResponse/__anyObjectResponse/objects/routineForm');

my($view_control) = $doc->findnodes('/si.irose.app.gea.asset.store.MixedObjectsStore/__responses/si.irose.gea.mas.syncserver.LoadSiteDataResponse/__anyObjectResponse/objects/routineForm/viewControl');
my $nodeName;
my $routineId;
my %bindingNameHash;
my $routine_title;

for my $rfNode ($routine_form->findnodes('./*')) {
	if ($rfNode->nodeName() =~ /title/) {
		$routine_title = $rfNode->textContent();
	}
}

for my $vcNode ($view_control->findnodes('./*')) {
#	print $vcNode->nodeName(), "\n";
	$nodeName = $vcNode->nodeName();
	if ($nodeName =~ /ctrlId/) {
		$routineId = $vcNode->textContent();
	}
	
	if ($nodeName =~ /sections/) {
		for my $section_control ($vcNode->findnodes('./*')) {
#			print $section_control->nodeName()."\n";
			for my $field_control ($section_control->findnodes('./tables/tableControl/fields/fieldControl')) {
#				print "  ".$field_control->nodeName()."\n";
				my $bindingName = "";
				my $readOnly = "";
				for my $fcNode ($field_control->findnodes('./*')) {
					#print "    ".$fcNodes->nodeName()."\n";
					my $fcNodeName = $fcNode->nodeName();
					if ($fcNodeName =~ /bindingField/) {
						$bindingName = $fcNode->textContent();
					}
					for my $nodeReadOnly ($fcNode->findnodes('./readOnly')) {
#						print $nodeReadOnly->nodeName();
						$readOnly = $nodeReadOnly->textContent();
					}
				}
				#print $bindingName." - ".$readOnly."\n";
				if ($bindingName) {
					if ($readOnly =~ /false/) {
			#			print $bindingName." - ".$readOnly."\n";
						$bindingNameHash{ $bindingName } = $readOnly;
					}
				}
			}
#			print "\n";
		}
	}
}

# my $parserRoutines = XML::LibXML->new();
# my $docRoutines    = $parser->parse_file($routines_file);

my @routines_files;
$routines_dir =~ s/\/^//;
opendir (DIR, $routines_dir) or die $!;
while (my $file = readdir(DIR)) {
	print "$file\n";
	if ($file =~ /\d+__dl/) {
		push @routines_files, $file;
	}
}

my $routines_file_data = "";
print "Found files\n";
for my $file (@routines_files) {
	print $file."\n";
	open FILE, $routines_dir."/".$file or die "Couldn't open file: $!"; 
	$routines_file_data .= join("", <FILE>); 
	close FILE;
}

my %routine_mapping;
my $routineIdRef;
my $routineMappingIdRef;
my @routines = ($routines_file_data =~ /<routine>(.*?)<\/routine>/sg);
print "Number of routines: ".scalar(@routines)."\n";
for my $routine (@routines) {
	my @entries = ($routine =~ /<entry>(.*?)<\/entry>/sg);
	print "Number of entries: ".scalar(@entries)."\n";
	for my $entry (@entries) {
		if ($entry =~ /ows_ID/) {
			($routineIdRef) = $entry =~ /<string>.*<\/string>.*<string>(.*?)<\/string>/sg;
		}
		
		if ($entry =~ /ows_MappingId/) {
			($routineMappingIdRef) = $entry =~ /<string>.*<\/string>.*<string>(.*?)<\/string>/sg;
		}
	}
	print $routineIdRef." - ".$routineMappingIdRef."\n";
	$routine_mapping{ $routineIdRef } = $routineMappingIdRef;
}

my @hash_keys = keys %bindingNameHash;
my $sql_insert = "";
for my $key (keys %bindingNameHash) {
	$sql_insert .= "insert into routine_hidden_fields (id, routine_mapping_id, attr_key, attr_value) values (".$sql_start_id_number.", '".$routine_mapping{$routineId}."', '".$key."', 'Default');\n";
	$sql_start_id_number++;
}
print @hash_keys."\n";
print "Routine title: ".$routine_title."\n";
print $sql_insert."\n";