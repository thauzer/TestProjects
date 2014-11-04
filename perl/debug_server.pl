use IO::Socket::INET;

# auto-flush on socket
$| = 1;

# creating a listening socket
my $socket = new IO::Socket::INET (
LocalHost => '0.0.0.0',
LocalPort => '33555',
Proto => 'tcp',
Listen => 5,
Reuse => 1,
Blocking => '1'
);
die "cannot create socket $!\n" unless $socket;
print "server waiting for client connection on port 7777\n";

my $ret_val = undef;

while(1)
{
	# waiting for a new client connection
	my $client_socket = $socket->accept();

	# get information about a newly connected client
	my $client_address = $client_socket->peerhost();
	my $client_port = $client_socket->peerport();
	print "connection from $client_address:$client_port\n";

	# read up to 1024 characters from the connected client
	my $data = "";
	my $counter = 0;
	while(1) {
		$line = <$client_socket>;
		print $line;
		if ($counter == 10) {
			last;
		}
		$counter++;
	}

	# write response data to the connected client
	$data = "ok";
	$client_socket->send($data);

	# notify client that response has been sent
	shutdown($client_socket, 1);
}

$socket->close();